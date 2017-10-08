import sys
import logging
import asyncio
from concurrent.futures import ProcessPoolExecutor
import pytest
import pynais as ns
import commands as cmd
import time
from . import cancel_tasks


@pytest.mark.asyncio
async def test_serial(event_loop):

    port1 = 6000
    line1 = ns.SerialLine(port=port1)

    await ns.junction.init()

    await asyncio.sleep(3)

    await ns.junction.shutdown()    
    cancel_tasks()
