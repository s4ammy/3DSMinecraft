import argparse
import hashlib
import json
import random
import struct
from pathlib import Path

from nativeMeshChecks import CheckBiome, CheckHiddenCase
from nativeMemoryChecks import CheckBuilderMemory
from nativeMenuChecks import CheckNativeMenus
from nativeScrollChecks import CheckScrollingLifetime, CheckScrollIntegration
from nativeLightingChecks import CheckLightReads
from advancedOverlayChecks import CheckAdvancedOverlay
from nativeWorldChecks import CheckWorldSettings
from checkLayeredFsCompatibility import CheckCandidate as CheckLayeredFs, InjectPath

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8,
    UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12, UC_ARM_REG_SP,
    UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_D0, UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC)


registers = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8,
    UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12]
stateAddress, hudAddress, stackTop, stopAddress = 0x2000000, 0x2008000, 0x203e000, 0x203f000
ticksPerSecond = 268111856


class CSettingsHarness:
    def __init__(self, code, symbols, layout):
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.cpu.mem_map(0x100000, 0xb00000)
        self.cpu.mem_write(0x100000, code)
        self.cpu.mem_map(stateAddress, 0x40000)
        self.cpu.mem_map(0x3000000, 0x2000000)
        self.symbols = symbols
        self.layout = layout
        self.tick = 1000
        self.calls = []
        self.callback = None
        self.word(0xb3fc30, stateAddress)
        self.cpu.reg_write(UC_ARM_REG_C1_C0_2, 0xf00000)
        self.cpu.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
        self.cpu.hook_add(UC_HOOK_CODE, self.onCode)
        if 'InitializeWorldSettings' in symbols and self.word(0x7bbcf0) == 0xe92d4010:
            self.execute(symbols['InitializeWorldSettings'])

    def word(self, address, value=None):
        if value is None:
            return struct.unpack('<I', self.cpu.mem_read(address, 4))[0]
        self.cpu.mem_write(address, struct.pack('<I', value))

    def setting(self, index, value):
        self.cpu.mem_write(stateAddress + self.layout['globalSettings'] + index * 2, struct.pack('<H', value))

    def returnValue(self, value):
        self.cpu.reg_write(UC_ARM_REG_R0, value)
        self.cpu.reg_write(UC_ARM_REG_PC, self.cpu.reg_read(UC_ARM_REG_LR))

    def onCode(self, cpu, address, size, data):
        if self.callback and self.callback(address):
            return
        if address == 0x4cf728:
            cpu.mem_write(cpu.reg_read(UC_ARM_REG_R0), struct.pack('<Q', self.tick))
            self.returnValue(0)
        elif address in (0x280250, 0x2e6d44, 0x67548c):
            count = 2 if address == 0x280250 else 9
            stack = cpu.reg_read(UC_ARM_REG_SP)
            assert stack % 4 == 0
            self.calls.append((address, [cpu.reg_read(register) for register in registers[:4]], cpu.reg_read(UC_ARM_REG_D0)))
            saved = struct.unpack(f'<{count}I', cpu.mem_read(stack, count * 4))
            for register, value in zip(registers[4:4 + count - 1], saved[:-1]):
                cpu.reg_write(register, value)
            cpu.reg_write(UC_ARM_REG_SP, stack + count * 4)
            cpu.reg_write(UC_ARM_REG_PC, saved[-1])

    def execute(self, address, args=(), floats=(0.25, 128.0)):
        self.calls = []
        for index, register in enumerate(registers):
            self.cpu.reg_write(register, 0xabc00000 + index)
        for register, value in zip(registers, args):
            self.cpu.reg_write(register, value & 0xffffffff)
        for index in range(16):
            self.cpu.reg_write(UC_ARM_REG_D0 + index, 0x3ff0000000000000 + index)
        self.cpu.reg_write(UC_ARM_REG_D0, int.from_bytes(struct.pack('<ff', *floats[:2]), 'little'))
        if len(floats) > 2:
            self.cpu.reg_write(UC_ARM_REG_D0 + 1, int.from_bytes(struct.pack('<f', floats[2]), 'little'))
        self.cpu.reg_write(UC_ARM_REG_SP, stackTop)
        self.cpu.reg_write(UC_ARM_REG_LR, stopAddress)
        self.cpu.emu_start(address, stopAddress, count=100000)
        assert self.cpu.reg_read(UC_ARM_REG_PC) == stopAddress
        assert self.cpu.reg_read(UC_ARM_REG_SP) == stackTop
        for index, register in enumerate(registers[4:12], 4):
            assert self.cpu.reg_read(register) == 0xabc00000 + index, (hex(address), index)
        for index in range(8, 16):
            assert self.cpu.reg_read(UC_ARM_REG_D0 + index) == 0x3ff0000000000000 + index
        return self.cpu.reg_read(UC_ARM_REG_R0)


