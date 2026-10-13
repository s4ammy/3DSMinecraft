import argparse
import json
import struct
from pathlib import Path

from unicorn import Uc, UC_ARCH_ARM, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_CPSR


class CBuilderFixture:
    def __init__(self, code):
        self.cpu = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        self.cpu.mem_map(0x100000, 0xb00000)
        self.cpu.mem_write(0x100000, code)
        self.cpu.mem_map(0x2000000, 0x20000)
        self.nextAllocation = 0x2010000
        self.records = []
        self.cpu.hook_add(UC_HOOK_CODE, self.onCode)

    def word(self, address):
        return struct.unpack("<I", self.cpu.mem_read(address, 4))[0]

    def string(self, address):
        return bytes(self.cpu.mem_read(address, 160)).split(b"\0")[0].decode()

    def onCode(self, cpu, address, size, userData):
        r0, r1 = cpu.reg_read(UC_ARM_REG_R0), cpu.reg_read(UC_ARM_REG_R1)
        if address in (0x2ff220, 0x2ff260, 0x2febbc, 0x1007d0) and not cpu.reg_read(UC_ARM_REG_CPSR) & 0x20:
            raise RuntimeError(f"Native Thumb routine {address:x} entered in ARM state")
        if address == 0x2ff220:
            cpu.mem_write(r0, struct.pack("<I", r1))
        elif address == 0x2ff260:
            cpu.mem_write(r0, bytes(cpu.mem_read(r1, 4)))
        elif address == 0x8f35a0:
            self.records.append([self.string(self.word(r1)), self.word(r1 + 4), cpu.mem_read(r1 + 8, 1)[0]])
        elif address == 0x1214f4:
            begin, end, capacity = struct.unpack("<III", cpu.mem_read(r0, 12))
            if begin == 0:
                begin = self.nextAllocation
                self.nextAllocation += 0x100
                end, capacity = begin, begin + 0x100
            cpu.mem_write(end, bytes(cpu.mem_read(r1, 4)))
            cpu.mem_write(r0, struct.pack("<III", begin, end + 4, capacity))
        elif address == 0x902ee4:
            cpu.mem_write(r0, bytes(cpu.mem_read(r1, 12)))
        elif address == 0x8f2fec:
            begin, end = struct.unpack("<II", cpu.mem_read(r1 + 4, 8))
            self.records.append([self.string(self.word(r1)), [self.string(self.word(index)) for index in range(begin, end, 4)]])
        elif address not in (0x2febbc, 0x38de6c, 0x1007d0):
            return

        cpu.reg_write(UC_ARM_REG_PC, cpu.reg_read(UC_ARM_REG_LR))

    def run(self, address, mode=0):
        self.cpu.mem_write(0x2000144, struct.pack("<I", mode))
        self.cpu.reg_write(UC_ARM_REG_R0, 0x2000000 if address == 0x4a403c else 0x2000100)
        self.cpu.reg_write(UC_ARM_REG_R1, 0x2000100 if address == 0x4a403c else 0x2000000)
        self.cpu.reg_write(UC_ARM_REG_SP, 0x200f000)
        self.cpu.reg_write(UC_ARM_REG_LR, 0x200ff00)
        self.cpu.emu_start(address, 0x200ff00, count=100000)
        if self.cpu.reg_read(UC_ARM_REG_PC) != 0x200ff00 or self.cpu.reg_read(UC_ARM_REG_SP) != 0x200f000:
            raise RuntimeError("Builder failed to return or unbalanced its stack")
        return self.records


def Main():
    parser = argparse.ArgumentParser(description="Differential ARM execution of compacted native input binding builders")
    parser.add_argument("--reference-code", type=Path, required=True)
    parser.add_argument("--patched-code", type=Path)
    arguments = parser.parse_args()
    original = arguments.reference_code.read_bytes()
    candidate = arguments.patched_code.read_bytes() if arguments.patched_code else None
    results = {}
    for address, modes in ((0x4a403c, range(6)), (0x711014, [0]), (0x710a14, [0])):
        for mode in modes:
            expected = CBuilderFixture(original).run(address, mode)
            if candidate is not None:
                actual = CBuilderFixture(candidate).run(address, mode)
                if actual != expected:
                    raise RuntimeError(f"Builder {address:x}, layout {mode} differs: {actual!r} != {expected!r}")
            results[f"{address:x}:{mode}"] = expected
    print(json.dumps(results if candidate is None else {"matchedBuilders": len(results)}, indent=4))


if __name__ == "__main__":
    Main()
