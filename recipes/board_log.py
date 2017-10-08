"""simple example: logs from serial port are redirected to stdout
"""
import asyncio
import sys
import logging
import click

logging.basicConfig(
    level=logging.DEBUG,
    format='%(name)s: %(message)s',
    stream=sys.stderr,
)
LOG = logging.getLogger('main')

async def handler(port):
    """read log messages form junction gateway and redirect to stdout
    """

    reader, writer = await asyncio.open_connection('127.0.0.1', port)

    data = '---'
    while data:
        data = await reader.read(100)
        print('board> %r' % data.decode())

    print('Close the socket')
    writer.close()


@click.command()
@click.option('--port', default=3002, help="tcp logger port")
def main(port):
    """ loop trampoline
    """
    asyncio.get_event_loop().run_until_complete(handler(port))

# pylint: disable=E1120
if __name__ == "__main__":
    main()
