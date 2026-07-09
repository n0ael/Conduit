import os
import sys

# Make the package importable as `ConduitRemote` when running pytest from
# anywhere (tests live inside the package folder).
_PKG_PARENT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
if _PKG_PARENT not in sys.path:
    sys.path.insert(0, _PKG_PARENT)