def CheckDivision(harness):
    generator = random.Random(417)
    values = [0, 1, 2, 3, 10, 100, 0x7fffffff, 0x80000000, 0xfffffffe, 0xffffffff]
    pairs = [(a, b) for a in values for b in values if b] + [(generator.getrandbits(32), generator.randrange(1, 2**32)) for _ in range(512)]
    for numerator, denominator in pairs:
        quotient = harness.execute(harness.symbols['__aeabi_uidivmod'], (numerator, denominator))
        assert (quotient, harness.cpu.reg_read(UC_ARM_REG_R1)) == divmod(numerator, denominator)
        signedN = struct.unpack('<i', struct.pack('<I', numerator))[0]
        signedD = struct.unpack('<i', struct.pack('<I', denominator))[0]
        expected = abs(signedN) // abs(signedD) * (-1 if (signedN < 0) != (signedD < 0) else 1)
        assert harness.execute(harness.symbols['__aeabi_idiv'], (numerator, denominator)) == expected & 0xffffffff
    return len(pairs) * 2


def CheckVisuals(harness):
    for setting in range(3):
        harness.setting(1, setting)
        harness.execute(0x28024c, (hudAddress, 0x2010000, 0x2010100))
        assert len(harness.calls) == (0 if setting == 2 else 1)
        if harness.calls:
            address, args, floats = harness.calls[0]
            assert args[:3] == [hudAddress, 0x2010000, 0x2010100]
            assert struct.unpack('<ff', floats.to_bytes(8, 'little')) == (0.25, 64.0 if setting == 1 else 128.0)
    for setting in range(2):
        harness.setting(2, setting)
        harness.execute(0x2e6d40, (hudAddress, 1, 2, 3))
        assert len(harness.calls) == 1 - setting
        if harness.calls:
            assert harness.calls[0][1] == [hudAddress, 1, 2, 3]
            assert struct.unpack('<ff', harness.calls[0][2].to_bytes(8, 'little')) == (0.25, 128.0)

    harness.word(hudAddress + 492, 0)
    harness.word(hudAddress + 216, 1)
    for rate in range(4):
        harness.setting(3, rate)
        harness.tick = 1000
        harness.execute(0x675488, (hudAddress, 1, 2, 3))
        assert len(harness.calls) == 1
        harness.execute(0x675488, (hudAddress, 1, 2, 3))
        assert len(harness.calls) == int(rate == 0)
        harness.tick += ticksPerSecond // 4
        harness.execute(0x675488, (hudAddress, 1, 2, 3))
        assert len(harness.calls) == int(rate <= 1)
        harness.tick += ticksPerSecond
        harness.execute(0x675488, (hudAddress, 1, 2, 3))
        assert len(harness.calls) == int(rate != 3)
        harness.word(hudAddress + 216, 0)
        harness.execute(0x675488, (hudAddress, 1, 2, 3))
        assert len(harness.calls) == 1
        harness.word(hudAddress + 216, 1)
    return 25


