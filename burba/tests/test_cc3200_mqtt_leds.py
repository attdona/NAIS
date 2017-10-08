"""Test mqtt messages exchanges between a client and the cc3200 board
"""
import os
import asyncio
import logging
import sys

# the Leds message
import commands as cmd

import pytest
import pynais as ns


# the firmware test machinery
import pynais.iottest as iot

import hbmqtt.client as mqtt
from hbmqtt.mqtt.constants import QOS_0
import hbmqtt.broker

# import firmware configuration
import tc

logging.basicConfig(
    level=logging.DEBUG,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()

EVENTS = (
    ('board init', 'BOARD_INIT'),
    ('board ready', 'BOARD_READY')
)

def handle_event(event, uart):
    """set wlan uid/password using serial port
    """
    if (event.name == 'BOARD_INIT'):
        profile = ns.msg.Profile(
            os.environ['TEST_UID'], pwd=os.environ['TEST_PWD'])
        uart.write(ns.marshall(profile))

        board_cfg = ns.msg.Config(host=ns.my_ip_address(),
                                  network=tc.network, board=tc.board)
        uart.write(ns.marshall(board_cfg))

        return 1

    if (event.name == 'BOARD_READY'):
        return 1

    if (event.name == 'SERIAL_TIMEOUT'):
        assert 0


async def start_broker():
    """start mqtt borker before running the test
    """
    config = {
        'listeners': {
            'default': {
                'type': 'tcp',
                'bind': '0.0.0.0:1883',
            }
        },
        'sys_interval': 10
    }

    # start broker
    broker = hbmqtt.broker.Broker(config)
    await broker.start()

    #await a little bit for board connection ...
    await asyncio.sleep(4)

    return broker


def test_init_board():
    """compile, flash and provisioning of uid/pwd
    """

    # check that wlan env variables username and password are set 
    tc.check_test_requirements()

    iot.run('nais', tc.fw_deploy_ctx, events=EVENTS, handler=handle_event)


@pytest.mark.asyncio
async def test_switch_led():
    """switch on and off the red led
    """
    broker = await start_broker()

    msg_on = ns.marshall(cmd.Leds(red='on'))
    msg_off = ns.marshall(cmd.Leds(red='off'))

    cli = mqtt.MQTTClient()
    await cli.connect(tc.broker)

    await cli.subscribe([('hb/{}/{}'.format(tc.network, tc.board), QOS_0)])

    await cli.publish('{}/{}'.format(tc.network, tc.board), msg_on, qos=QOS_0)
    await asyncio.sleep(1)
    await cli.publish('{}/{}'.format(tc.network, tc.board), msg_off, qos=QOS_0)

    try:
        for _ in range(2):
            # wait for the ack
            message = await cli.deliver_message(timeout=3)
            packet = message.publish_packet
            LOG.debug("%s => %s",
                packet.variable_header.topic_name, str(packet.payload.data))
    except asyncio.TimeoutError:
        assert 0
    finally:
        # wait before shutting down the broker, otherwise the pub packets get lost
        await asyncio.sleep(1)
        await cli.disconnect()
        await broker.shutdown()
        tc.cancel_tasks()
