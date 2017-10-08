"""Persist a WLAN profile to the board
"""
import logging
import asyncio
import sys
import click
import pynais as ns

logging.basicConfig(
    level=logging.DEBUG,
    format='%(name)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()


async def main(port):
    sock = await ns.connect('0.0.0.0', port)

    uid = ''
    while not uid:
        uid = input("enter username: ")

    pwd = ''
    while not pwd:
        pwd = input("enter password: ")

    add_profile = ns.msg.Profile(uid, pwd)

    # send the profile message to enable wifi and autoconnect the board to AP
    # send the config message to configure the server ip address
    await ns.proto_send(sock, add_profile, wait_ack=True)

    sock.close()


@click.command()
@click.option('--port', default=3002, help='line port')
def trampoline(port):
    """Starter
    """
    loop = asyncio.get_event_loop()

    loop.run_until_complete(main(port))
    loop.close()


#pylint: disable=E1120
if __name__ == "__main__":
    trampoline()