def ReadTerrainTextures(romfsPath):
    textures = []
    with romfsPath.open('rb') as stream:
        assert stream.read(4) == b'IVFC'
        stream.seek(4096)
        header = struct.unpack('<10I', stream.read(40))
        assert header[0] == 40
        stream.seek(4096 + header[7])
        metadata = stream.read(header[8])
        offset = 0
        while offset < len(metadata):
            parent, sibling, dataOffset, dataSize, nextHash, nameSize = struct.unpack_from('<IIQQII', metadata, offset)
            name = metadata[offset + 32:offset + 32 + nameSize].decode('utf-16le')
            if name.startswith('atlas.terrain.meta_') and name.endswith('.3dst'):
                stream.seek(4096 + header[9] + dataOffset)
                data = stream.read(dataSize)
                assert len(data) == dataSize
                if struct.unpack_from('<I', data, 28)[0] == 3:
                    textures.append((name, data))
            offset += (32 + nameSize + 3) & ~3
        assert offset == len(metadata)
    return textures


def CheckTextureLoader(harness, romfsPath):
    textures = ReadTerrainTextures(romfsPath)
    assert len(textures) == 12, len(textures)
    for enabled in (0, 1):
        harness.word(stateAddress + harness.layout['activeLowMemoryTextures'], enabled)
        for name, data in textures:
            header = struct.unpack('<8I', data[:32])
            baseBytes = header[3] * header[4] * (4 if header[2] == 0 else 2)
            skipped = baseBytes if enabled else 0
            expected = data[32 + skipped:]
            cpu = harness.cpu
            cpu.mem_write(stackTop + 8, data[:32])
            cpu.mem_write(stackTop + 40, name.encode() + b'\0')
            cpu.mem_write(stackTop + 352, struct.pack('<6I', 0, 0x1234, 32, 0, len(data), 0))
            events = []

            def Callback(address):
                if address == 0x4f95b4:
                    assert [cpu.reg_read(register) for register in registers[:4]] == [65536, 257, 0, len(expected)]
                    events.append('allocate')
                    harness.returnValue(0x3000000)
                elif address == 0x11b060:
                    position = cpu.reg_read(UC_ARM_REG_R2) | cpu.reg_read(UC_ARM_REG_R3) << 32
                    stack = cpu.reg_read(UC_ARM_REG_SP)
                    destination, count = struct.unpack('<II', cpu.mem_read(stack, 8))
                    assert position == 32 + skipped and count == len(expected)
                    cpu.mem_write(destination, data[position:position + count])
                    harness.word(cpu.reg_read(UC_ARM_REG_R0), count)
                    events.append('read')
                    harness.returnValue(0)
                elif address == 0x4fbd38:
                    descriptor = cpu.reg_read(UC_ARM_REG_R1)
                    assert [harness.word(descriptor + offset) for offset in (24, 28, 32)] == [header[5] >> enabled, header[6] >> enabled, 3 - enabled]
                    assert bytes(cpu.mem_read(0x3000000, len(expected))) == expected
                    events.append('initialize')
                    cpu.reg_write(UC_ARM_REG_PC, stopAddress)
                else:
                    return False
                return True

            harness.callback = Callback
            cpu.reg_write(UC_ARM_REG_R0, len(data))
            cpu.reg_write(UC_ARM_REG_R7, 0x2010000)
            cpu.reg_write(UC_ARM_REG_SP, stackTop)
            cpu.emu_start(0x249944, stopAddress, count=100000)
            assert cpu.reg_read(UC_ARM_REG_PC) == stopAddress
            assert cpu.reg_read(UC_ARM_REG_SP) == stackTop
            assert events == ['allocate', 'read', 'initialize']
    harness.callback = None
    assert harness.word(stateAddress + harness.layout['textureLoads']) == 12
    return len(textures) * 2


