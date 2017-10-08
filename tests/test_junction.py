import json
import sys
import logging
import asyncio
from concurrent.futures import ProcessPoolExecutor
import websockets
import pytest
import pynais as ns
import commands as cmd
import time

from . import cancel_tasks

port = iter(range(4001,5000))

logging.basicConfig(
    level=logging.DEBUG,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()



@pytest.fixture
def init_tcp_junction():
    async def init_tcp_junction_impl(expect_json=False, src_to_dst=None):
        port1 = next(port)
        port2 = next(port)
        line1 = ns.TcpLine(port=port1)
        line2 = ns.TcpLine(port=port2)

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

def test_add_protobuf_invalid_protoclass():
    """Invalid protobuf generated class, raise an exception
    """
    try:
        ns.add_protobuf_model(50, object, 'model')
    except ns.InvalidProtoclass as e:
        LOG.debug("catched exception: %r", e)


@pytest.mark.asyncio
async def test_junction_tcp_init(event_loop, shutdown):
    board = ns.TcpLine(port=next(port))
    await ns.junction.init()

    await shutdown()

@pytest.mark.asyncio
async def test_junction_ws_init(event_loop, shutdown):
    board = ns.WSLine(port=next(port))
    await ns.junction.init()

    await shutdown()

@pytest.mark.asyncio
async def test_junction_route(event_loop, shutdown, init_tcp_junction):

    # remember to terminate the ascii string message with a newline ('\n')
    ascii_msg = b"ciao pippo\n"

    port1, port2 = await init_tcp_junction()
    
    #start the clients
    reader1, writer1 = await asyncio.open_connection('0.0.0.0', port1)
    reader2, writer2 = await asyncio.open_connection('0.0.0.0', port2)
    
    writer1.write(ascii_msg)

    recv_msg = await ns.msg_receive(ns.Channel(reader2, writer2))

    LOG.debug("msg delivered!: %s", recv_msg)

    assert recv_msg == ascii_msg

    await shutdown()

@pytest.mark.asyncio
async def test_junction_protobuf(event_loop, shutdown, init_tcp_junction):
    origins = [cmd.Leds(red='on'), ns.msg.Config(host=ns.my_ip_address()), ns.msg.Profile('pippo', 'pluto'), ns.msg.Ack(1)]

    port1, port2 = await init_tcp_junction()

    #start the clients
    reader1, writer1 = await asyncio.open_connection('0.0.0.0', port1)
    reader2, writer2 = await asyncio.open_connection('0.0.0.0', port2)

    for origin in origins:

        proto_msg = ns.marshall(origin)

        writer1.write(proto_msg)

        recv_msg = await ns.msg_receive(ns.Channel(reader2,writer2))

        LOG.debug("msg delivered!: %s", recv_msg)

        received = ns.unmarshall(recv_msg)


    await shutdown()


@pytest.mark.asyncio
async def test_invalid_protobuf(event_loop, shutdown, init_tcp_junction):
    port1, port2 = await init_tcp_junction()

    chan1 = await ns.connect('0.0.0.0', port1)
    chan2 = await ns.connect('0.0.0.0', port2)

    origin = "this is not an protobuf entity message"
    
    with pytest.raises(ns.InvalidModel) as e:
        await ns.proto_send(chan1, origin)
    
    LOG.debug(e)
    await shutdown()


def morph(message):
    please_fail = 1/0
    return message

@pytest.mark.asyncio
async def test_transform(event_loop, shutdown, init_tcp_junction):

    port1, port2 = await init_tcp_junction(src_to_dst=morph)

    await ns.junction.init()

    await asyncio.sleep(0.5)

    chan1 = await ns.connect('0.0.0.0', port1)
    chan2 = await ns.connect('0.0.0.0', port2)

    origin = ns.msg.Profile(uid='uid', pwd='pwd')
    
    await ns.proto_send(chan1, origin)
    
    await shutdown()


async def ws_run(port1, port2):
    mut = json.dumps("pippo")

    websocket = await ns.wsconnect('localhost', port1)
    await websocket.send(mut)
    websocket.close()

    websocket = await ns.wsconnect('localhost', port2)
    msg = await websocket.recv()
    LOG.debug("ws recv: %s", msg)
    
    assert mut == msg
    websocket.close()


@pytest.mark.asyncio
async def test_junction_ws(event_loop, shutdown):

    port1 = 3010
    port2 = 3011
    line1 = ns.WSLine(port=port1)
    line2 = ns.WSLine(port=port2)

    ns.junction.route(line1, line2)

    task = asyncio.ensure_future(ws_run(port1, port2))
    await asyncio.sleep(0.1)
    await ns.junction.init()

    await task

    await shutdown()



class MyCmd:
    ID = 79

    def set_protobuf(self, obj):
        obj.id = 100
        for i in range(20):
            obj.svals.append('very long string')

    def build_from_protobuf(self, obj):
        self.id = obj.id

ns.setCommand(MyCmd.ID, MyCmd)


@pytest.mark.asyncio
async def test_long_message(event_loop, shutdown, init_tcp_junction):

    cmd = MyCmd()

    port1, port2 = await init_tcp_junction()

    chan1 = await ns.connect('0.0.0.0', port1)
    chan2 = await ns.connect('0.0.0.0', port2)

    await ns.proto_send(chan1, cmd)

    recv = await ns.msg_receive(chan2)

    await shutdown()


def ack_response(msg):
    ack = ns.msg.Ack(id=MyCmd.ID)
    ack.sts = 0
    return ns.marshall(ack)

@pytest.mark.asyncio
async def test_msg_send(event_loop, init_tcp_junction, shutdown):

    port1, port2 = await init_tcp_junction(src_to_dst=ack_response)

    chan1 = await ns.connect('0.0.0.0', port1)
    chan2 = await ns.connect('0.0.0.0', port2)

    await ns.msg_send(chan1, 'pippo\n')

    recv = await ns.msg_receive(chan2)

    await shutdown()


@pytest.mark.asyncio
async def test_proto_receive(event_loop, init_tcp_junction, shutdown):

    port1, port2 = await init_tcp_junction(src_to_dst=ack_response)

    chan1 = await ns.connect('0.0.0.0', port1)
    chan2 = await ns.connect('0.0.0.0', port2)

    cmd = MyCmd()
    await ns.proto_send(chan1, cmd)

    recv = await ns.proto_receive(chan2)

    #test if received packet is ack
    ns.is_ack(recv)

    ns.is_ack(recv, command_type=MyCmd)

    #test that this is not an ack
    cfg = ns.msg.Profile(uid='uid', pwd='pwd')
    ns.is_ack(cfg, command_type=MyCmd)

    chan1.close()

    await shutdown()


async def echo(in_channel):
    msg = await ns.msg_receive(in_channel)

    # test the ignored message case when waiting the ack protobuf message
    await ns.msg_send(in_channel, b'ignore this\n')
    await ns.msg_send(in_channel, msg)

@pytest.mark.asyncio
async def test_msg_send_wait_for_ack(event_loop, init_tcp_junction, shutdown):

    port1, port2 = await init_tcp_junction(src_to_dst=ack_response)

    chan1 = await ns.connect('0.0.0.0', port1)
    chan2 = await ns.connect('0.0.0.0', port2)

    cmd = MyCmd()

    asyncio.ensure_future(echo(chan2))

    await ns.proto_send(chan1, cmd, wait_ack=True)

    await shutdown()

