import struct

from unicorn.arm_const import UC_ARM_REG_D0, UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_SP


def CheckNativeUi(harness, configure):
    symbols = harness.symbols
    create = next(value for name, value in symbols.items() if name.startswith('_ZN8NativeUi6Create'))
    update = next(value for name, value in symbols.items() if name.startswith('_ZN8NativeUi6Update'))
    row, reference, objectAddress, children, widget, label = [0x2026000 + index * 0x400 for index in range(6)]
    events = []
    calls = []
    choices = []

    def ReadText(address):
        return bytes(harness.cpu.mem_read(address, 160)).split(b'\0', 1)[0].decode()

    def Callback(address):
        cpu = harness.cpu
        args = [cpu.reg_read(register) for register in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3)]
        constructors = {symbols['ControlsCreateButton']: 0, symbols['NativeCreateSlider']: 1,
            symbols['NativeCreateBoolean']: 2, symbols['NativeCreateDropdown']: 3}
        if address in constructors:
            kind = constructors[address]
            assert args[0] == reference and args[1] == 0x2021000
            if kind in (1, 2, 3):
                assert args[2] == {1: 0xa31c64, 2: 0xa31c5c, 3: 0xa31c8c}[kind]
            calls.append(kind)
            if kind == 3:
                stack = cpu.reg_read(UC_ARM_REG_SP)
                count, buttonId, enabled = struct.unpack('<3I', cpu.mem_read(stack, 12))
                assert enabled == 0 and 0x1000 <= buttonId < 0x1015
                choices.extend(ReadText(harness.word(args[3] + index * 4)) for index in range(count))
            harness.word(reference, objectAddress)
            harness.word(reference + 4, 0x1234)
            harness.word(objectAddress + 56, children)
            harness.word(children, widget)
            harness.returnValue(0)
        elif address == symbols['ControlsStringAssign']:
            assert args[0] == row + 4
            events.append('label')
            harness.word(row + 4, args[1])
            harness.returnValue(0)
        elif address == symbols['NativeDropdownSelect']:
            assert args[0] == widget
            events.append('selection')
            harness.word(widget + 208, args[1])
            harness.returnValue(0)
        else:
            return False
        return True

    harness.callback = Callback
    cases = 0
    for page in range(7):
        for index in range(21):
            harness.cpu.mem_write(row, bytes(16))
            harness.cpu.mem_write(objectAddress, bytes(0x100))
            harness.cpu.mem_write(widget, bytes(0x100))
            harness.execute(configure, (row, page, index))
            minimum, maximum, step, kind = harness.cpu.mem_read(harness.word(row + 12), 4)
            calls.clear()
            choices.clear()
            harness.execute(create, (reference, row, 0x2021000, 0x1000 + index))
            assert calls == [kind] and harness.word(row + 8) == widget
            if kind == 3:
                assert len(choices) == maximum + 1
                assert all(choices)
                assert harness.word(objectAddress) == 0x99d7e0 and harness.word(objectAddress + 4) == 0x99d884
                if index == 1:
                    assert choices == ['Gameplay binds', 'Menu binds', 'Camera', 'Touch stick', 'Display', 'Binding tools', 'Overlay']
                if index == 0:
                    assert choices == ['L + Circle Pad', 'Circle Pad Pro', 'Touch stick', 'ABXY camera']
            if kind != 0:
                assert harness.word(objectAddress + 96) == row
                if kind == 2:
                    harness.cpu.mem_write(widget + 220, struct.pack('<4f', 1.0, 1.0, 1.0, 1.0))
                harness.cpu.mem_write(label, b'Original\0')
                harness.cpu.mem_write(label + 160, b'Changed\0')
                harness.word(row + 4, label)
                harness.word(widget + 208, maximum)
                events.clear()
                harness.execute(update, (row, label, maximum))
                assert events == []
                if kind == 2:
                    assert bytes(harness.cpu.mem_read(widget + 220, 16)) == struct.pack('<4f', 1.0, 1.0, 1.0, 1.0)
                harness.execute(update, (row, label + 160, minimum))
                assert events == ['label'] + (['selection'] if kind == 3 else [])
                if kind == 2:
                    assert bytes(harness.cpu.mem_read(widget + 220, 16)) == struct.pack('<4f', 1.0, 1.0, 1.0, 1.0)
                if kind == 3:
                    events.clear()
                    harness.cpu.mem_write(widget + 202, b'\x01')
                    harness.execute(update, (row, label + 160, maximum))
                    assert events == [] and harness.word(widget + 208) == minimum
            cases += 1
    harness.callback = None
    return cases


