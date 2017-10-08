import sys
import json
import logging
import asyncio
from concurrent.futures import ProcessPoolExecutor
import websockets
import pytest
import pynais as ns
import commands as cmd
import time
from . import cancel_tasks

port = iter(range(5001,5010))

logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()

def receiver_tc1(port):
    with ns.create_connection(('127.0.0.1', port)) as sock:
        response = sock.proto_receive()

        response = sock.ascii_receive()

        with pytest.raises(ns.ConnectionClosed) as e:
            sock.ascii_receive()

def receiver_tc2(port):
    with ns.create_connection(('127.0.0.1', port)) as sock:

        with pytest.raises(ns.ConnectionClosed) as e:
            sock.ascii_receive()

TC3_MSG = 'pippo'
def receiver_tc3(port):
    with ns.create_connection(('127.0.0.1', port)) as sock:
        response = sock.ascii_receive(expect_json=True)
        assert TC3_MSG == json.loads(response.decode())

async def init_tcp2_junction_impl(expect_json=False):
    port1 = next(port)
    port2 = next(port)
    line1 = ns.TcpLine(port=port1, expect_json=expect_json)
    line2 = ns.TcpLine(port=port2)

    ns.junction.route(line1, line2)
    await ns.junction.init()
    await asyncio.sleep(0.1)

    return (port1, port2)

@pytest.fixture
def init_tcp2_junction():
    """Create a junction with 2 tcp endpoints
    """
    return init_tcp2_junction_impl

async def run_receiver_and_shutdown_impl(port2, receiver):
    executor = ProcessPoolExecutor()

    asyncio.ensure_future(asyncio.get_event_loop().run_in_executor(executor, receiver, port2))

    await asyncio.sleep(0.5)

    await ns.junction.shutdown()
     
    await asyncio.sleep(1)  
    cancel_tasks()

@pytest.fixture
def run_receiver_and_shutdown(request):
    """Run receiver tcp client and shutdown junction 
    """
    return run_receiver_and_shutdown_impl

@pytest.mark.asyncio
async def test_socket_tc1(event_loop, init_tcp2_junction, run_receiver_and_shutdown):
    port1, port2 = await init_tcp2_junction()
 
    leds = cmd.Leds(red='on')

    with ns.create_connection(('127.0.0.1', port1)) as sock:
        sock.proto_send(leds)
        sock.ascii_send(b'pippo\n')

    await run_receiver_and_shutdown(port2, receiver_tc1)

@pytest.mark.asyncio
async def test_socket_tc2(event_loop, init_tcp2_junction, run_receiver_and_shutdown):
    port1, port2 = await init_tcp2_junction()

    leds = cmd.Leds(red='on')

    with ns.create_connection(('127.0.0.1', port1)) as sock:
        sock.ascii_send(b'a')

    await run_receiver_and_shutdown(port2, receiver_tc2)

@pytest.mark.asyncio
async def test_socket_tc3(event_loop, init_tcp2_junction, run_receiver_and_shutdown):
    """Send a json message
    """
    port1, port2 = await init_tcp2_junction(expect_json=True)

    jmsg = json.dumps(TC3_MSG)
    with ns.create_connection(('127.0.0.1', port1)) as sock:
        sock.ascii_send(jmsg)

    
    await run_receiver_and_shutdown(port2, receiver_tc3)
 