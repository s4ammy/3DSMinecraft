import argparse
import json
import struct
from pathlib import Path

from unicorn import UcError, UC_HOOK_MEM_READ
from unicorn.arm_const import UC_ARM_REG_PC, UC_ARM_REG_R0


def CheckScrollingLifetime(harness, deferred=True):
    state, menu, screen, container, pane, replacement, client = (
        0x2000000, 0x2020000, 0x2021000, 0x2022000, 0x2023000, 0x2025000, 0x2027000)
    symbols = harness.symbols
    listenerVtable = next((value for name, value in symbols.items() if 'scrollListenerVtable' in name), None)
    flush = next((value for name, value in symbols.items() if name.startswith('_ZN11NativeMenus17FlushScrollLayout')), None)
    if deferred and (listenerVtable is None or flush is None):
        raise AssertionError('Missing deferred scrolling implementation')
    cases = []

    for fraction in (0.1, 0.5, 0.9, 1.0):
        for poison in (False, True):
            harness.cpu.mem_write(menu, bytes(0x8000))
            harness.word(state + harness.layout['nativeMenu'], menu)
            harness.word(state + harness.layout['screen'], screen)
            harness.word(menu, screen)
            harness.word(screen + 148, container)
            harness.word(container, 0x9c0e18)
            harness.word(container + 20, 252)
            harness.word(container + 24, 180)
            harness.word(container + 96, listenerVtable if deferred else 0x9c0ec8)
            harness.word(container + 100, client)
            harness.word(client + 208, client + 1024)
            harness.word(container + 124, pane)
            harness.word(container + 128, 800)
            harness.word(container + 132, 252)
            harness.word(container + 136, 180)
            harness.word(pane, 0x9a7638)
            harness.word(pane + 60, container + 96)
            harness.word(pane + 64, 1)
            harness.word(pane + 68, 1)
            harness.word(pane + 72, 2)
            harness.word(pane + 112, 252)
            harness.word(pane + 116, 180)
            harness.word(pane + 144, 252)
            harness.word(pane + 148, 400)
            freed = []
            staleReads = []
            allocations = []

            def OnRead(cpu, access, address, size, value, data):
                if pane in freed and pane <= address < pane + 728:
                    staleReads.append((cpu.reg_read(UC_ARM_REG_PC), address))

            def Callback(address):
                first = harness.cpu.reg_read(UC_ARM_REG_R0)
                if address == 0x241c88:
                    assert first == pane
                    freed.append(first)
                    if poison:
                        #Model reused allocation metadata in the dead pane, as seen in dump 7.
                        harness.word(pane + 60, 0x5544)
                        harness.cpu.mem_write(pane + 272, struct.pack('<f', 1.0))
                    harness.returnValue(0)
                elif address == symbols['ControlsAllocate']:
                    assert first == 728
                    allocations.append(first)
                    harness.returnValue(replacement)
                elif address == 0x241668:
                    assert first == replacement
                    harness.word(replacement, 0x9a7638)
                    harness.word(replacement + 64, 1)
                    harness.word(replacement + 68, 1)
                    harness.word(replacement + 72, 2)
                    harness.word(replacement + 112, 252)
                    harness.word(replacement + 116, 180)
                    harness.word(replacement + 144, 252)
                    harness.word(replacement + 148, 90)
                    harness.returnValue(replacement)
                elif address in (0x124ce4, 0x62cfb4, 0x689690):
                    harness.returnValue(0)
                else:
                    return False
                return True

            harness.callback = Callback
            hook = harness.cpu.hook_add(UC_HOOK_MEM_READ, OnRead, begin=pane, end=pane + 727)
            fault = None
            try:
                harness.execute(symbols['NativeSetScroll'], (pane,), (fraction, 0.0))
            except UcError:
                fault = harness.cpu.reg_read(UC_ARM_REG_PC)
            if deferred:
                assert fault is None and not freed and not staleReads
                assert harness.cpu.mem_read(menu + 5, 1) == b'\x01'
                harness.execute(flush, (screen,))
                assert freed == [pane] and allocations == [728] and not staleReads
                assert harness.word(container + 124) == replacement
                assert harness.word(replacement + 60) == container + 96
                assert harness.cpu.mem_read(menu + 5, 1) == b'\x00'
                harness.execute(flush, (screen,))
                assert freed == [pane]
            else:
                assert freed == [pane] and staleReads
                assert fault == (0x240dd0 if poison else None), hex(fault or 0)
            harness.cpu.hook_del(hook)
            cases.append({'fraction': fraction, 'poison': poison, 'staleReads': len(staleReads),
                'fault': hex(fault) if fault is not None else None})
    harness.callback = None
    return {'deferred': deferred, 'cases': cases,
        'scope': 'Actual ARM normalized-scroll, owner callback, layout/rebuild and clamp routines. Empty child list with changed content height. Pane destructor, allocator/constructor and external input/resource dependencies are controlled callbacks. Both retained bytes and simulated allocation-header reuse are checked. This is a deterministic lifetime regression, not an unmodified full-game reproduction.'}