def CheckMeshes(stock, code, symbols):
    faces = 0
    for grass in (False, True):
        for mask in [None] + [upper | lower for upper in (0, 0xffffffc0) for lower in range(64)]:
            before = CheckHiddenCase(stock, mask, grass, symbols['SkipHiddenBlockFaces'])
            after = CheckHiddenCase(code, mask, grass, symbols['SkipHiddenBlockFaces'])
            for key in ('result', 'calls', 'memory', 'liveStack'):
                assert before[key] == after[key], (mask, grass, key)
            assert after['occlusionCalls'] == ([] if mask is not None and mask & 63 == 63 else before['occlusionCalls'])
            faces += 1
    return {'hiddenFaceCases': faces, 'biome': CheckBiome(stock, code)}


def CheckPrompts(harness):
    helper, client, font, label, text = 0x2011000, 0x2012000, 0x2013000, 0x2014000, 0x2015000
    harness.execute(harness.symbols['_ZN15ControlSettings10InitializeERNS_7TConfigE'], (stateAddress + harness.layout['config'],))
    harness.word(stateAddress + harness.layout['screen'], 0)
    harness.word(stateAddress + harness.layout['hud'], hudAddress)
    harness.word(helper + 56, client)
    harness.word(client + 92, font)
    harness.word(hudAddress + 4, client)
    harness.word(hudAddress + 184, 0)
    harness.word(label, text)
    harness.cpu.mem_write(text, b'Action\0')
    events = []
    currentScreen = hudAddress

    def ReadText(pointer):
        return bytes(harness.cpu.mem_read(harness.word(pointer), 96)).split(b'\0')[0].decode()

    scaledText = next((value for name, value in harness.symbols.items() if name.startswith('_ZN12DebugOverlay14DrawScaledText')), None)

    def Callback(address):
        cpu = harness.cpu
        args = [cpu.reg_read(register) for register in registers[:4]]
        if address == 0x12d544:
            assert args[0] == client
            harness.returnValue(currentScreen)
        elif address == harness.symbols['ControlsOriginalPrompt']:
            assert args[:3] == [helper, 10, 20]
            assert harness.word(cpu.reg_read(UC_ARM_REG_SP)) == label
            events.append(('glyph', args[3]))
            harness.returnValue(0)
        elif address == 0x71ad28:
            assert args[0] == font and args[2] == 0
            assert cpu.reg_read(UC_ARM_REG_D0) & 0xffffffff == 0x3f800000
            harness.returnValue(len(ReadText(args[1])) * 6)
        elif address in (0x55bb84, scaledText):
            assert args[0] == font
            if address != scaledText:
                assert args[3] == 1 and harness.word(cpu.reg_read(UC_ARM_REG_SP)) == 0
            x, y = struct.unpack('<ff', cpu.reg_read(UC_ARM_REG_D0).to_bytes(8, 'little'))
            scale = struct.unpack('<f', cpu.reg_read(UC_ARM_REG_D0 + 1).to_bytes(8, 'little')[:4])[0]
            events.append(('text', ReadText(args[1]), x, y, scale))
            harness.returnValue(0)
        elif address == 0x11d954:
            assert args[1:3] == [label, 0]
            harness.word(args[0], text)
            harness.returnValue(0)
        elif address == harness.symbols['ControlsStringRelease']:
            assert harness.word(args[0]) == text
            events.append(('release',))
            harness.returnValue(0)
        else:
            return False
        return True

    harness.callback = Callback
    cases = 0
    buttons = [1, 2, 0x400, 0x800, 0x40, 0x80, 0x20, 0x10, 0x200, 0x100, 0x4000, 0x8000]
    for gameplay in (False, True):
        currentScreen = hudAddress if gameplay else client
        action = 0 if gameplay else 15
        for binding in buttons + [0, 4, 8, 0x600]:
            harness.cpu.mem_write(stateAddress + harness.layout['profiles'] + action * 2, struct.pack('<H', binding))
            harness.word(stackTop, label)
            events.clear()
            harness.execute(0x471074, (helper, 10, 20, 0))
            if binding in buttons:
                assert events == [('glyph', buttons.index(binding))], events
            else:
                expected = {0: '-', 4: 'Select', 8: 'Start', 0x600: 'X+L'}[binding]
                assert len(events) == 3 and events[0][:4] == ('text', expected, 10.0, 23.0), events
                assert abs(events[0][4] - min(1, 16 / (6 * len(expected)))) < 0.00001
                assert events[1] == ('text', 'Action', 28.0, 23.0, 1.0) and events[2] == ('release',)
            cases += 1
    harness.callback = None
    return cases


