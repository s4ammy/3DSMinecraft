import struct

from nativeMeshChecks import CArmHarness, chunkAddress, returnAddress, savedRegisters, stackTop
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2,
    UC_ARM_REG_R4, UC_ARM_REG_SP, UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC)


class CBuilderHarness(CArmHarness):
    def __init__(self, code):
        super().__init__(code)
        if self.word(0x7bbcf0) == 0xe92d4010:
            self.emulator.reg_write(UC_ARM_REG_C1_C0_2, 0xf00000)
            self.emulator.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
            self.emulator.reg_write(UC_ARM_REG_SP, stackTop)
            self.emulator.emu_start(0x7bbcf0, 0x7bc084, count=10000)
        self.emulator.mem_map(0x30000000, 0x100000)
        self.nextAllocation = 0x30000000
        self.allocations = {}
        self.resources = set()
        self.constructed = []
        self.destroyed = []
        self.word(chunkAddress + 578 * 4, chunkAddress + 0x1000)
        self.word(chunkAddress + 0x1000 + 880, chunkAddress + 0x2000)
        self.word(chunkAddress + 0x2000 + 16, 0x22000000)

    def word(self, address, value=None):
        if value is None:
            return struct.unpack("<I", self.emulator.mem_read(address, 4))[0]
        self.emulator.mem_write(address, struct.pack("<I", value))

    def returnValue(self, value):
        self.emulator.reg_write(UC_ARM_REG_R0, value)
        self.emulator.reg_write(UC_ARM_REG_PC, self.emulator.reg_read(UC_ARM_REG_LR))

    def onInstruction(self, emulator, address, size, userData):
        super().onInstruction(emulator, address, size, userData)
        argument = emulator.reg_read(UC_ARM_REG_R0)
        if address in (0x11493c, 0x1010ca):
            pointer = self.nextAllocation
            self.nextAllocation += (argument + 7) & ~7
            if self.nextAllocation >= 0x30100000:
                raise AssertionError("Test allocator exhausted")
            self.allocations[pointer] = argument
            self.returnValue(pointer)
        elif address in (0x1007d0, 0x2ff2f0):
            if argument:
                if argument in self.resources:
                    raise AssertionError("Resource freed before destruction")
                del self.allocations[argument]
            self.returnValue(0)
        elif address == 0x3a6790:
            if self.allocations[argument] != 0x350 or argument in self.resources:
                raise AssertionError("Invalid builder construction")
            if emulator.reg_read(UC_ARM_REG_R1) != 0x22000000:
                raise AssertionError("Chunk source changed")
            shared = emulator.reg_read(UC_ARM_REG_R2)
            self.constructed.append((argument, shared))
            self.resources.add(argument)
            self.returnValue(argument)
        elif address == 0x3a69ac:
            self.resources.remove(argument)
            self.destroyed.append(argument)
            self.returnValue(argument)


def CheckBuilderMemory(code):
    harness = CBuilderHarness(code)
    cpu = harness.emulator
    capCases = []
    for requested in (0, 1, 2, 3, 4, 5, 10, 64, 0x7fffffff, 0x80000000, 0xfffffffe, 0xffffffff):
        for index, register in enumerate(savedRegisters):
            cpu.reg_write(register, 0xabcd0000 + index)
        cpu.reg_write(UC_ARM_REG_R4, chunkAddress)
        cpu.reg_write(UC_ARM_REG_R0, requested)
        cpu.reg_write(UC_ARM_REG_SP, stackTop)
        cpu.reg_write(UC_ARM_REG_LR, returnAddress)
        cpu.mem_write(chunkAddress, b"\xa5" * 64)
        cpu.emu_start(0x1d87a0, 0x1d87a4, count=30)
        signed = struct.unpack("<i", struct.pack("<I", requested))[0]
        expected = min(signed, 4) & 0xffffffff
        snapshot = bytearray(b"\xa5" * 64)
        struct.pack_into("<I", snapshot, 20, expected)
        assert bytes(cpu.mem_read(chunkAddress, 64)) == snapshot
        assert cpu.reg_read(UC_ARM_REG_PC) == 0x1d87a4
        assert cpu.reg_read(UC_ARM_REG_SP) == stackTop
        assert cpu.reg_read(UC_ARM_REG_LR) == returnAddress
        for index, register in enumerate(savedRegisters):
            assert cpu.reg_read(register) == (chunkAddress if register == UC_ARM_REG_R4 else 0xabcd0000 + index)
        capCases.append({"requested": signed, "stored": min(signed, 4)})

    harness = CBuilderHarness(code)
    cpu = harness.emulator
    cycleCases = 0
    for count in (0, 1, 4, 4, 4, 4, 10, 4):
        harness.word(chunkAddress + 20, count)
        begin = len(harness.constructed)
        harness.execute(0x1d6614, (chunkAddress, 0, 0))
        assert harness.assertCalls == 0
        made = harness.constructed[begin:]
        assert len(made) == count + 1
        assert [shared for _, shared in made] == [0] * count + [0xac42d8], (count, made)
        first = harness.word(chunkAddress + 534 * 4)
        last = harness.word(chunkAddress + 535 * 4)
        assert last - first == count * 4
        assert [harness.word(first + i * 4) for i in range(count)] == [pointer for pointer, _ in made[:-1]]
        assert harness.word(chunkAddress + 537 * 4) == made[-1][0]
        harness.execute(0x1d6250, (chunkAddress, 0))
        assert not harness.resources
        assert harness.word(chunkAddress + 534 * 4) == harness.word(chunkAddress + 535 * 4)
        assert harness.word(chunkAddress + 537 * 4) == 0
        assert set(harness.allocations) == ({first} if first else set())
        assert harness.assertCalls == 0
        cycleCases += 1

    emptyRetries = 0
    for priority in (0, 1):
        before = bytes(cpu.mem_read(chunkAddress, 0x3000))
        allocations = dict(harness.allocations)
        for _ in range(60):
            #An invalid chunk pointer proves the empty-pool path never dereferences it.
            harness.execute(0x1d62e0, (chunkAddress, 0xdead0000, priority))
            assert bytes(cpu.mem_read(chunkAddress, 0x3000)) == before
            assert harness.allocations == allocations
            assert harness.assertCalls == 0
            emptyRetries += 1

    return {"capCases": capCases, "nativeCreateClearCycles": cycleCases, "emptyPoolRetries": emptyRetries,
        "scope": "Native cap, pool vector ownership and clear/recreate flow with controlled object constructors and allocation; native empty-pool retry leaves state untouched. Not worker concurrency or GPU validation."}
