import re
import tempfile
from pathlib import Path

from buildPerformancePayloads import Compile


def Main():
    projectPath = Path(__file__).resolve().parents[1]
    source = (projectPath / 'src/ContainerCancel.h').read_text()
    with tempfile.TemporaryDirectory(prefix='mc3ds-container-check-') as temporaryDirectory:
        code, _ = Compile('ContainerCancel.s', 0x100000, 'CloseContainerOnMenuCancel', Path(temporaryDirectory),
            linkedSymbols={'ContainerCancel': 0x100000, 'CancelReturn': 0x100000})
        record = re.search(r'containerCancelHelperHex = "([0-9a-f]+)"', source)
        if record is None or record.group(1) != code.hex() or len(code) != 16:
            raise RuntimeError('Container payload does not match its assembly source')
    print('Container cancel payload is reproducible')


if __name__ == '__main__':
    Main()