def CheckOverlay(harness, symbols, background=False):
    advanced = 'NativeRenderOptions' in symbols
    scaledText = next((value for name, value in symbols.items() if name.startswith('_ZN12DebugOverlay14DrawScaledText')), None)
    client, font, device = 0x2012000, 0x2013000, 0x2016000
    harness.word(stateAddress + harness.layout['hud'], hudAddress)
    harness.word(client + 92, font)
    harness.word(hudAddress + 184, 0)
    harness.word(0xa35884, device)
    harness.cpu.mem_write(device + 388, b'\x02\x00\x00')
    for address in (0xa30d18, 0xa30d1c, 0xa30d24):
        harness.word(address, device)
    currentScreen = hudAddress
    events = []
    measuredWidths = []

    def Callback(address):
        cpu = harness.cpu
        args = [cpu.reg_read(register) for register in registers[:4]]
        if address == 0x12d544:
            assert args[0] == client
            harness.returnValue(currentScreen)
        elif address == 0x1399a8:
            destinations = args + list(struct.unpack('<II', cpu.mem_read(cpu.reg_read(UC_ARM_REG_SP), 8)))
            for index, destination in enumerate(destinations):
                harness.word(destination, (1 if index % 2 == 0 else 2) * 1048576)
            events.append('heaps')
            harness.returnValue(0)
        elif advanced and address in (0x123170, 0x1231b0):
            assert args[0] == device + 88
            harness.returnValue(0)
        elif address == 0x71ad28 and background:
            assert args[0] == font and args[2] == 0
            assert struct.unpack('<f', (cpu.reg_read(UC_ARM_REG_D0) & 0xffffffff).to_bytes(4, 'little'))[0] == 1.0
            text = bytes(cpu.mem_read(harness.word(args[1]), 64)).split(b'\0')[0]
            measuredWidths.append(len(text) * 6)
            harness.returnValue(measuredWidths[-1])
        elif address == 0x6cff68 and background:
            bottom, color = struct.unpack('<II', cpu.mem_read(cpu.reg_read(UC_ARM_REG_SP), 8))
            assert args[:3] == [0, 1, 1] and args[3] == max(measuredWidths) + 8
            assert bottom == len(measuredWidths) * 14 + 3
            rgba = struct.unpack('<4f', cpu.mem_read(color, 16))
            assert all(abs(channel - 20 / 255) < 0.000001 for channel in rgba[:3]) and rgba[3] == 0.75
            assert 'draw' not in events
            events.append('background')
            harness.returnValue(0)
        elif address in (0x55b7bc, 0x55bb84, scaledText):
            assert args[0] == font
            if address != scaledText:
                assert args[3] == (1 if address == 0x55bb84 else 0)
            assert bytes(cpu.mem_read(harness.word(args[1]), 64)).split(b'\0')[0]
            events.append('draw')
            harness.returnValue(0)
        elif address == 0x4f9044:
            events.append('native')
            harness.returnValue(0x12345678)
        elif address == 0x4cf728:
            events.append('tick')
            cpu.mem_write(args[0], struct.pack('<Q', harness.tick))
            harness.returnValue(0)
        else:
            return False
        return True

    harness.callback = Callback
    cases = 0
    for mode in range(3):
        harness.setting(0, mode)
        for gameplay in (False, True):
            currentScreen = hudAddress if gameplay else client
            harness.cpu.mem_write(0xac3818, bytes(104))
            harness.cpu.mem_write(0xac3818 + 80, struct.pack('<4I', 300, 333, 667, 1))
            harness.word(0xac3818 + 96, 2)
            events.clear()
            measuredWidths.clear()
            harness.execute(symbols['DrawDebugOverlay'], (0, client))
            panel = ['background'] if background else []
            expected = [] if not gameplay or mode == 0 else (panel + ['draw'] if mode == 1 else ['heaps'] + panel + ['draw'] * (6 if advanced else 5))
            assert events == expected, (mode, gameplay, events)
            cases += 1
        for display in (0x400, 0x401):
            events.clear()
            assert harness.execute(symbols['OnDisplayVblankWithOverlayStats'], (display,)) == 0x12345678
            assert events == (['native', 'tick'] if mode and display == 0x400 else ['native'])
            cases += 1
    harness.callback = None
    return cases