def CheckSwitchColors(harness, configure):
    assert harness.word(0x1dd184) == 0xe59f11f4
    update = next(value for name, value in harness.symbols.items() if name.startswith('_ZN8NativeUi6Update'))
    row, widget, label, texture, thumb = [0x2028000 + index * 0x400 for index in range(5)]
    white = struct.pack('<4f', 1.0, 1.0, 1.0, 1.0)
    green = struct.pack('<4f', 0.35, 1.0, 0.35, 1.0)
    vertices = []
    draws = []

    def Callback(address):
        cpu = harness.cpu
        if address == 0x4f0854:
            assert cpu.reg_read(UC_ARM_REG_R0) == widget + 152
            harness.returnValue(texture)
        elif address == 0x714bac:
            harness.returnValue(texture)
        elif address == 0x1b1bf0:
            assert cpu.reg_read(UC_ARM_REG_R1) == 4
            harness.returnValue(0)
        elif address == 0x1b300c:
            vertices.append(bytes(cpu.mem_read(0xb2dd40, 16)))
            harness.returnValue(0)
        elif address in (0x1b1984, 0x27efe0):
            if address == 0x27efe0:
                assert cpu.reg_read(UC_ARM_REG_R0) == thumb
                position = struct.unpack('<2f', cpu.reg_read(UC_ARM_REG_D0).to_bytes(8, 'little'))
                assert position == (10.0 + 21.0 * cpu.mem_read(widget + 248, 1)[0], 20.0)
            draws.append(bytes(cpu.mem_read(0xb2dd40, 16)))
            harness.returnValue(0)
        else:
            return False
        return True

    harness.callback = Callback
    cases = 0
    #This isolated fixture does not execute the game's global color initialization.
    harness.cpu.mem_write(harness.symbols['touchWhiteColor'], white)
    harness.cpu.mem_write(label, b'Toggle\0')
    harness.cpu.mem_write(texture + 4, struct.pack('<2I', 256, 256))
    for page, index in ((2, 3), (2, 4), (2, 7), (3, 7), (4, 4), (4, 6), (5, 7), (6, 5)):
        harness.cpu.mem_write(row, bytes(16))
        harness.execute(configure, (row, page, index))
        assert harness.cpu.mem_read(harness.word(row + 12) + 3, 1)[0] == 2
        harness.word(row + 4, label)
        harness.word(row + 8, widget)
        for focused in (False, True):
            harness.cpu.mem_write(widget, bytes(328))
            harness.word(widget + 12, 10)
            harness.word(widget + 16, 20)
            harness.word(widget + 324, thumb)
            harness.cpu.mem_write(widget + 48, bytes([focused]))
            harness.cpu.mem_write(widget + 200, struct.pack('<4I', 0, 195, 33, 18))
            harness.cpu.mem_write(widget + 216, b'\x01')
            harness.cpu.mem_write(widget + 220, white)
            for enabled in (False, True, False):
                harness.execute(update, (row, label, int(enabled)))
                snapshot = bytes(harness.cpu.mem_read(widget, 328))
                vertices.clear()
                draws.clear()
                harness.execute(0x1dd150, (widget,))
                assert bytes(harness.cpu.mem_read(widget + 220, 16)) == white
                assert vertices == [white] * 4 and draws == [white] * 2
                assert bytes(harness.cpu.mem_read(widget, 328)) == snapshot
                cases += 1
        #The stock renderer resets the shared draw color before drawing a switch.
        harness.cpu.mem_write(widget + 220, white)
        harness.word(widget + 244, 0xa31c5c)
        harness.cpu.mem_write(widget + 248, b'\x01')
        harness.cpu.mem_write(0xb2dd40, green)
        vertices.clear()
        draws.clear()
        harness.execute(0x1dd150, (widget,))
        assert vertices == [white] * 4 and draws == [white] * 2
        cases += 1
    harness.callback = None
    return cases


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
            harness.cpu.mem_write(menu, bytes(404))
            harness.word(menu, screen)
            harness.cpu.mem_write(menu + 4, bytes([page]))
            harness.cpu.mem_write(state + harness.layout['page'], bytes([page]))
            item = menu + 8 + row * 16
            harness.execute(configure, (item, page, row))
            minimum, maximum, step, kind = harness.cpu.mem_read(harness.word(item + 12), 4)
            expectedSetting = 25
            if page == 2:
                expectedSetting = {5: 0, 6: 1, 7: 2, 14: 18, 15: 19}.get(row, 25)
            elif page == 3 and 2 <= row <= 7:
                expectedSetting = row + 1
            elif page == 4:
                expectedSetting = row + 7 if 2 <= row <= 6 else row + 11 if 10 <= row <= 13 else 25
            elif page == 5 and row == 7:
                expectedSetting = 14
            elif page == 6 and 2 <= row <= 5:
                expectedSetting = 20 if row == 5 else row + 13
            assert harness.cpu.mem_read(harness.word(item + 12) + 4, 1)[0] == expectedSetting, (page, row, 'setting mapping')
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
        harness.cpu.mem_write(menu, bytes(404))
        harness.word(menu, screen)
        harness.cpu.mem_write(menu + 4, bytes([page]))
        harness.cpu.mem_write(state + harness.layout['page'], bytes([page]))
        harness.word(screen + 168, 4)
        harness.cpu.mem_write(widget + 0x2000 + 724, struct.pack('<f', 0.6))
        harness.execute(poll, (state,))
        assert harness.word(menu + 344 + page * 4) == 4
        assert abs(struct.unpack('<f', harness.cpu.mem_read(menu + 372 + page * 4, 4))[0] - 0.6) < 0.00001
        harness.word(screen + 168, 1)
        harness.cpu.mem_write(widget + 0x2000 + 724, struct.pack('<f', 0.0))
        harness.execute(poll, (state,))
        assert harness.word(menu + 344 + page * 4) == 4
        assert abs(struct.unpack('<f', harness.cpu.mem_read(menu + 372 + page * 4, 4))[0] - 0.6) < 0.00001
        scrollCases += 2

    def DropdownCallback(address):
        if address == 0x230b18:
            harness.returnValue(0)
            return True
        return False

    harness.callback = DropdownCallback
    dropdownCases = 0
    for selected in range(5):
        harness.word(menu + 400, 0)
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
            harness.word(menu + 400, widgets[opened])
            for target in widgets:
                harness.execute(0x5d92fc, (target, screen, 77))
            assert all(harness.cpu.mem_read(target + 202, 1)[0] == 0 for target in widgets), (selected, opened)
            mixedInputCases += 1

    harness.cpu.mem_write(menu, bytes(404))
    harness.word(menu, screen)
    harness.cpu.mem_write(state + harness.layout['page'], b'\x00')
    harness.execute(configure, (menu + 8, 0, 0))
    harness.word(menu + 16, widget)
    harness.cpu.mem_write(widget + 202, b'\x01')
    harness.callback = Callback
    harness.execute(poll, (state,))
    assert harness.word(menu + 400) == widget
    harness.cpu.mem_write(widget + 202, b'\x00')
    harness.word(widget + 208, harness.cpu.mem_read(state + harness.layout['mode'], 1)[0])
    harness.execute(poll, (state,))
    assert harness.word(menu + 400) == 0

    harness.callback = DropdownCallback
    harness.word(widget + 120, 419)
    harness.cpu.mem_write(widget + 48, b'\x00')
    harness.execute(0x5d92fc, (widget, screen, 77))
    assert harness.cpu.mem_read(widget + 202, 1)[0] == 1

    navigationEvents = []

    def NavigationCallback(address):
        if address == symbols['NativeOriginalNavigation']:
            navigationEvents.append(tuple(harness.cpu.reg_read(register) for register in (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2)))
            harness.returnValue(91)
            return True
        return DropdownCallback(address)

    harness.callback = NavigationCallback
    navigationCases = 0
    harness.cpu.mem_write(screen + 280, b'\x02')
    harness.word(widget + 196, 0)
    harness.word(widget + 216, widget + 0x3000)
    harness.word(widget + 220, widget + 0x3000 + 16)
    for capture in (-1, 0):
        harness.word(state + harness.layout['captureIndex'], capture & 0xffffffff)
        for expanded in (0, 1):
            for event in (0, 1, 2):
                for direction in range(6):
                    harness.cpu.mem_write(widget + 202, bytes([expanded]))
                    harness.cpu.mem_write(widget + 48, b'\x00')
                    harness.word(widget + 208, 3)
                    harness.word(screen + 168, 7)
                    navigationEvents.clear()
                    result = harness.execute(symbols['NativeOptionsNavigation'], (screen, event, direction))
                    consumed = capture >= 0 or expanded
                    assert result == (1 if consumed else 91)
                    assert navigationEvents == ([] if consumed else [(screen, event, direction)])
                    expected = (0 if direction == 2 else 2 if direction == 1 else 3) if expanded and capture < 0 and event == 1 else 3
                    assert harness.word(widget + 208) == expected
                    assert harness.word(screen + 168) == 7
                    navigationCases += 1
    harness.cpu.mem_write(screen + 280, b'\x00')
    navigationEvents.clear()
    assert harness.execute(symbols['NativeOptionsNavigation'], (screen, 1, 2)) == 91
    assert navigationEvents == [(screen, 1, 2)]
    harness.word(state + harness.layout['captureIndex'], 0xffffffff)
    harness.callback = None
    adapterCases = CheckNativeUi(harness, configure)
    switchColorCases = CheckSwitchColors(harness, configure)
    return {'cases': cases, 'scrollMemoryCases': scrollCases, 'nativeDropdownBroadcastCases': dropdownCases + 1,
        'mixedTouchControllerDropdownCases': mixedInputCases, 'dropdownOwnerPollingCases': 2,
        'modalNavigationCases': navigationCases + 1,
        'adapterConstructionAndLabelCases': adapterCases,
        'switchColorAndPositionCases': switchColorCases,
        'scope': 'Actual ARM row configuration, value mapping, quantization, polling and native dropdown open/confirm focus isolation, including unchanged stock ID behavior. Native allocation, row rebuild, save notifications, stock option setters and UI sound are controlled callbacks. Azahar UI validation is separate.'}
