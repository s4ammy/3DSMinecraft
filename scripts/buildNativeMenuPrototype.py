import argparse
import hashlib
import json
import struct
from pathlib import Path

from buildRuntimeControls import lumaPathAddress, lumaPathCapacity, nativeReservations as reservations, referenceHash


baselineHash = "b4a45114e7fbc85b56f500cb26d8d7c454417446ea23a3a1cf593e7cae03f3c0"
baselineIpsHash = "886a892d577987563c1cf2ea7292bb6c2bf583bf9d9af8c516e9e6062d64fe7a"


def ArmBranch(source, destination, link=False):
    distance = destination - source - 8
    if distance % 4 or not -(1 << 25) <= distance < (1 << 25):
        raise ValueError("Invalid ARM branch")
    return (0xeb000000 if link else 0xea000000) | ((distance >> 2) & 0xffffff)


def ApplyIps(original, patch):
    if patch[:5] != b"PATCH" or patch[-3:] != b"EOF":
        raise ValueError("Invalid IPS framing")
    code = bytearray(original)
    cursor = 5
    while cursor < len(patch) - 3:
        offset = int.from_bytes(patch[cursor:cursor + 3], "big")
        size = int.from_bytes(patch[cursor + 3:cursor + 5], "big")
        cursor += 5
        if size:
            value = patch[cursor:cursor + size]
            cursor += size
        else:
            size = int.from_bytes(patch[cursor:cursor + 2], "big")
            value = patch[cursor + 2:cursor + 3] * size
            cursor += 3
        if len(value) != size or offset + size > len(code):
            raise ValueError("IPS record is out of range")
        code[offset:offset + size] = value
    if cursor != len(patch) - 3:
        raise ValueError("IPS trailing bytes")
    return code


