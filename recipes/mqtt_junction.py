"""Simple mqtt junction example
"""
import logging
import sys
import click
import pynais as ns

logging.basicConfig(
    level=logging.DEBUG,
    format='%(levelname)s:%(name)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()


@click.command()
@click.option('--dev', default='/dev/ttyUSB0', help='serial device')
@click.option('--topic', default='back/door', help='serial device')
def main(dev, topic):
    """link a mqtt channel with a serial line
    """

    mqtt = ns.MqttLine(topic=topic)   # mqtt endpoint
    board = ns.SerialLine(device=dev)  # board linked with serial wire

    ns.junction.route(src=mqtt, dst=board)

    ns.junction.run()

# pylint: disable=E1120
if __name__ == "__main__":
    main()