def CheckScrollIntegration(harness):
    state, menu, screen, container = 0x2000000, 0x2020000, 0x2021000, 0x2022000
    symbols = harness.symbols
    buildRows = next(value for name, value in symbols.items() if name.startswith('_ZN11NativeMenus9BuildRows'))
    forget = next(value for name, value in symbols.items() if name.startswith('_ZN11NativeMenus12ForgetScreen'))
    listener = next(value for name, value in symbols.items() if 'scrollListenerVtable' in name)
    harness.cpu.mem_write(menu, bytes(0x4000))
    harness.word(state + harness.layout['nativeMenu'], menu)
    harness.word(screen + 148, container)
    harness.word(screen + 392, screen + 512)
    events = []

    def Callback(address):
        if address in (symbols['NativeOriginalOptionsInitialize'], buildRows, symbols['ControlsStringAssign']):
            harness.returnValue(0)
        elif address == symbols['ControlsGetOptions']:
            harness.returnValue(screen + 1024)
        elif address == symbols['NativeOriginalRender']:
            events.append('render')
            harness.returnValue(0)
        elif address == symbols['NativeLayoutScrollContainer']:
            assert harness.cpu.reg_read(UC_ARM_REG_R0) == container
            events.append('layout')
            harness.returnValue(0)
        else:
            return False
        return True

    harness.callback = Callback
    cases = 0
    for mode in (0, 1, 2):
        harness.word(container + 96, 0x9c0ec8)
        harness.cpu.mem_write(screen + 280, bytes([mode]))
        harness.execute(symbols['NativeInitializeOptions'], (screen,))
        assert harness.word(container + 96) == (listener if mode == 2 else 0x9c0ec8)
        cases += 1
    for matches in (False, True):
        for mode in (0, 2):
            for pending in (False, True):
                harness.word(state + harness.layout['screen'], screen if matches else screen + 1024)
                harness.cpu.mem_write(screen + 280, bytes([mode]))
                harness.cpu.mem_write(menu + 5, bytes([pending]))
                events.clear()
                harness.execute(symbols['NativeRenderOptions'], (screen, 0, 0, 0))
                assert events == (['layout', 'render'] if matches and mode == 2 and pending else ['render'])
                cases += 1
    harness.word(menu, screen)
    harness.cpu.mem_write(menu + 5, b'\x01')
    harness.execute(forget, (screen + 1024,))
    assert harness.cpu.mem_read(menu + 5, 1) == b'\x01'
    harness.execute(forget, (screen,))
    assert harness.cpu.mem_read(menu + 5, 1) == b'\x00' and harness.word(menu) == 0
    harness.callback = None
    return cases + 2


def Main():
    from checkRuntimeSettings import CSettingsHarness

    parser = argparse.ArgumentParser()
    parser.add_argument('--code', type=Path, required=True)
    parser.add_argument('--payload', type=Path, required=True)
    parser.add_argument('--previous', action='store_true')
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    harness = CSettingsHarness(args.code.read_bytes(), json.loads((args.payload / 'symbols.json').read_text()),
        json.loads((args.payload / 'state-layout.json').read_text()))
    report = CheckScrollingLifetime(harness, not args.previous)
    if not args.previous:
        report['integrationCases'] = CheckScrollIntegration(harness)
    args.output.write_text(json.dumps(report, indent=4) + '\n')
    print(json.dumps(report, indent=4))


if __name__ == '__main__':
    Main()
