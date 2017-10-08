import sys
import logging
import asyncio
import pytest
import pynais as ns

from . import cancel_tasks

port = iter(range(4001,5000))

logging.basicConfig(
    level=logging.DEBUG,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()

def stop_loop(loop):
    loop.stop()


@pytest.mark.asyncio   
async def not_working_for_python_362_test_junction_run(event_loop):

    port1 = next(port)
    port2 = next(port)
    line1 = ns.TcpLine(port=port1)
    line2 = ns.TcpLine(port=port2)

    ns.junction.route(line1, line2)

    ns.junction.loop.call_later(1, stop_loop, ns.junction.loop)
    ns.junction.run()
    
    await ns.junction.shutdown()
    cancel_tasks()

def test_junction_run():
    port1 = next(port)
    port2 = next(port)
    line1 = ns.TcpLine(port=port1)
    line2 = ns.TcpLine(port=port2)

    ns.junction.route(line1, line2)
    
    ns.junction.loop.call_later(1, stop_loop, ns.junction.loop)
    ns.junction.run()
