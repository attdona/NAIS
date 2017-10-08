"""Simple demo for switching board leds
"""
import click
import pynais as ns
from commands import Leds


@click.command()
@click.argument('color', default='red')
@click.option('--on', 'value', flag_value='on',
              default=True)
@click.option('--off', 'value', flag_value='off')
def main(color, value):
    """Starter
    """
    leds = Leds()
    setattr(leds, color, value)

    with ns.create_connection(('127.0.0.1', 3002)) as sock:
        sock.proto_send(leds)

        response = sock.proto_receive()

        print(response)

#pylint: disable=E1120
if __name__ == '__main__':
    main()
