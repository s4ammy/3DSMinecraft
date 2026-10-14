import argparse
import hashlib
import json
import random
import struct
from pathlib import Path

from checkRuntimeSettings import CSettingsHarness
from buildRuntimeControls import referenceHash
from unicorn.arm_const import UC_ARM_REG_D0, UC_ARM_REG_FPSCR


class CRenderHarness(CSettingsHarness):
    def __init__(self, code):
        self.instructions = 0
        self.spriteLookups = 0
        super().__init__(code, {}, {})

    def onCode(self, cpu, address, size, data):
        self.instructions += 1
        if address == 0x702174:
            self.spriteLookups += 1
        super().onCode(cpu, address, size, data)

    def run(self, address, args=(), floats=(0.25, 0.0)):
        self.instructions = 0
        self.spriteLookups = 0
        self.cpu.reg_write(UC_ARM_REG_FPSCR, 0)
        result = self.execute(address, args, floats)
        return result


def CheckVertices(before, after):
    generator = random.Random(20261014)
    tessellator, output = 0x2008000, 0x2010000
    edgeBits = [0, 0x80000000, 1, 0x007fffff, 0x00800000, 0x3effffff, 0x3f000000,
        0x3f7fffff, 0x3f800000, 0x3f800001, 0xbf800000, 0x40000000, 0x47000000,
        0x7f800000, 0xff800000, 0x7fc00000, 0xffc00000, 0x7f800001, 0x7f7fffff]
    values = [struct.pack('<4I', value, value, value, value) for value in edgeBits]
    values += [struct.pack('<4f', *(generator.random() for _ in range(4))) for _ in range(1000)]
    values += [struct.pack('<4I', *(generator.getrandbits(32) for _ in range(4))) for _ in range(1000)]
    counts = {'cases': 0, 'normalBaselineInstructions': 0, 'normalCandidateInstructions': 0}
    for index, uv in enumerate(values):
        outputs = []
        for harness in (before, after):
            data = bytearray(generator.randbytes(0x100)) if harness == before else bytearray(initial)
            struct.pack_into('<I', data, 60, output + 16)
            data[104:120] = uv
            initial = bytes(data)
            harness.cpu.mem_write(tessellator, initial)
            harness.cpu.mem_write(output, bytes([0xa5]) * 64)
            result = harness.run(0x1b0f68, (tessellator,))
            outputs.append((result, bytes(harness.cpu.mem_read(tessellator, 0x100)), bytes(harness.cpu.mem_read(output, 64))))
            if len(edgeBits) <= index < len(edgeBits) + 1000:
                counts['normalBaselineInstructions' if harness == before else 'normalCandidateInstructions'] += harness.instructions
        assert outputs[0] == outputs[1], ('vertex', index, uv.hex(), outputs)
        assert outputs[1][0] == output + 28
        counts['cases'] += 1
    return counts


def CheckSprites(before, after):
    texture, atlas, table, records = 0x2008000, 0x2009000, 0x200a000, 0x200b000
    counts = {'cases': 0, 'cacheHitBaselineInstructions': 0, 'cacheHitCandidateInstructions': 0,
        'refreshBaselineInstructions': 0, 'refreshCandidateInstructions': 0}
    generator = random.Random(142026)
    cases = [(u, start) for u in (-1.0, -0.0, 0.0, 0.5, 1.0, 2.0, float('inf'), float('nan')) for start in (0.0, -0.0, 0.3, -0.2, float('nan'))]
    cases += [(generator.uniform(-2, 2), generator.choice([0.0, generator.random()])) for _ in range(100)]
    for address in (0x709544, 0x7095d0):
        for mode in ('hit', 'stale', 'empty', 'null-atlas-hit', 'null-atlas-stale', 'invalid-index'):
            for coordinates in cases:
                outputs = []
                for harness in (before, after):
                    harness.cpu.mem_write(texture, bytes(0x100))
                    harness.cpu.mem_write(atlas, bytes(0x100))
                    harness.cpu.mem_write(table, struct.pack('<3I', 0, 2, records))
                    harness.cpu.mem_write(records, struct.pack('<14f', 0.1, 0.2, 0.7, 0.9, 0, 0, 0, 0.3, 0.4, 0.8, 0.95, 0, 0, 0))
                    harness.cpu.mem_write(0xac5fe8, struct.pack('<3I', 0, 0, 0))
                    harness.cpu.mem_write(0xac5ff4, struct.pack('<4f', 0.0, 0.0, 1.0, 1.0))
                    harness.word(0x91ba44, 0xffffffff)
                    harness.word(atlas + 136, 123)
                    harness.word(texture + 4, 0 if mode.startswith('null') else atlas)
                    harness.word(texture + 8, table)
                    harness.word(texture + 12, 123)
                    harness.word(texture + 16, 5 if mode == 'invalid-index' else 1)
                    harness.word(texture + 20, 0 if mode == 'empty' else records)
                    harness.word(texture + 24, 123 if mode == 'hit' else 0xffffffff if mode == 'null-atlas-hit' else 122)
                    result = harness.run(address, (texture,), coordinates)
                    outputs.append((result, harness.cpu.reg_read(UC_ARM_REG_D0) & 0xffffffff,
                        bytes(harness.cpu.mem_read(texture, 0x100)), harness.spriteLookups))
                    category = 'cacheHit' if mode in ('hit', 'null-atlas-hit') else 'refresh'
                    counts[category + ('BaselineInstructions' if harness == before else 'CandidateInstructions')] += harness.instructions
                assert outputs[0] == outputs[1], ('sprite', hex(address), mode, coordinates, outputs)
                assert outputs[1][-1] == int(mode not in ('hit', 'null-atlas-hit'))
                counts['cases'] += 1
    return counts


def Main():
    parser = argparse.ArgumentParser(description="Differential ARM execution of EU allocation-free rendering routines")
    parser.add_argument('--reference-code', type=Path, required=True)
    parser.add_argument('--patched-code', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    stock, candidate = args.reference_code.read_bytes(), args.patched_code.read_bytes()
    if hashlib.sha256(stock).hexdigest() != referenceHash or len(candidate) != len(stock):
        raise ValueError('Expected matching supported EU executables')
    before, after = CRenderHarness(stock), CRenderHarness(candidate)
    report = {'codeSha256': hashlib.sha256(candidate).hexdigest(), 'vertexPacking': CheckVertices(before, after),
        'spriteCoordinates': CheckSprites(before, after),
        'scope': 'Exact EU ARM routines, output bytes, state updates, native cache refresh and callee-saved core/VFP registers. No GPU callbacks or allocations added. Instruction counts are not hardware FPS or long-session validation.'}
    assert report['vertexPacking']['normalCandidateInstructions'] < report['vertexPacking']['normalBaselineInstructions']
    assert report['spriteCoordinates']['cacheHitCandidateInstructions'] < report['spriteCoordinates']['cacheHitBaselineInstructions']
    assert report['spriteCoordinates']['refreshCandidateInstructions'] <= report['spriteCoordinates']['refreshBaselineInstructions']
    args.output.write_text(json.dumps(report, indent=4) + '\n')
    print(json.dumps(report, indent=4))


if __name__ == '__main__':
    Main()
