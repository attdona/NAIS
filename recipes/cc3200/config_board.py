import logging
import asyncio
import sys
import click
import socket
import serial
import pynais as ns

logging.basicConfig(
    level=logging.DEBUG,
    format='%(name)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()

def main_serial(dev, server_address):
    """Connect directly to serial port
    """
    with serial.Serial(dev, 115200, timeout=1) as ser:
        ser.write(ns.marshall(server_address))

async def main(junction_port, server_address):
    """Connect to serial using the junction micro-router
    """
    sock = await ns.connect('0.0.0.0', junction_port)

    # send the config message to configure the server ip address
    await ns.proto_send(sock, server_address, wait_ack=True)

    LOG.debug('socket close')
    sock.close()


@click.command()
@click.option('--junction_port', default=3002, help='listening junction port')
@click.option('--port', default=1883, help='tcp server port')
@click.option('--network', default='local', help='network name')
@click.option('--board', default='board', help='board name')
@click.option('--dev', default=None, help='serial port device')
def trampoline(junction_port, port, network, board, dev):

    junction_address = ns.my_ip_address()
    LOG.debug("server ip: %s", junction_address)
    srv_address = input("server address ([%s]):" %
                        junction_address) or junction_address

    server_address = ns.msg.Config(
        host=srv_address, port=port, board=board, network=network)

    if dev:
        main_serial(dev, server_address)
    else:   
        loop = asyncio.get_event_loop()

        loop.run_until_complete(main(junction_port, server_address))
        loop.close()


if __name__ == "__main__":
    trampoline()
