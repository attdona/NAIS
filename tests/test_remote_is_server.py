import logging
import asyncio
import pytest
import socket
import pynais as ns
from . import cancel_tasks

logging.basicConfig(level=logging.DEBUG)
LOG = logging.getLogger()


async def myserver(reader, writer):
    LOG.debug("great, apparently it works!")
    msg = await reader.readline()

    # test if reconnect to server 
    writer.close()

@pytest.mark.asyncio
async def test_name_not_known(event_loop):
    port1 = 7000
    port2 = 4001

    line1 = ns.TcpLine(port=port1)

    line2 = ns.TcpLine(host='no-one', port=port2, remote_is_server=True)

    ns.junction.route(line1, line2)

    await ns.junction.init()
    await asyncio.sleep(30)
    await ns.junction.shutdown()

    cancel_tasks()

@pytest.mark.asyncio
async def test_connection_refused(event_loop):
    port1 = 7000
    port2 = 4001

    line1 = ns.TcpLine(port=port1)

    line2 = ns.TcpLine(port=port2, remote_is_server=True)

    ns.junction.route(line1, line2)


    await ns.junction.init()
    await asyncio.sleep(2.5)
    await ns.junction.shutdown()

    cancel_tasks()
    

@pytest.mark.asyncio
async def test_remote_is_server(event_loop):

    # remember to terminate the ascii string message with a newline ('\n')
    ascii_msg = b"ciao pippo\n"

    port1 = 7000
    port2 = 4001

    loop = asyncio.get_event_loop()

    factory = asyncio.start_server(myserver, 'localhost', port2, family=socket.AF_INET)

    server = await factory
    await asyncio.sleep(1)

    line1 = ns.TcpLine(port=port1)
    line2 = ns.TcpLine(port=port2, remote_is_server=True)

    ns.junction.route(line1, line2)


    await ns.junction.init()

    #await asyncio.sleep(0.5)
    
    #start the clients
    #reader1, writer1 = await asyncio.open_connection('0.0.0.0', port1)
    channel = await ns.connect('0.0.0.0', port1)

    channel.writer.write(ascii_msg)

    await asyncio.sleep(1)

    #recv_msg = await ns.msg_receive(reader2)

    #LOG.debug("msg delivered!: %s", recv_msg)

    #assert recv_msg == ascii_msg
    
    # print("CLOSING")
    # server.close()
    # await server.wait_closed()
    # print("CLOSED")
    # await asyncio.sleep(5)

    await ns.junction.shutdown()

    cancel_tasks()


