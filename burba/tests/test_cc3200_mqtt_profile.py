"""Test mqtt messages exchanges between a client and the cc3200 board
"""
import os
import asyncio
import pytest
import pynais as ns

# the firmware test machinery
import pynais.iottest as iot

import hbmqtt.client as mqtt
from hbmqtt.mqtt.constants import QOS_0
import hbmqtt.broker

# import firmware configuration
import tc

EVENTS = (
    ('board init', 'BOARD_INIT'),
    ('board ready', 'BOARD_READY')
)

def handle_event(event, uart):
    """manage events from board
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
    """start mqtt broker
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
    """compile, flash and setup wlan uid/pwd 
    """
    # check that wlan env variables username and password are set 
    tc.check_test_requirements()

    iot.run('nais', tc.fw_deploy_ctx, events=EVENTS, handler=handle_event)


@pytest.mark.asyncio
async def test_add_profile():
    """add a wlan profile
    """
    broker = await start_broker()

    profile = ns.msg.Profile(uid='baba', pwd='there_is_no_secret_for_me')
    msg = ns.marshall(profile)

    cli = mqtt.MQTTClient()
    await cli.connect(tc.broker)

    await cli.publish('{}/{}'.format(tc.network, tc.board), msg, qos=QOS_0)

    await cli.subscribe([('hb/{}/{}'.format(tc.network, tc.board), QOS_0)])

    try:
        # wait for the ack
        message = await cli.deliver_message(timeout=3)
        packet = message.publish_packet
        payload = packet.payload.data
        print("%s => %s" %
              (packet.variable_header.topic_name, str(payload)))

        ack = ns.unmarshall(payload)
        print(ack)
    except asyncio.TimeoutError:
        assert 0
    finally:
        await cli.disconnect()
        await broker.shutdown()
        tc.cancel_tasks()


