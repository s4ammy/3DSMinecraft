import random
import struct

from unicorn import UC_ARCH_ARM, UC_HOOK_CODE, UC_MODE_ARM, Uc
from unicorn.arm_const import (UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3,
    UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8,
    UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12, UC_ARM_REG_SP,
    UC_ARM_REG_LR, UC_ARM_REG_PC, UC_ARM_REG_D0, UC_ARM_REG_C1_C0_2, UC_ARM_REG_FPEXC)


textAddress = 0x100000
returnAddress = 0x12000000
stackAddress = 0x11000000
stackTop = stackAddress + 0x1F000
chunkAddress = 0x20000000
positionAddress = 0x21000000
subchunkAddress = 0x22000000
biomeTableAddress = 0xB0F8F0
hookAddress = 0x17aa38
savedRegisters = (UC_ARM_REG_R4, UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7,
    UC_ARM_REG_R8, UC_ARM_REG_R9, UC_ARM_REG_R10, UC_ARM_REG_R11)


class CArmHarness:
    def __init__(self, code):
        self.emulator = Uc(UC_ARCH_ARM, UC_MODE_ARM)
        codeCapacity = (len(code) + 0xFFF) & ~0xFFF
        self.emulator.mem_map(textAddress, codeCapacity)
        self.emulator.mem_write(textAddress, code)
        self.emulator.mem_map(textAddress + codeCapacity, 0xB40000 - textAddress - codeCapacity)
        for address, size in ((stackAddress, 0x20000), (returnAddress, 0x1000),
                (chunkAddress, 0x10000), (positionAddress, 0x1000), (subchunkAddress, 0x10000)):
            self.emulator.mem_map(address, size)
        self.emulator.mem_write(returnAddress, bytes.fromhex("1eff2fe1"))
        self.assertAddress = struct.unpack("<I", self.emulator.mem_read(0xA35964, 4))[0]
        self.assertCalls = 0
        self.instructionCount = 0
        self.emulator.hook_add(UC_HOOK_CODE, self.onInstruction)

    def onInstruction(self, emulator, address, size, userData):
        self.instructionCount += 1
        if address == self.assertAddress:
            self.assertCalls += 1
            emulator.reg_write(UC_ARM_REG_PC, emulator.reg_read(UC_ARM_REG_LR))

    def execute(self, address, arguments, instructionLimit=2000000):
        self.assertCalls = 0
        self.instructionCount = 0
        for index, register in enumerate(savedRegisters):
            self.emulator.reg_write(register, 0xABCD0000 + index)
        for register, value in zip((UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3), arguments):
            self.emulator.reg_write(register, value)
        self.emulator.reg_write(UC_ARM_REG_SP, stackTop)
        self.emulator.reg_write(UC_ARM_REG_LR, returnAddress)
        self.emulator.emu_start(address, returnAddress, timeout=30000000, count=instructionLimit)
        if self.emulator.reg_read(UC_ARM_REG_PC) != returnAddress:
            raise AssertionError("ARM routine did not return")
        if self.emulator.reg_read(UC_ARM_REG_SP) != stackTop:
            raise AssertionError("ARM routine changed stack balance")
        if any(self.emulator.reg_read(register) != 0xABCD0000 + index for index, register in enumerate(savedRegisters)):
            raise AssertionError("ARM routine changed a callee-saved register")

        return self.emulator.reg_read(UC_ARM_REG_R0)


sourceAddress = positionAddress
worldPositionAddress = positionAddress + 0x100
dimensionAddress = positionAddress + 0x200
dimensionTableAddress = positionAddress + 0x300
dimensionCallback = 0x12000100


class CBiomeHarness(CArmHarness):
    def __init__(self, code):
        super().__init__(code)
        self.lookups = []
        self.available = {}
        self.dimensionCalls = 0
        self.table = b"".join(struct.pack("<I", 0 if index == 7 else 0x23000000 + index * 4) for index in range(256))
        self.emulator.mem_write(biomeTableAddress, self.table)
        self.emulator.mem_write(dimensionAddress, struct.pack("<I", dimensionTableAddress))
        self.emulator.mem_write(dimensionTableAddress + 0x98, struct.pack("<I", dimensionCallback))

    def onInstruction(self, emulator, address, size, userData):
        super().onInstruction(emulator, address, size, userData)
        if address in (0x17E838, 0x17E804):
            coordinates = struct.unpack("<ii", emulator.mem_read(emulator.reg_read(UC_ARM_REG_R1), 8))
            self.lookups.append((address, coordinates))
            emulator.reg_write(UC_ARM_REG_R0, self.available.get(coordinates, 0))
            emulator.reg_write(UC_ARM_REG_PC, emulator.reg_read(UC_ARM_REG_LR))
        elif address == dimensionCallback:
            self.dimensionCalls += 1
            emulator.reg_write(UC_ARM_REG_R0, 3)
            emulator.reg_write(UC_ARM_REG_PC, emulator.reg_read(UC_ARM_REG_LR))

    def setSource(self, cachedChunk, height=256, allowStage2=False):
        data = bytearray(64)
        data[8] = int(allowStage2)
        struct.pack_into("<I", data, 20, dimensionAddress)
        struct.pack_into("<h", data, 24, height)
        struct.pack_into("<I", data, 52, cachedChunk)
        self.emulator.mem_write(sourceAddress, bytes(data))

    def setChunk(self, address, x, z, biomeId):
        data = bytearray(0x200)
        struct.pack_into("<ii", data, 32, x, z)
        data[128:384] = bytes((biomeId,)) * 256
        self.emulator.mem_write(address, bytes(data))

    def query(self, x, y, z):
        self.lookups = []
        self.dimensionCalls = 0
        self.emulator.mem_write(worldPositionAddress, struct.pack("<iii", x, y, z))
        value = self.execute(hookAddress, (sourceAddress, worldPositionAddress))
        return (value, self.assertCalls, self.dimensionCalls, self.lookups,
            bytes(self.emulator.mem_read(sourceAddress, 64)),
            bytes(self.emulator.mem_read(chunkAddress, 0x2000)),
            bytes(self.emulator.mem_read(biomeTableAddress, 1024)))