def Main():
    parser = argparse.ArgumentParser(description="Build the compact native settings experiment over the preserved v0.4.18 patch")
    parser.add_argument("--stock-code", type=Path, required=True)
    parser.add_argument("--baseline-code", type=Path, required=True)
    parser.add_argument("--baseline-ips", type=Path, help="Defaults to code.ips beside the baseline executable")
    parser.add_argument("--payload", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--overlay", type=Path)
    parser.add_argument("--render-fast-paths", type=Path, help="Optional separately verified in-place rendering payload")
    args = parser.parse_args()
    stock = args.stock_code.read_bytes()
    baseline = args.baseline_code.read_bytes()
    baselineIps = (args.baseline_ips or args.baseline_code.with_suffix(".ips")).read_bytes()
    if hashlib.sha256(stock).hexdigest() != referenceHash or hashlib.sha256(baseline).hexdigest() != baselineHash:
        raise ValueError("Expected exact stock and preserved v0.4.18 executables")
    if hashlib.sha256(baselineIps).hexdigest() != baselineIpsHash or ApplyIps(stock, baselineIps) != baseline:
        raise ValueError("The preserved v0.4.18 IPS does not match")
    symbols = json.loads((args.payload / "symbols.json").read_text())
    candidate = bytearray(baseline)
    patch = bytearray(baselineIps[:-3])
    touched = set()

    def Write(address, payload):
        offset = address - 0x100000
        if not payload or len(payload) > 0xffff or offset == 0x454f46 or offset < 0 or offset + len(payload) > len(candidate):
            raise ValueError("Invalid prototype write")
        if address < lumaPathAddress + lumaPathCapacity and address + len(payload) > lumaPathAddress:
            raise ValueError("Prototype write overlaps Luma LayeredFS path storage")
        locations = set(range(offset, offset + len(payload)))
        if touched & locations:
            raise ValueError("Prototype writes overlap")
        touched.update(locations)
        candidate[offset:offset + len(payload)] = payload
        patch.extend(offset.to_bytes(3, "big") + len(payload).to_bytes(2, "big") + payload)

    for name, address, capacity, _ in reservations:
        if name == "ReadOnly" and any(baseline[address - 0x100000:address - 0x100000 + capacity]):
            raise ValueError("Read-only segment padding is occupied")
        payload = (args.payload / f"controls{name}.bin").read_bytes()
        if len(payload) > capacity:
            raise ValueError("Prototype payload overflow")
        replacement = bytearray(payload.ljust(capacity, b"\0"))
        if name == "Hooks":
            offset = address - 0x100000 + 0xa0
            replacement[0xa0:0xb0] = baseline[offset:offset + 16]
        Write(address, replacement)

    hooks = [
        (0x108120, "RuntimeInputHook", False), (0x10b6bc, "RuntimeAccessoryHook", False),
        (0x10bcd4, "RuntimeShutdownHook", False), (0x235634, "BuildControlRows", True),
        (0x2362cc, "HandleControlButton", False), (0x236598, "DispatchControlRelease", False),
        (0x236e0c, "DestroyControlRows", False), (0x632cd0, "LoadControlSettings", True),
        (0x635034, "SaveControlSettings", True), (0x679344, "RuntimeHudDestroyHook", False),
        (0x28024c, "RenderRuntimeParticles", False), (0x2e6d40, "RuntimeWeatherHook", False),
        (0x675488, "RuntimeMapHook", False), (0x249944, "RuntimeTextureHook", False),
        (0x30a800, "SkipHiddenBlockFaces", False), (0x471074, "DrawRuntimePrompt", False),
        (0x17aa38, "GetBiomeFromCachedChunk", False), (0x1d87a0, "RuntimeBuilderLimit", False),
        (0x26e840, "NativeMainButtonEvent", False), (0x1a11ac, "NativePauseButtonEvent", False),
        (0x5d92fc, "NativeDropdownKey", False), (0x235638, "NativeSkipStockRows", False),
        (0x1e759c, "RuntimeNeighborEnd", False), (0x3c74bc, "RuntimeVisibleEnd", False),
        (0x5cde28, "RuntimeShuffleEnd", False), (0x607480, "RuntimeLoadingEnd", False),
        (0x1999ac, "RuntimeStageTotal", False), (0x199bb4, "RuntimeLoadingReady", False),
        (0x1e8d7c, "RuntimeLocalReady", False), (0x1e8d8c, "RuntimePeerReady", False),
        (0x199bac, "RuntimeProgressDivisor", False), (0x3266cc, "RuntimeResidentRadius", False),
        (0x72c498, "RuntimeMapRadius", False), (0x1971d8, "RuntimeRenderRadius", False),
        (0x19a060, "RuntimeInitialRenderRadius", False), (0x31d640, "RuntimeRenderBlocks", True)]
    for address, name, link in hooks:
        Write(address, struct.pack("<I", ArmBranch(address, symbols[name], link)))
    Write(0x679000, struct.pack("<I", ArmBranch(0x679000, symbols["DrawRuntimeControls"], True) & 0x0fffffff))
    for address, expected, name in [
        (0x9ab390 + 0x114, 0x26eda4, "NativeInitializeMainMenu"),
        (0x99d898 + 0x114, 0x1a182c, "NativeInitializePauseMenu"),
        (0x9a6fa4 + 0x114, 0x236b50, "NativeInitializeOptions"),
        (0x9a6fa4 + 424, 0x234ac4, "NativeOptionsBack"),
        (0x9a6fa4 + 400, 0x3dd1a8, "NativeOptionsNavigation"),
        (0x9a6fa4 + 272, 0x236bec, "NativeRenderOptions")]:
        if struct.unpack_from("<I", baseline, address - 0x100000)[0] != expected:
            raise ValueError(f"Native menu vtable changed at {address:#x}")
        Write(address, struct.pack("<I", symbols[name]))
    if "OnDisplayVblankWithOverlayStats" in symbols:
        if args.overlay is not None:
            raise ValueError("Integrated overlay must not be overwritten by a separate overlay")
        Write(0x4f903c, struct.pack("<I", symbols["OnDisplayVblankWithOverlayStats"]))
    elif args.overlay is not None:
        overlay = (args.overlay / "overlay.bin").read_bytes()
        overlaySymbols = json.loads((args.overlay / "symbols.json").read_text())
        capacity = 0x35d5f0 - 0x35cadc
        if not 4 < len(overlay) <= capacity or overlaySymbols["DrawDebugOverlay"] != 0x35cae0:
            raise ValueError("Overlay overlaps the preserved heap helpers")
        Write(0x35cadc, overlay.ljust(capacity, b"\0"))
        Write(0x4f903c, struct.pack("<I", overlaySymbols["OnDisplayVblankWithOverlayStats"]))
    renderRegions = []
    if args.render_fast_paths is not None:
        from buildRenderFastPaths import regions

        for name, address, capacity in regions:
            offset = address - 0x100000
            if candidate[offset:offset + capacity] != stock[offset:offset + capacity]:
                raise ValueError(f"Rendering function {name} is already modified")
            payload = (args.render_fast_paths / f"{name}.bin").read_bytes()
            if not 0 < len(payload) <= capacity:
                raise ValueError(f"Rendering payload {name} exceeds its original function")
            Write(address, payload.ljust(capacity, b"\0"))
            renderRegions.append(name)
    patch.extend(b"EOF")
    for address, value in json.loads((args.payload / "reused-strings.json").read_text()).items():
        offset = int(address, 16) - 0x100000
        expected = bytes.fromhex(value)
        if candidate[offset:offset + len(expected)] != expected:
            raise ValueError(f"Reused stock string was overwritten at {address}")
    if ApplyIps(stock, patch) != candidate or ApplyIps(baseline, patch) != candidate:
        raise AssertionError("Prototype IPS compatibility round trip failed")
    pathOffset = lumaPathAddress - 0x100000
    if candidate[pathOffset:pathOffset + lumaPathCapacity] != bytes(lumaPathCapacity):
        raise ValueError("Luma LayeredFS path storage must remain unused")
    args.output.mkdir(parents=True, exist_ok=True)
    (args.output / "code.bin").write_bytes(candidate)
    (args.output / "code.ips").write_bytes(patch)
    report = {"baselineSha256": baselineHash, "codeSha256": hashlib.sha256(candidate).hexdigest(),
        "ipsSha256": hashlib.sha256(patch).hexdigest(), "experimental": True,
        "nativeMenuStateBytes": 404, "newBssBytes": 0, "overlayBackground": "DrawDebugOverlay" in symbols or args.overlay is not None,
        "renderFastPaths": renderRegions,
        "lumaPathReservation": {"address": hex(lumaPathAddress), "bytes": lumaPathCapacity},
        "reusedChunkTableTailBytes": 32, "worldSettingsRestartRequired": True,
        "worldDefaults": {"renderChunks": 1, "nearbyRadius": 2, "nearbyOffsets": 25, "mapRadiusBlocks": 24, "deferredBuilders": 4},
        "scope": "Compact native settings and optional in-place rendering experiment. Existing settings, bindings, encoding, release files and production payload headers are preserved. Runtime validation is recorded separately."}
    (args.output / "build.json").write_text(json.dumps(report, indent=4) + "\n")
    print(json.dumps(report, indent=4))


if __name__ == "__main__":
    Main()
