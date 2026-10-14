import argparse
import hashlib
import json
import struct
from pathlib import Path

from unicorn import UcError, UC_HOOK_MEM_INVALID
from unicorn.arm_const import UC_ARM_REG_R0, UC_ARM_REG_R1, UC_ARM_REG_R10, UC_ARM_REG_LR, UC_ARM_REG_PC

from buildRuntimeControls import lumaPathAddress, lumaPathCapacity, nativeReservations, referenceHash
from checkControlBuilders import CBuilderFixture


layeredFsPath = b"lf:/luma/titles/000400000017CA00/romfs\0"


def ReadLayout(exheader):
    if len(exheader) != 0x800:
        raise ValueError("Expected the complete compatible bootstrap ExHeader")
    textAddress, textSize = (struct.unpack_from("<I", exheader, offset)[0] for offset in (0x10, 0x18))
    roAddress, roSize = (struct.unpack_from("<I", exheader, offset)[0] for offset in (0x20, 0x28))
    roundedText = (textSize + 0xfff) & ~0xfff
    roundedRo = (roSize + 0xfff) & ~0xfff
    if textAddress != 0x100000 or roAddress != textAddress + roundedText:
        raise ValueError("Unsupported executable segment layout")
    if roAddress + roSize != lumaPathAddress or roundedRo - roSize < lumaPathCapacity:
        raise ValueError("Bootstrap does not match the guarded LayeredFS path reservation")
    if roundedText - textSize < 0x114:
        raise ValueError("Bootstrap has no space for the retained LayeredFS text payload")
    return {"pathAddress": lumaPathAddress, "pathOffset": roundedText + roSize,
        "textPaddingOffset": textSize, "textPaddingEnd": roundedText}


def InjectPath(code, exheader):
    layout = ReadLayout(exheader)
    offset = layout["pathOffset"]
    if offset + lumaPathCapacity > len(code) or len(layeredFsPath) != 39:
        raise ValueError("LayeredFS path write exceeds its reservation")
    result = bytearray(code)
    result[offset:offset + len(layeredFsPath)] = layeredFsPath
    return bytes(result)


def CheckCandidate(stock, code, exheader):
    if hashlib.sha256(stock).hexdigest() != referenceHash or len(stock) != len(code):
        raise ValueError("Expected the supported update and an unchanged executable extent")
    layout = ReadLayout(exheader)
    offset = layout["pathOffset"]
    if code[offset:offset + lumaPathCapacity] != bytes(lumaPathCapacity):
        raise AssertionError("Candidate occupies Luma's LayeredFS path reservation")
    for name, address, capacity, _ in nativeReservations:
        if address < lumaPathAddress + lumaPathCapacity and address + capacity > lumaPathAddress:
            raise AssertionError(f"Payload reservation {name} overlaps the LayeredFS path")
    begin, end = layout["textPaddingOffset"], layout["textPaddingEnd"]
    if code[begin:end] != stock[begin:end]:
        raise AssertionError("Candidate occupies the bootstrap's LayeredFS text padding")

    injected = InjectPath(code, exheader)
    cases = 0
    for address, modes in ((0x4a403c, range(6)), (0x711014, [0]), (0x710a14, [0]), (0x41efa8, [0])):
        for mode in modes:
            expected = CBuilderFixture(stock).run(address, mode)
            for candidate in (code, injected):
                fixture = CBuilderFixture(candidate)
                if fixture.run(address, mode) != expected:
                    raise AssertionError(f"Input builder {address:x}:{mode} changed after LayeredFS injection")
                if bytes(fixture.cpu.mem_read(lumaPathAddress, lumaPathCapacity)) != candidate[offset:offset + lumaPathCapacity]:
                    raise AssertionError("Input builder overwrote the LayeredFS path")
                cases += 1
    return {"builderCases": cases, "pathAddress": hex(lumaPathAddress),
        "pathBytes": len(layeredFsPath), "reservedBytes": lumaPathCapacity,
        "textPaddingPreserved": True, "pathSurvivesBuilders": True}


class CCrashFixture(CBuilderFixture):
    def onCode(self, cpu, address, size, userData):
        #Execute the actual constructor and strlen when the path corrupted the source pointer.
        if address == 0x2ff220 and cpu.reg_read(UC_ARM_REG_R1) == int.from_bytes(layeredFsPath[:4], "little"):
            return
        super().onCode(cpu, address, size, userData)


def ReproducePreviousCrash(code, exheader):
    fixture = CCrashFixture(InjectPath(code, exheader))
    reads = []

    def InvalidAccess(cpu, access, address, size, value, userData):
        reads.append({"address": hex(address), "bytes": size})
        return False

    fixture.cpu.hook_add(UC_HOOK_MEM_INVALID, InvalidAccess)
    try:
        fixture.run(0x41efa8)
    except UcError:
        registers = {name: hex(fixture.cpu.reg_read(register)) for name, register in (
            ("pc", UC_ARM_REG_PC), ("lr", UC_ARM_REG_LR), ("r0", UC_ARM_REG_R0), ("r10", UC_ARM_REG_R10))}
        if reads != [{"address": "0x2f3a666c", "bytes": 4}] or registers != {
            "pc": "0x2fe9b4", "lr": "0x2ff233", "r0": "0x2f3a666c", "r10": "0xa28464"}:
            raise AssertionError(f"Previous patch did not reproduce the hardware fault: {reads}, {registers}")
        return {"matchedHardwareDump": True, "invalidReads": reads, "registers": registers}
    raise AssertionError("Previous patch unexpectedly survived the known LayeredFS collision")


def Main():
    parser = argparse.ArgumentParser(description="Check native payloads after the bootstrap's Luma LayeredFS path write")
    parser.add_argument("--reference-code", type=Path, required=True)
    parser.add_argument("--patched-code", type=Path, required=True)
    parser.add_argument("--bootstrap-exheader", type=Path, required=True)
    parser.add_argument("--previous-code", type=Path)
    parser.add_argument("--with-path-code", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    stock, code, exheader = (path.read_bytes() for path in (args.reference_code, args.patched_code, args.bootstrap_exheader))
    report = {"codeSha256": hashlib.sha256(code).hexdigest(), "checks": CheckCandidate(stock, code, exheader),
        "previousCrash": ReproducePreviousCrash(args.previous_code.read_bytes(), exheader) if args.previous_code else None,
        "scope": "Actual ARM input builders before and after Luma's 39-byte path write, with controlled string/vector callbacks. The previous-crash check executes the native constructor and strlen for the corrupted pointer. Not a complete Luma boot or hardware validation."}
    if args.with_path_code:
        args.with_path_code.write_bytes(InjectPath(code, exheader))
    args.output.write_text(json.dumps(report, indent=4) + "\n")
    print(json.dumps(report, indent=4))


if __name__ == "__main__":
    Main()
