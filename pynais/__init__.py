__all__ = ['nais']

SYNC_START = 0x1E
SYNC_END   = 0x17

SYNC_START_B = bytes([SYNC_START])
SYNC_END_B = bytes([SYNC_END])

SLINE = 2 # SLINE 0-based position
DLINE = 3 # DLINE 0-based position

from .nais import *
from .junction import *
from .config import *