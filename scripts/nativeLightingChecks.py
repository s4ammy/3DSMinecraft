import random
import struct

from unicorn import UC_HOOK_CODE


def CheckLightReads(baseline, candidate):
    context, section, lights = 0x2010000, 0x2012000, 0x2016000
    generator = random.Random(20261014)
    counts = [0, 0]
    active = [0]

    def Count(cpu, address, size, data):
        counts[active[0]] += 1

    hooks = [harness.cpu.hook_add(UC_HOOK_CODE, Count) for harness in (baseline, candidate)]
    paths = {}
    cases = 0
    try:
        for path in ('present', 'missing-section', 'missing-light'):
            totals = [0, 0]
            for index in range(1500):
                position = generator.getrandbits(32)
                shifts = generator.choice(((6, 12, 18), (4, 10, 16), (8, 16, 24)))
                localShifts = generator.choice(((0xf, 0x3c0, 2, 0xf000, 4), (0x3f, 0, 255, 0, 32)))
                data = bytearray(generator.randbytes(432))
                sectionIndex = ((position >> shifts[2]) & 3) * 16 + ((position >> shifts[1]) & 3) * 4 + ((position >> shifts[0]) & 3)
                for slot in range(64):
                    struct.pack_into('<I', data, slot * 4, section if path != 'missing-section' else 0)
                #The neighborhood is a 3x3x3 subset of a four-stride table.
                if sectionIndex >= 48:
                    position &= ~(3 << shifts[2])
                    sectionIndex &= 15
                struct.pack_into('<I', data, 352, localShifts[0])
                struct.pack_into('<I', data, 356, localShifts[1])
                data[360] = localShifts[2]
                struct.pack_into('<I', data, 368, localShifts[3])
                data[372] = localShifts[4]
                struct.pack_into('<I', data, 380, 3)
                for offset, shift in zip((388, 392, 396), shifts):
                    data[offset] = shift
                lightData = generator.randbytes(4096)
                fallback = generator.randrange(256)
                outputs = []
                for variant, harness in enumerate((baseline, candidate)):
                    harness.cpu.mem_write(context, bytes(data))
                    harness.word(section + 6144, lights if path == 'present' else 0)
                    harness.cpu.mem_write(lights, lightData)
                    harness.cpu.mem_write(0x989d32, bytes([fallback]))
                    counts[variant] = 0
                    active[0] = variant
                    result = harness.execute(0x384900, (context, position))
                    totals[variant] += counts[variant]
                    final = bytes(harness.cpu.mem_read(context, len(data)))
                    expectedContext = bytearray(data)
                    expectedContext[240 + sectionIndex] = 1
                    assert final == expectedContext
                    local = (position & localShifts[0]) | ((position & localShifts[1]) >> localShifts[2]) | ((position & localShifts[3]) >> localShifts[4])
                    expected = lightData[local] if path == 'present' else data[349] if path == 'missing-section' else fallback
                    assert result == expected, (path, index, result, expected)
                    outputs.append((result, final))
                assert outputs[0] == outputs[1]
                cases += 1
            paths[path] = {'cases': 1500, 'baselineInstructions': totals[0], 'candidateInstructions': totals[1]}
    finally:
        for harness, hook in zip((baseline, candidate), hooks):
            harness.cpu.hook_del(hook)
    return {'cases': cases, 'paths': paths,
        'scope': 'Native ARM light lookup, including touched-section writes, missing sections, absent brightness buffers, register shifts >=32, output, context and callee-saved registers. No allocation or persistent cache. Not a whole-game benchmark.'}
