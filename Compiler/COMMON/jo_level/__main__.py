"""Command line: python3 Compiler/COMMON/jo_level dump|check FILE.JOL, python3 Compiler/COMMON/jo_level gen"""

import os
import sys

if __package__ in (None, ''):
    # Run as a directory (python3 Compiler/COMMON/jo_level ...): import the package from its parent directory
    sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    __package__ = 'jo_level'

from jo_level import level, gen  # noqa: E402


def main(argv):
    if len(argv) >= 2 and argv[0] == 'dump':
        print(level.dump(argv[1]))
    elif len(argv) >= 2 and argv[0] == 'check':
        with open(argv[1], 'rb') as f:
            level.validate(f.read())
        print(argv[1], 'OK')
    elif argv[:1] == ['gen']:
        gen.write_all()
    else:
        print(__doc__)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
