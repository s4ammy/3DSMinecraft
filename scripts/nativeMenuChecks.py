import struct

from unicorn.arm_const import UC_ARM_REG_D0, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2


def CheckNativeMenus(harness):
    if 'nativeMenu' not in harness.layout:
        return None
    state = 0x2000000
    menu, screen, options, widget = 0x2020000, 0x2021000, 0x2022000, 0x2023000
    symbols = harness.symbols

    def Symbol(prefix):
        return next(value for name, value in symbols.items() if name.startswith(prefix))

    configure = Symbol('_ZN11NativeMenus12ConfigureRow')
    getValue = Symbol('_ZN11NativeMenus5Value')
    setValue = Symbol('_ZN11NativeMenus8SetValue')
    poll = Symbol('_ZN11NativeMenus4Poll')
    changed = Symbol('_ZN15RuntimeControls7Changed')
    buildRows = Symbol('_ZN11NativeMenus9BuildRows')
    events = []

    def Callback(address):
        cpu = harness.cpu
        first, second, third = [cpu.reg_read(register) for register in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2)]
        if address == symbols['TouchGetCurrentScreen']:
            harness.returnValue(screen)
        elif address == changed:
            assert first == state
            events.append('changed')
            harness.returnValue(0)
        elif address == buildRows:
            events.append('rebuild')
            harness.returnValue(0)
        elif address == symbols['ControlsSetFloat']:
            assert first == options and second == 0xa31c64
            cpu.mem_write(options + 88, (cpu.reg_read(UC_ARM_REG_D0) & 0xffffffff).to_bytes(4, 'little'))
            harness.returnValue(0)
        elif address == symbols['ControlsSetBoolean']:
            assert first == options and second in (0xa31c5c, 0xa31d14)
            cpu.mem_write(options + (96 if second == 0xa31c5c else 116), bytes([third != 0]))
            harness.returnValue(0)
        else:
            return False
        return True

    harness.callback = Callback
    harness.word(state + harness.layout['nativeMenu'], menu)
    harness.word(state + harness.layout['screen'], screen)
    harness.word(state + harness.layout['options'], options)
    harness.word(screen + 284, 2)
    harness.word(screen + 148, widget + 0x1000)
    harness.word(widget + 0x1000 + 124, widget + 0x2000)
    harness.word(state + harness.layout['captureIndex'], 0xffffffff)
    cases = 0
    for page in range(7):
        for row in range(21):
            harness.cpu.mem_write(menu, bytes(488))
            harness.word(menu, screen)
            harness.cpu.mem_write(menu + 4, bytes([page]))
            harness.cpu.mem_write(state + harness.layout['page'], bytes([page]))
            item = menu + 8 + row * 20
            harness.execute(configure, (item, page, row))
            minimum, maximum, step, kind = struct.unpack('<HHHB', harness.cpu.mem_read(item + 12, 7))
            if kind == 0:
                cases += 1
                continue
            assert step > 0 and maximum > minimum
            harness.word(item + 8, widget)
            for value in (minimum, (minimum + maximum) // (2 * step) * step, min(maximum, 1) if kind == 2 else maximum):
                if page == 4 and row == 9 and value == 0:
                    continue
                harness.cpu.mem_write(state + harness.layout['page'], bytes([page]))
                harness.execute(setValue, (state, row, value))
                assert harness.execute(getValue, (state, row)) == value, (page, row, value)
                other = minimum if value != minimum else (min(maximum, 1) if kind == 2 else maximum)
                if page == 4 and row == 9:
                    other = 3 if value != 3 else 1
                harness.execute(setValue, (state, row, other))
                harness.cpu.mem_write(state + harness.layout['page'], bytes([page]))
                if kind == 1:
                    harness.cpu.mem_write(widget + 80, struct.pack('<f', (value - minimum) / (maximum - minimum)))
                elif kind == 2:
                    harness.cpu.mem_write(widget + 248, bytes([value]))
                else:
                    harness.word(widget + 208, value)
                    harness.cpu.mem_write(widget + 202, b'\x01')
                    events.clear()
                    harness.execute(poll, (state,))
                    assert events == [], (page, row, 'expanded dropdown committed')
                    harness.cpu.mem_write(widget + 202, b'\x00')
                events.clear()
                harness.execute(poll, (state,))
                assert harness.execute(getValue, (state, row)) == value, (page, row, value)
                expected = [] if other == value or (row == 1 and page == value) else (['rebuild', 'changed'] if row == 1 else ['changed'])
                assert events == expected, (page, row, value, events, expected)
                cases += 1

    scrollCases = 0
    for page in range(7):
        harness.cpu.mem_write(menu, bytes(488))
        harness.word(menu, screen)
        harness.cpu.mem_write(menu + 4, bytes([page]))
        harness.cpu.mem_write(state + harness.layout['page'], bytes([page]))
        harness.word(screen + 168, 4)
        harness.cpu.mem_write(widget + 0x2000 + 724, struct.pack('<f', 0.6))
        harness.execute(poll, (state,))
        assert harness.word(menu + 428 + page * 4) == 4
        assert abs(struct.unpack('<f', harness.cpu.mem_read(menu + 456 + page * 4, 4))[0] - 0.6) < 0.00001
        harness.word(screen + 168, 1)
        harness.cpu.mem_write(widget + 0x2000 + 724, struct.pack('<f', 0.0))
        harness.execute(poll, (state,))
        assert harness.word(menu + 428 + page * 4) == 4
        assert abs(struct.unpack('<f', harness.cpu.mem_read(menu + 456 + page * 4, 4))[0] - 0.6) < 0.00001
        scrollCases += 2

    def DropdownCallback(address):
        if address == 0x230b18:
            harness.returnValue(0)
            return True
        return False

    harness.callback = DropdownCallback
    dropdownCases = 0
    for selected in range(5):
        harness.word(menu + 484, 0)
        widgets = [widget + index * 256 for index in range(5)]
        for index, target in enumerate(widgets):
            harness.cpu.mem_write(target, bytes(244))
            harness.word(target, 0x9da5b8)
            harness.word(target + 120, 0x1000 + index)
            harness.cpu.mem_write(target + 8, b'\x01')
            harness.cpu.mem_write(target + 48, bytes([index == selected]))
            harness.cpu.mem_write(target + 236, struct.pack('<4H', 77, 78, 79, 80))
        for target in widgets:
            harness.execute(0x5d92fc, (target, screen, 77))
        assert [harness.cpu.mem_read(target + 202, 1)[0] for target in widgets] == [int(index == selected) for index in range(5)]
        for target in widgets:
            harness.execute(0x5d92fc, (target, screen, 77))
        assert all(harness.cpu.mem_read(target + 202, 1)[0] == 0 for target in widgets)
        dropdownCases += 2

    mixedInputCases = 0
    for selected in range(5):
        for opened in range(5):
            for index, target in enumerate(widgets):
                harness.cpu.mem_write(target + 48, bytes([index == selected]))
                harness.cpu.mem_write(target + 202, bytes([index == opened]))
            harness.word(menu + 484, widgets[opened])
            for target in widgets:
                harness.execute(0x5d92fc, (target, screen, 77))
            assert all(harness.cpu.mem_read(target + 202, 1)[0] == 0 for target in widgets), (selected, opened)
            mixedInputCases += 1

    harness.cpu.mem_write(menu, bytes(488))
    harness.word(menu, screen)
    harness.cpu.mem_write(state + harness.layout['page'], b'\x00')
    harness.word(menu + 16, widget)
    harness.cpu.mem_write(menu + 26, b'\x03')
    harness.cpu.mem_write(widget + 202, b'\x01')
    harness.callback = Callback
    harness.execute(poll, (state,))
    assert harness.word(menu + 484) == widget
    harness.cpu.mem_write(widget + 202, b'\x00')
    harness.word(widget + 208, harness.cpu.mem_read(state + harness.layout['mode'], 1)[0])
    harness.execute(poll, (state,))
    assert harness.word(menu + 484) == 0

    harness.callback = DropdownCallback
    harness.word(widget + 120, 419)
    harness.cpu.mem_write(widget + 48, b'\x00')
    harness.execute(0x5d92fc, (widget, screen, 77))
    assert harness.cpu.mem_read(widget + 202, 1)[0] == 1
    harness.callback = None
    return {'cases': cases, 'scrollMemoryCases': scrollCases, 'nativeDropdownBroadcastCases': dropdownCases + 1,
        'mixedTouchControllerDropdownCases': mixedInputCases, 'dropdownOwnerPollingCases': 2,
        'scope': 'Actual ARM row configuration, value mapping, quantization, polling and native dropdown open/confirm focus isolation, including unchanged stock ID behavior. Native allocation, row rebuild, save notifications, stock option setters and UI sound are controlled callbacks. Azahar UI validation is separate.'}