def CheckBiome(baseline, candidate):
    before, after = CBiomeHarness(baseline), CBiomeHarness(candidate)
    generator = random.Random(20261003)
    coordinates = [-2147483648, -1025, -17, -16, -15, -1, 0, 1, 15, 16, 17, 1023, 2147483647]
    points = [(x, generator.choice(coordinates), z) for x in coordinates for z in coordinates]
    points += [tuple(generator.randrange(-2147483648, 2147483648) for _ in range(3)) for _ in range(128)]
    modes = ("hit", "empty", "wrong-x", "wrong-z", "missing", "null-biome", "zero-height", "negative-height")
    counts = {mode: {"cases": 0, "baselineInstructions": 0, "candidateInstructions": 0} for mode in modes}
    for x, y, z in points:
        for mode in modes:
            biomeId = 7 if mode == "null-biome" else 9
            for harness in (before, after):
                harness.setChunk(chunkAddress, (x >> 4) + int(mode == "wrong-x"),
                    (z >> 4) + int(mode == "wrong-z"), biomeId)
                harness.setChunk(chunkAddress + 0x200, x >> 4, z >> 4, biomeId)
                harness.available = {} if mode == "missing" else {(x >> 4, z >> 4): chunkAddress + 0x200}
                height = 0 if mode == "zero-height" else -1 if mode == "negative-height" else 256
                cached = 0 if mode in ("empty", "missing") else chunkAddress
                harness.setSource(cached, height, bool(x & 1))

            expected, observed = before.query(x, y, z), after.query(x, y, z)
            if expected != observed:
                raise AssertionError(f"Biome query differs for {mode}: {(x, y, z)}")

            expectedId = 3 if mode in ("missing", "null-biome", "zero-height", "negative-height") else 9
            if observed[0] != 0x23000000 + expectedId * 4:
                raise AssertionError("Biome query disagrees with independent fixture model")

            item = counts[mode]
            item["cases"] += 1
            item["baselineInstructions"] += before.instructionCount
            item["candidateInstructions"] += after.instructionCount

    for harness in (before, after):
        harness.setChunk(chunkAddress, -1, 2, 0)
        harness.emulator.mem_write(chunkAddress + 128, bytes(range(256)))
        harness.setSource(chunkAddress)

    columnCases = 0
    for localX in range(16):
        for localZ in range(16):
            arguments = (localX - 16, -100, localZ + 32)
            expected, observed = before.query(*arguments), after.query(*arguments)
            biomeId = localX + 16 * localZ
            expectedId = 3 if biomeId == 7 else biomeId
            if expected != observed or observed[0] != 0x23000000 + expectedId * 4:
                raise AssertionError("Nonuniform biome columns or null-entry fallback differ")

            columnCases += 1

    for harness in (before, after):
        harness.setSource(0)
        harness.available = {}

    mutationCases = 0
    for step in range(4):
        for harness in (before, after):
            if step:
                harness.available[(0, 0)] = chunkAddress
                harness.setChunk(chunkAddress, 0, 0, 10 + step)

        expected, observed = before.query(5, 70, 9), after.query(5, 70, 9)
        expectedId = 3 if step == 0 else 10 + step
        if expected != observed or observed[0] != 0x23000000 + expectedId * 4:
            raise AssertionError("Newly available neighbor or changed biome ID became stale")

        mutationCases += 1

    for harness in (before, after):
        harness.available = {}
        for cx in range(1, 4):
            for cz in range(-2, 1):
                address = chunkAddress + len(harness.available) * 0x200
                harness.available[(cx, cz)] = address
                harness.setChunk(address, cx, cz, (cx * 11 + cz) & 0xff)
        harness.setSource(0)

    region = {"queries": 0, "baselineInstructions": 0, "candidateInstructions": 0, "coldLookups": 0}
    for x in range(32, 48):
        for z in range(-16, 0):
            for dz in range(-2, 3):
                for dx in range(-2, 3):
                    arguments = (x + 4 * dx, 70, z + 4 * dz)
                    if before.query(*arguments) != after.query(*arguments):
                        raise AssertionError("Biome refresh lookup sequence differs")

                    region["queries"] += 1
                    region["baselineInstructions"] += before.instructionCount
                    region["candidateInstructions"] += after.instructionCount
                    region["coldLookups"] += len(after.lookups)

    return {"cases": sum(item["cases"] for item in counts.values()) + columnCases + mutationCases,
        "paths": counts, "nonuniformColumnCases": columnCases, "availabilityAndMutationCases": mutationCases,
        "refreshQueryTrace": region,
        "scope": "Native biome getter, chunk cache, coordinate conversion and biome table routines. Cold chunk resolver and dimension virtual are controlled callbacks. Compare result, assertions, resolver order, cache state, chunk/table memory, stack balance and callee-saved registers. Not full color output, hardware cycles or whole-game FPS."}


