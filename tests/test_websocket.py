import json
import sys
import logging
import asyncio
import concurrent
from concurrent.futures import ProcessPoolExecutor
import websockets
import pytest
import pynais as ns
import commands as cmd
import time

from . import cancel_tasks

port = iter(range(4501,5000))

logging.basicConfig(
    level=logging.INFO,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()


@pytest.mark.asyncio
async def test_ws_connection_closed(event_loop):
    p1 = next(port)
    p2 = next(port)

    line1 = ns.WSLine(port=p1)
    line2 = ns.WSLine(port=p2)

    await ns.junction.init()

    sock = await ns.wsconnect('0.0.0.0', p1)

    await asyncio.sleep(0.2)
    await sock.close()

    await ns.junction.shutdown()
    cancel_tasks()

@pytest.mark.asyncio
async def test_ws_remote_connection_refused(event_loop):
    p1 = next(port)
    p2 = next(port)

    line1 = ns.WSLine(port=p1)
    line2 = ns.WSLine(port=p2, remote_is_server=True)

    await ns.junction.init()

    await asyncio.sleep(3)

    await ns.junction.shutdown()
    cancel_tasks()


async def my_server(websocket, path):
    try:
        while True:
            name = await websocket.recv()
            print("< {}".format(name))

            greeting = "Hello {}!".format(name)
            await websocket.send(greeting)
            print("> {}".format(greeting))
    except concurrent.futures._base.CancelledError:
        LOG.debug("ending test ...")
    except websockets.exceptions.ConnectionClosed:
        # connection is closed by test_ws_remote_server::ns.junction.shutdown()
        pass

@pytest.mark.asyncio
async def test_ws_remote_server(event_loop):
    p1 = next(port)
    p2 = next(port)

    start_server = websockets.serve(my_server, 'localhost', p2)
    x = await start_server

    line1 = ns.WSLine(port=p1)
    line2 = ns.WSLine(port=p2, remote_is_server=True)

    ns.junction.route(line1, line2)

    await ns.junction.init()

    sock = await ns.wsconnect('0.0.0.0', p1)
    await sock.send('hola\n')
    
    #await sock.close()

    await asyncio.sleep(3)
    await ns.junction.shutdown()

    x.close()
    await x.wait_closed()
    cancel_tasks()

@pytest.mark.asyncio
async def test_tcp_to_ws(event_loop):
    p1 = next(port)
    p2 = next(port)

    start_server = websockets.serve(my_server, 'localhost', p2)
    x = await start_server

    line1 = ns.TcpLine(port=p1)
    line2 = ns.WSLine(port=p2, remote_is_server=True)

    ns.junction.route(line1, line2)

    await ns.junction.init()

    profile = ns.msg.Profile(uid='uid', pwd='pwd')
    sock = await ns.connect('0.0.0.0', p1)
    await ns.proto_send(sock, profile)
    
    #await sock.close()

    await asyncio.sleep(3)
    await ns.junction.shutdown()

    x.close()
    await x.wait_closed()
    cancel_tasks()