def Main():
    parser = argparse.ArgumentParser(description='Execute native settings hooks and mesh fast paths against private fixtures')
    parser.add_argument('--reference-code', type=Path, required=True)
    parser.add_argument('--patched-code', type=Path, required=True)
    parser.add_argument('--symbols', type=Path, required=True)
    parser.add_argument('--world-baseline', type=Path)
    parser.add_argument('--overlay-symbols', type=Path, required=True)
    parser.add_argument('--overlay-background', action='store_true')
    parser.add_argument('--texture-romfs', type=Path, required=True)
    parser.add_argument('--layeredfs-exheader', type=Path, help='Check layout and execute all settings checks after Luma path injection')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    stock, code = args.reference_code.read_bytes(), args.patched_code.read_bytes()
    codeHash = hashlib.sha256(code).hexdigest()
    assert hashlib.sha256(stock).hexdigest() == '902ccd5a06d59797c52976b2905dc56a5ca0a868021432fcc7eb127ba3320d65'
    layeredFs = None
    if args.layeredfs_exheader:
        exheader = args.layeredfs_exheader.read_bytes()
        layeredFs = CheckLayeredFs(stock, code, exheader)
        code = InjectPath(code, exheader)
    symbols = json.loads(args.symbols.read_text())
    layout = json.loads(args.symbols.with_name('state-layout.json').read_text())
    harness = CSettingsHarness(code, symbols, layout)
    worldChecks = CheckWorldSettings(harness, CSettingsHarness(args.world_baseline.read_bytes(), {}, layout)) if args.world_baseline else None
    report = {'codeSha256': codeHash, 'executedCodeSha256': hashlib.sha256(code).hexdigest(),
        'layeredFs': layeredFs, 'divisionCases': CheckDivision(harness),
        'visualGateCases': CheckVisuals(harness), 'terrainLoaderCases': CheckTextureLoader(harness, args.texture_romfs),
        'promptCases': CheckPrompts(harness),
        'overlayCases': CheckOverlay(harness, json.loads(args.overlay_symbols.read_text()), args.overlay_background),
        'worldSettings': worldChecks,
        'meshes': CheckMeshes(stock, code, symbols),
        'builderMemory': CheckBuilderMemory(code),
        'nativeMenus': CheckNativeMenus(harness),
        'scrollLifetime': CheckScrollingLifetime(CSettingsHarness(code, symbols, layout)) if 'NativeScrollChanged' in symbols else None,
        'scrollIntegrationCases': CheckScrollIntegration(CSettingsHarness(code, symbols, layout)) if 'NativeScrollChanged' in symbols else None,
        'lightReads': CheckLightReads(CSettingsHarness(stock, symbols, layout), harness) if 'ReadPackedLight' in symbols else None,
        'advancedOverlay': CheckAdvancedOverlay(harness),
        'scope': 'Actual ARM payload and native texture read/descriptor/mesh control flow, with controlled GPU, filesystem and world dependencies. Not a hardware FPS or long-session stability test.'}
    args.output.write_text(json.dumps(report, indent=4) + '\n')
    print(json.dumps(report, indent=4))


if __name__ == '__main__':
    Main()