def CheckHiddenCase(code, mask, grass, helperAddress=0x35d200):
    cpu = Uc(UC_ARCH_ARM, UC_MODE_ARM)
    cpu.mem_map(0x100000, 0xb00000)
    cpu.mem_write(0x100000, code)
    cpu.mem_map(0x2000000, 0x10000)
    cpu.mem_map(0x3000000, 0x10000)
    stack, tessellator, occluder, block, result, stop = 0x200e000, 0x3000000, 0x3001000, 0x3002000, 0x3003000, 0x300f000
    cpu.mem_write(tessellator, bytes([0x39]) * 4096)
    cpu.mem_write(occluder, struct.pack('<I', mask or 0))
    cpu.mem_write(stack + 0x94, struct.pack('<I', result))
    saved = [0x55500000 + index for index in range(8)] + [stop]
    cpu.mem_write(stack + 0x9c, struct.pack('<9I', *saved))
    registers = [UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R2, UC_ARM_REG_R3, UC_ARM_REG_R4,
        UC_ARM_REG_R5, UC_ARM_REG_R6, UC_ARM_REG_R7, UC_ARM_REG_R8, UC_ARM_REG_R9,
        UC_ARM_REG_R10, UC_ARM_REG_R11, UC_ARM_REG_R12]
    values = [stack + 8, tessellator + 0xcc, 123, 456, tessellator, 0,
        occluder if mask is not None else 0, 3, block, 0 if grass else 1, 0 if grass else 1, 0x3a03126f, 0x1234]
    for register, value in zip(registers, values):
        cpu.reg_write(register, value)
    cpu.reg_write(UC_ARM_REG_SP, stack)
    cpu.reg_write(UC_ARM_REG_LR, 0x30a7e4)
    cpu.reg_write(UC_ARM_REG_C1_C0_2, 0xf00000)
    cpu.reg_write(UC_ARM_REG_FPEXC, 0x40000000)
    for index in range(16):
        cpu.reg_write(UC_ARM_REG_D0 + index, 0x3ff0000000000000 + index)
    calls, occlusionCalls, instructions = [], [], 0

    def OnCode(machine, address, size, userData):
        nonlocal instructions
        instructions += 1
        if 0x6bb090 <= address <= 0x6bb0f0:
            if address == 0x6bb090:
                occlusionCalls.append(machine.reg_read(UC_ARM_REG_R1))
            return
        if 0x30a800 <= address <= 0x30ac08 or helperAddress <= address < helperAddress + 0x40:
            return
        args = [machine.reg_read(register) for register in registers[:4]]
        calls.append([hex(address), args])
        #Only unchanged downstream geometry/lighting dependencies are controlled.
        #The complete native face checks, branches and cleanup execute directly.
        machine.reg_write(UC_ARM_REG_R0, result)
        machine.reg_write(UC_ARM_REG_PC, machine.reg_read(UC_ARM_REG_LR))

    cpu.hook_add(UC_HOOK_CODE, OnCode)
    cpu.emu_start(0x30a800, stop, count=5000)
    assert cpu.reg_read(UC_ARM_REG_PC) == stop
    assert cpu.reg_read(UC_ARM_REG_SP) == stack + 0xc0
    assert [cpu.reg_read(register) for register in registers[4:12]] == saved[:8]
    assert [cpu.reg_read(UC_ARM_REG_D0 + index) for index in range(16)] == [0x3ff0000000000000 + index for index in range(16)]
    return {'result': cpu.reg_read(UC_ARM_REG_R0), 'calls': calls, 'occlusionCalls': occlusionCalls,
        'instructions': instructions, 'memory': bytes(cpu.mem_read(0x3000000, 0x10000)),
        'liveStack': bytes(cpu.mem_read(stack, 0xc0))}

