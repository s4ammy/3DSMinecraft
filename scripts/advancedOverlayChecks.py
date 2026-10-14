import struct

from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_SP, UC_ARM_REG_D0


def CheckAdvancedOverlay(harness):
    if 'diagnostics' not in harness.layout:
        return None
    state, client, hud, font, arena, node = 0x2000000, 0x2010000, 0x2012000, 0x2013000, 0x2014000, 0x2015000
    def Symbol(prefix):
        return next(value for name, value in harness.symbols.items() if name.startswith(prefix))
    largest = Symbol('_ZN12DebugOverlay16ReadLargestBlock')
    graph = Symbol('_ZN12DebugOverlay14DrawFrameGraph')
    scaledText = Symbol('_ZN12DebugOverlay14DrawScaledText')
    events, vertices, panels, texts = [], [], [], []
    expectedScale = 1.0
    expectedOpacity = 0.75
    matrixStack, matrix = 0x2018000, 0x2019000
    matrixReference = 0
    def ScaledTextCallback(address):
        nonlocal matrixReference
        cpu = harness.cpu
        args = [cpu.reg_read(register) for register in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)]
        floats = struct.unpack('<ff', cpu.reg_read(UC_ARM_REG_D0).to_bytes(8, 'little')) + (struct.unpack('<f', (cpu.reg_read(UC_ARM_REG_D0 + 1) & 0xffffffff).to_bytes(4, 'little'))[0],)
        if address == 0x19b9dc:
            assert args[1] == matrixStack
            matrixReference = args[0]
            cpu.mem_write(matrixReference, struct.pack('<II', matrixStack, matrix))
            events.append('push')
        elif address == 0x19b7a8:
            assert args[0] == matrixReference
            events.append('matrix')
            harness.returnValue(matrix)
            return True
        elif address == 0x5fc550:
            assert args[0] == matrix and floats == (24.0, 32.0, 0.0)
            events.append('translate')
        elif address == 0x5fc1f8:
            assert args[0] == matrix and floats == (expectedScale, expectedScale, 1.0)
            events.append('scale')
        elif address == 0x55bb84:
            assert args == [font, node, node + 16, 1]
            assert harness.word(cpu.reg_read(UC_ARM_REG_SP)) == 0 and floats == (0.0, 0.0, 1.0)
            events.append('draw')
        elif address == 0x19b5c4:
            assert args[0] == matrixReference
            events.append('release')
        else:
            return False
        harness.returnValue(0)
        return True

    harness.word(0xa358b8, matrixStack)
    harness.word(font + 708, 0x3f800000)
    harness.callback = ScaledTextCallback
    for expectedScale in (0.5, 0.75, 1.0, 1.25):
        events.clear()
        harness.execute(scaledText, (font, node, node + 16), (24.0, 32.0, expectedScale))
        assert events == ['push', 'matrix', 'translate', 'scale', 'draw', 'release']
        assert harness.word(font + 708) == 0x3f800000

    def Callback(address):
        cpu = harness.cpu
        args = [cpu.reg_read(register) for register in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)]
        if address in (0x123170, 0x1231b0):
            assert args[0] == arena + 88
            events.append('lock' if address == 0x123170 else 'unlock')
        elif address == 0x12d544:
            harness.returnValue(hud)
            return True
        elif address == 0x1399a8:
            pointers = args + list(struct.unpack('<II', cpu.mem_read(cpu.reg_read(UC_ARM_REG_SP), 8)))
            for index, pointer in enumerate(pointers):
                harness.word(pointer, (1 + index % 2) * 1048576)
        elif address == 0x71ad28:
            scale = struct.unpack('<f', (cpu.reg_read(UC_ARM_REG_D0) & 0xffffffff).to_bytes(4, 'little'))[0]
            assert abs(scale - expectedScale) < 0.00001
            text = bytes(cpu.mem_read(harness.word(args[1]), 64)).split(b'\0')[0]
            harness.returnValue(round(len(text) * 6 * scale))
            return True
        elif address == 0x6cff68:
            bottom, color = struct.unpack('<II', cpu.mem_read(cpu.reg_read(UC_ARM_REG_SP), 8))
            panels.append((args[1], args[2], args[3], bottom))
            rgba = struct.unpack('<4f', cpu.mem_read(color, 16))
            assert all(abs(value - 20 / 255) < 0.00001 for value in rgba[:3])
            assert abs(rgba[3] - expectedOpacity) < 0.00001
        elif address == scaledText:
            x, y = struct.unpack('<ff', cpu.reg_read(UC_ARM_REG_D0).to_bytes(8, 'little'))
            scale = struct.unpack('<f', (cpu.reg_read(UC_ARM_REG_D0 + 1) & 0xffffffff).to_bytes(4, 'little'))[0]
            assert 0 < scale <= expectedScale + 0.00001
            texts.append((x, y, scale))
        elif address == harness.symbols['TouchBegin']:
            assert args[1] == 132
        elif address == harness.symbols['TouchVertex']:
            vertices.append(struct.unpack('<ff', cpu.reg_read(UC_ARM_REG_D0).to_bytes(8, 'little')))
        elif address == harness.symbols['TouchSetGuiColor']:
            cpu.mem_write(args[0], bytes(cpu.mem_read(args[1], 16)))
        elif address in (harness.symbols['TouchColor'], harness.symbols['TouchDraw']):
            pass
        elif address == 0x4f9044:
            harness.returnValue(0x12345678)
            return True
        elif address == 0x4cf728:
            cpu.mem_write(args[0], struct.pack('<Q', harness.tick))
        else:
            return False
        harness.returnValue(0)
        return True
    harness.callback = Callback
    for sizes in ([], [16], [16, 524288, 32, 1048576, 128]):
        harness.word(arena + 60, node if sizes else 0)
        for index, size in enumerate(sizes):
            harness.word(node + index * 16 + 4, size)
            harness.word(node + index * 16 + 12, node + (index + 1) * 16 if index + 1 < len(sizes) else 0)
        events.clear()
        assert harness.execute(largest, (arena,)) == max(sizes, default=0)
        assert events == ['lock', 'unlock']
    harness.word(state + harness.layout['hud'], hud)
    harness.word(hud + 184, 0)
    harness.word(client + 92, font)
    for slot in (0xa30d18, 0xa30d1c, 0xa30d24):
        harness.word(slot, arena)
    harness.cpu.mem_write(state + harness.layout['diagnostics'], struct.pack('<32I', *range(0, 128, 4)) + struct.pack('<4I', 0, 0, 0, 0))
    originalColor = struct.pack('<4f', 0.2, 0.3, 0.4, 0.5)
    harness.cpu.mem_write(harness.symbols['touchGuiColorState'], originalColor)
    cases = 0
    for mode in (1, 2):
        harness.setting(0, mode)
        for corner in range(4):
            harness.setting(6, corner)
            for scale in (50, 100, 125):
                harness.setting(7, scale)
                expectedScale = scale * 0.01
                for opacity in (0, 75, 100):
                    harness.setting(8, opacity)
                    expectedOpacity = opacity * 0.01
                    harness.setting(11, 1)
                    panels.clear()
                    vertices.clear()
                    texts.clear()
                    harness.cpu.mem_write(0xac3818, bytes(104))
                    harness.cpu.mem_write(0xac3818 + 80, struct.pack('<5I', 300, 333, 667, 1, 2))
                    harness.execute(harness.symbols['DrawDebugOverlay'], (0, client))
                    assert len(panels) == 1 and len(texts) == (1 if mode == 1 else 6)
                    left, top, right, bottom = panels[0]
                    assert 0 <= left < right <= 319 and 0 <= top < bottom <= 239
                    assert (right == 319 if corner & 1 else left == 1)
                    assert (bottom == 238 if corner & 2 else top == 1)
                    assert len(vertices) == 132
                    assert all(left <= x <= right and top <= y <= bottom for x, y in vertices)
                    for index in range(0, len(vertices), 4):
                        first, second, third, fourth = vertices[index:index + 4]
                        assert first[1] >= third[1] and first[0] == fourth[0] and second[0] == third[0]
                    assert bytes(harness.cpu.mem_read(harness.symbols['touchGuiColorState'], 16)) == originalColor
                    cases += 1
    device = 0x2017000
    harness.word(0xa35884, device)
    harness.cpu.mem_write(device + 388, b'\x02\x00\x00')
    harness.cpu.mem_write(0xac3818, bytes(104))
    harness.cpu.mem_write(state + harness.layout['diagnostics'], bytes(144))
    vblank = harness.symbols['OnDisplayVblankWithOverlayStats']
    harness.tick = 1000
    harness.execute(vblank, (0x400,))
    intervals = [17, 33, 50, 100, 250] * 10
    for interval in intervals:
        harness.tick += round(interval * 268111856 / 1000)
        assert harness.execute(vblank, (0x400,)) == 0x12345678
        harness.execute(vblank, (0x401,))
    cursor = harness.word(state + harness.layout['cursor'])
    assert cursor == len(intervals)
    history = struct.unpack('<32I', harness.cpu.mem_read(state + harness.layout['intervals'], 128))
    assert [history[(cursor + index) & 31] for index in range(32)] == intervals[-32:]
    harness.setting(11, 0)
    harness.tick += 10000000
    harness.execute(vblank, (0x400,))
    assert harness.word(state + harness.layout['cursor']) == cursor
    harness.callback = None
    return {'layoutCases': cases, 'scaledTextTransformCases': 4, 'lockedLargestBlockCases': 3, 'graphVerticesPerDraw': 132, 'historyFrames': len(intervals),
        'scope': 'Actual ARM layout, fitting, corners, scale, opacity, graph bounds and tint restoration with controlled renderer. Largest-free-payload walk under the native arena mutex ABI. Not GPU timing.'}
