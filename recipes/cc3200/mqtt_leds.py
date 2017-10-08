"""Switch the board leds using mqtt
"""
import asyncio
import logging
import sys
import click
from hbmqtt.client import MQTTClient, ClientException
from hbmqtt.mqtt.constants import QOS_1
import pynais as ns
import commands as cmd

logging.basicConfig(
    level=logging.INFO,
    format='%(name)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()


async def main(color, value, topic):
    """Starter
    """
    sub_topic = 'hb/{}'.format(topic)

    mqtt = MQTTClient()
    await mqtt.connect('mqtt://localhost')
    await mqtt.subscribe([(sub_topic, QOS_1)])

    leds = cmd.Leds()
    setattr(leds, color, value)
    await mqtt.publish(topic, ns.marshall(leds))

    try:
        while True:
            message = await mqtt.deliver_message()
            payload = message.publish_packet.payload.data
            if ns.is_protobuf(payload):
                obj = ns.unmarshall(payload)
                if ns.is_ack(obj):
                    break
            else:
                LOG.debug(">> %s", payload)

        await mqtt.unsubscribe([sub_topic])
        await mqtt.disconnect()

    except ClientException as cli_exc:
        LOG.error("Client exception: %s", cli_exc)


@click.command()
@click.argument('color', default='red')
@click.option('--on', 'value', flag_value='on',
              default=True)
@click.option('--off', 'value', flag_value='off')
@click.option('--topic', default='local/board')
def trampoline(color, value, topic):
    """Start asyncio loop
    """
    loop = asyncio.get_event_loop()

    loop.run_until_complete(main(color, value, topic))
    loop.close()

# pylint: disable=E1120
if __name__ == "__main__":
    trampoline()
