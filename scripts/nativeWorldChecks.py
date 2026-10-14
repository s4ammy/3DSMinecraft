import struct

from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8,
    UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12, UC_ARM_REG_SP,
    UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_CPSR, UC_ARM_REG_S1)


def CheckWorldSettings(harness, baseline):
    if 'InitializeWorldSettings' not in harness.symbols:
        return None
    cpu, symbols = harness.cpu, harness.symbols
    registers = (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
        UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8,
        UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12)
    active, table, state, stackTop = 0xb16348, 0xb161c0, 0x2000000, 0x203e000
    initialize = symbols['_ZN15ControlSettings10InitializeERNS_7TConfigE']
    apply = next(value for name, value in symbols.items() if name.startswith('_ZN12RuntimeWorld5Apply'))
    pending = next(value for name, value in symbols.items() if name.startswith('_ZN12RuntimeWorld12NeedsRestart'))
    config = state + harness.layout['config']
    harness.execute(initialize, (config,))
    bootRegisters = [0xabc00000 + index for index in range(13)]
    for register, value in zip(registers, bootRegisters):
        cpu.reg_write(register, value)
    cpu.reg_write(UC_ARM_REG_R0, 0xb161a8)
    cpu.reg_write(UC_ARM_REG_SP, stackTop)
    cpu.reg_write(UC_ARM_REG_LR, 0x12345678)
    cpu.mem_write(0xb161a8, b'\xa5' * 512)
    cpu.emu_start(0x7bbcf0, 0x7bc084, count=10000)
    assert cpu.reg_read(UC_ARM_REG_PC) == 0x7bc084
    assert cpu.reg_read(UC_ARM_REG_SP) == stackTop
    assert cpu.reg_read(UC_ARM_REG_LR) == 0x12345678
    assert [cpu.reg_read(register) for register in registers[:4]] == [0xb16368, 1, 4, 4]
    assert [cpu.reg_read(register) for register in registers[4:12]] == bootRegisters[4:12]
    assert bytes(cpu.mem_read(0xb161a8, 24)) == b'\xa5' * 24
    assert bytes(cpu.mem_read(active + 32, 40)) == b'\xa5' * 40
    baseline.cpu.reg_write(UC_ARM_REG_R0, 0xb161a8)
    baseline.cpu.emu_start(0x7bbcf0, 0x7bc084, count=1000)
    assert bytes(cpu.mem_read(table, 200)) == bytes(baseline.cpu.mem_read(table, 200))

    def Fragment(address, inputs, outputs, flags=None, floatBits=None, stackDelta=0):
        before = [0xabc00000 + index for index in range(13)]
        for index, value in inputs.items():
            before[index] = value
        for register, value in zip(registers, before):
            cpu.reg_write(register, value)
        cpu.reg_write(UC_ARM_REG_SP, stackTop)
        cpu.reg_write(UC_ARM_REG_LR, 0x12345678)
        status = cpu.reg_read(UC_ARM_REG_CPSR)
        cpu.reg_write(UC_ARM_REG_CPSR, (status & 0x0fffffff) | 0xb0000000)
        cpu.emu_start(address, address + 4, count=1000)
        assert cpu.reg_read(UC_ARM_REG_PC) == address + 4, hex(address)
        assert cpu.reg_read(UC_ARM_REG_SP) == stackTop + stackDelta, hex(address)
        assert cpu.reg_read(UC_ARM_REG_LR) == 0x12345678, hex(address)
        for index, register in enumerate(registers):
            expected = outputs.get(index, before[index])
            assert cpu.reg_read(register) == expected, (hex(address), index, hex(cpu.reg_read(register)), hex(expected))
        if flags is not None:
            assert cpu.reg_read(UC_ARM_REG_CPSR) >> 28 == flags, (hex(address), 'flags')
        if floatBits is not None:
            assert cpu.reg_read(UC_ARM_REG_S1) == floatBits

    cases, fragments = 0, 0
    for radius in (1, 2, 3):
        count = (radius * 2 + 1) ** 2
        stages = count * 2
        threshold = stages - stages // 10
        for render in (1, 2, 3):
            for mapRadius in (8, 16, 24, 32):
                for builders in (1, 2, 3, 4):
                    harness.execute(symbols['InitializeWorldSettings'])
                    for index, value in enumerate((render, radius, mapRadius, builders)):
                        harness.setting(12 + index, value)
                    before = bytes(cpu.mem_read(table + count * 8, active - table - count * 8))
                    harness.execute(apply, (config,))
                    values = struct.unpack('<6IfI', cpu.mem_read(active, 32))
                    assert values == (render, radius, mapRadius, builders, count * 8, threshold, float(stages), 1)
                    offsets = [struct.unpack('<ii', cpu.mem_read(table + i * 8, 8)) for i in range(count)]
                    assert set(offsets) == {(x, z) for x in range(-radius, radius + 1) for z in range(-radius, radius + 1)}
                    assert len(set(offsets)) == count
                    assert bytes(cpu.mem_read(table + count * 8, active - table - count * 8)) == before
                    assert harness.execute(pending, (config,)) == 0
                    for address, destination, source in ((0x1e759c, 9, 5), (0x3c74bc, 0, 7),
                            (0x5cde28, 7, 6), (0x607480, 8, 4)):
                        Fragment(address, {source: table}, {destination: table + count * 8}, 0xb)
                    Fragment(0x1999ac, {7: 305}, {7: 305 + stages}, 0xb)
                    Fragment(0x3266cc, {}, {2: radius * 16}, 0xb)
                    Fragment(0x72c498, {}, {0: mapRadius}, 0xb)
                    Fragment(0x199bac, {}, {}, 0xb, int.from_bytes(struct.pack('<f', stages), 'little'))
                    requestedRadius = max(min(render, radius), radius - 1)
                    Fragment(0x1971d8, {0: state, 1: 1}, {1: requestedRadius, 12: radius - 1}, stackDelta=-12)
                    Fragment(0x19a060, {0: state}, {0: requestedRadius}, 0xb)
                    for requested in (0, 1, 4, 10, 0x7fffffff, 0xffffffff):
                        signed = struct.unpack('<i', struct.pack('<I', requested))[0]
                        limited = min(signed, builders) & 0xffffffff
                        Fragment(0x1d87a0, {0: requested, 4: state}, {0: limited})
                        assert harness.word(state + 20) == limited
                    for completed in (0, threshold, threshold + 1, stages):
                        for address, source in ((0x199bb4, 0), (0x1e8d7c, 6), (0x1e8d8c, 0)):
                            flags = 0x8 if completed < threshold else 0x6 if completed == threshold else 0x2
                            Fragment(address, {source: completed}, {}, flags)
                    snapshot = bytes(cpu.mem_read(0xb161a8, 480))
                    harness.setting(12, render % 3 + 1)
                    assert harness.execute(pending, (config,)) == 1
                    harness.execute(apply, (config,))
                    assert bytes(cpu.mem_read(0xb161a8, 480)) == snapshot
                    cases += 1
                    fragments += 28
    loadingCases = 0
    renderRequestCases = 0
    initialRenderCases = 0
    renderBoundsCases = 0
    shuffleCases = 0
    player, sourceOwner, source, vtable, chunk = 0x2010000, 0x2020000, 0x2021000, 0x2022000, 0x2025000
    visited, randomBounds = [], []
    chunkStage = 6

    def Callback(address):
        if address == 0x2030000:
            assert cpu.reg_read(UC_ARM_REG_R0) == source
            visited.append(struct.unpack('<ii', cpu.mem_read(cpu.reg_read(UC_ARM_REG_R1), 8)))
            harness.returnValue(chunk if chunkStage >= 0 else 0)
        elif address in (0x5f741c, 0x720908, 0x5f5fe4):
            harness.returnValue(0)
        elif address == 0x695e64:
            harness.returnValue(1)
        elif address == 0x61e680:
            bound = cpu.reg_read(UC_ARM_REG_R1)
            randomBounds.append(bound)
            harness.returnValue((bound * 7 + 3) % bound)
        else:
            return False
        return True

    harness.callback = Callback
    cpu.mem_write(player, bytes(8000))
    cpu.mem_write(player + 4628, b'\x01')
    cpu.mem_write(player + 6312, b'\x01')
    harness.word(player + 4588, sourceOwner)
    harness.word(sourceOwner + 16, source)
    harness.word(source, vtable)
    harness.word(vtable + 8, 0x2030000)
    for radius in (1, 2, 3):
        for render in (1, 2, 3):
            requestedRadius = max(min(render, radius), radius - 1)
            harness.execute(symbols['InitializeWorldSettings'])
            harness.setting(12, render)
            harness.setting(13, radius)
            harness.execute(apply, (config,))
            harness.execute(0x1971d8, (player, 99))
            assert harness.word(player + 7100) == requestedRadius
            assert harness.word(player + 6516) == requestedRadius
            assert harness.execute(symbols['RuntimeRenderBlocks'], (player,)) == min(render, radius) * 16
            renderRequestCases += 1
            harness.word(player + 6516, 0)
            harness.word(player + 7100, 0)
            cpu.reg_write(UC_ARM_REG_R0, state)
            cpu.reg_write(UC_ARM_REG_R4, player)
            cpu.reg_write(UC_ARM_REG_R6, player + 4096)
            cpu.reg_write(UC_ARM_REG_SP, stackTop)
            cpu.emu_start(0x19a060, 0x19a0c4, count=10000)
            assert cpu.reg_read(UC_ARM_REG_PC) == 0x19a0c4
            assert harness.word(player + 7100) == requestedRadius
            assert harness.word(player + 6516) == requestedRadius
            assert harness.execute(symbols['RuntimeRenderBlocks'], (player,)) == min(render, radius) * 16
            initialRenderCases += 1
            for acceptedRadius in range(5):
                harness.word(player + 6516, acceptedRadius)
                cpu.reg_write(UC_ARM_REG_R0, player)
                cpu.reg_write(UC_ARM_REG_SP, stackTop)
                cpu.emu_start(0x31d640, 0x31d648, count=1000)
                assert cpu.reg_read(UC_ARM_REG_PC) == 0x31d648
                assert cpu.reg_read(UC_ARM_REG_SP) == stackTop
                assert cpu.reg_read(UC_ARM_REG_R7) == min(render, requestedRadius, acceptedRadius) * 16
                renderBoundsCases += 1
        harness.execute(symbols['InitializeWorldSettings'])
        harness.setting(13, radius)
        harness.execute(apply, (config,))
        count = (radius * 2 + 1) ** 2
        expectedOffsets = {(x, z) for x in range(-radius, radius + 1) for z in range(-radius, radius + 1)}
        for chunkStage in (-1, 0, 2, 6):
            visited.clear()
            harness.word(chunk + 56, max(chunkStage, 0))
            result = harness.execute(0x607444, (player, 0))
            assert result == count * (2 if chunkStage == 6 else 1 if chunkStage == 2 else 0)
            assert len(visited) == count and set(visited) == expectedOffsets
            loadingCases += 1
        frozen = bytes(cpu.mem_read(active, 32))
        randomBounds.clear()
        cpu.reg_write(UC_ARM_REG_R4, player)
        cpu.reg_write(UC_ARM_REG_R8, 1)
        cpu.reg_write(UC_ARM_REG_SP, stackTop)
        cpu.emu_start(0x5cde20, 0x5cde70, count=10000)
        assert cpu.reg_read(UC_ARM_REG_PC) == 0x5cde70
        assert randomBounds == list(range(2, count + 1))
        assert {struct.unpack('<ii', cpu.mem_read(table + i * 8, 8)) for i in range(count)} == expectedOffsets
        assert bytes(cpu.mem_read(active, 32)) == frozen
        shuffleCases += 1
    harness.callback = None
    harness.execute(symbols['InitializeWorldSettings'])
    harness.execute(initialize, (config,))
    return {'configurations': cases, 'hookFragments': fragments, 'defaultTableMatchesPrevious': True,
        'restartFreeze': True, 'boundsAndAbi': True, 'nativeLoadingLoops': loadingCases,
        'nativeShuffleLoops': shuffleCases,
        'nativeLocalRenderRequests': renderRequestCases,
        'nativeInitialRenderRequests': initialRenderCases,
        'nativeRenderAcceptanceBounds': renderBoundsCases,
        'loadingMarginIndependentOfRender': True,
        'scope': 'Actual ARM table initialization, bounded writes, register/flag hook contracts, all ranges and restart freeze. Not full-world or hardware validation.'}
