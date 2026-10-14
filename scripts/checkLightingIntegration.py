import argparse
import hashlib
import json
import sys
from pathlib import Path


projectPath = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(projectPath / 'experiments/chunkLoading'))
sys.path.insert(0, str(projectPath / 'experiments/performanceAudit'))
from checkLightingQueue import RunScene, sceneNames


def Main():
    parser = argparse.ArgumentParser(description='Compare native lighting propagation without changing queue allocation')
    parser.add_argument('--baseline', type=Path, required=True)
    parser.add_argument('--candidate', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    before, after = args.baseline.read_bytes(), args.candidate.read_bytes()
    report = {'baselineSha256': hashlib.sha256(before).hexdigest(), 'candidateSha256': hashlib.sha256(after).hexdigest(), 'scenes': {},
        'scope': 'Original ARM propagation and recalculation with synthetic preallocated neighborhoods and controlled block tables/allocator. Compare light/block bytes, touched sections and ordered propagation/recalculation. No queue replacement. No hardware FPS claim.'}
    for scene in sceneNames:
        baseline = RunScene(before, scene)
        candidate = RunScene(after, scene)
        if baseline[:3] != candidate[:3]:
            raise AssertionError(f'Native lighting output/order differs in {scene}')
        report['scenes'][scene] = {'baseline': baseline[3], 'candidate': candidate[3]}
        print(f'Matched {scene}: {baseline[3]["instructions"]} -> {candidate[3]["instructions"]} instructions', flush=True)
        args.output.write_text(json.dumps(report, indent=4) + '\n')


if __name__ == '__main__':
    Main()
