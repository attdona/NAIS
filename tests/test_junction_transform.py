import json
import sys
import logging
import asyncio
import websockets
import pytest
import pynais as ns
import commands as cmd


from . import cancel_tasks

port = iter(range(4100,5000))

logging.basicConfig(
    level=logging.DEBUG,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()



@pytest.fixture
def init_proto_to_ws_junction():
    async def init_tcp_junction_impl(expect_json=False, src_to_dst=None):
        port1 = next(port)
        port2 = next(port)
        line1 = ns.TcpLine(port=port1)
        line2 = ns.WSLine(port=port2)

        ns.junction.route(line1, line2, src_to_dst=src_to_dst)
        await ns.junction.init()
        await asyncio.sleep(0.1)

        return (port1, port2)


    """Create a junction with 2 tcpline endpoints
    """
    return init_tcp_junction_impl

@pytest.fixture
def shutdown():
    """Shutdown the junction and close running tasks
    """
    async def sdown(): 
        await ns.junction.shutdown()
        cancel_tasks()
    return sdown


def to_json(message):
    return None

@pytest.mark.asyncio
async def test_proto_to_ws(event_loop, init_proto_to_ws_junction, shutdown):
    port1, port2 = await init_proto_to_ws_junction(src_to_dst=to_json)

    tcp_sock = await ns.connect('0.0.0.0', port1)

    ws_sock = await ns.wsconnect('0.0.0.0', port2)

    #send a protobuf
    await ns.proto_send(tcp_sock, ns.msg.Profile(uid='uid', pwd='pwd'))

    await shutdown()