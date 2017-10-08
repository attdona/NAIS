"""mqtt line tests
"""
import logging
import asyncio
import pytest
import hbmqtt
import hbmqtt.broker
from hbmqtt.mqtt.constants import QOS_1
import hbmqtt.client
import pynais as ns
from . import cancel_tasks

logging.basicConfig(level=logging.DEBUG)
LOG = logging.getLogger()

CONFIG = {
    'listeners': {
        'default': {
            'type': 'tcp',
            'bind': '0.0.0.0:1883',
        }
    },
    'sys_interval': 10
}


def cancel_tasks_old():
    """Cancel background tasks to reset environment between test cases
    """
    pendings = asyncio.Task.all_tasks()

    for task in pendings:
        task.cancel()


# pylint: disable=W0613
@pytest.mark.asyncio
async def test_mqtt(event_loop):
    """ mqtt line linked to tcp and ws lines
    """
    topic = "a/b"

    # start broker
    broker = hbmqtt.broker.Broker(CONFIG)
    await broker.start()

    # start the junction
    port1 = 1883
    port2 = 3001
    port3 = 9000

    line1 = ns.MqttLine(topic=topic, port=port1)
    line2 = ns.TcpLine(port=port2)
    line3 = ns.WSLine(port=port3)

    ns.junction.route(line1, line2)
    ns.junction.route(line1, line3)

    await ns.junction.init()

    await asyncio.sleep(0.1)

    #start tcp client
    channel = await ns.connect('0.0.0.0', port2)

    #start ws client
    ws_socket = await ns.wsconnect('0.0.0.0', port3)

    #start mqtt client
    mqtt_client = hbmqtt.client.MQTTClient()
    await mqtt_client.connect('mqtt://127.0.0.1')

    await mqtt_client.subscribe([('hb/'+topic, QOS_1)])

    #send a message from tcp client
    ascii_msg = b'pippo\n'
    await ns.msg_send(channel, ascii_msg)

    json_msg = "'JSON_MSG'"
    await ws_socket.send(json_msg)

    #wait for a mqtt packet
    message = await mqtt_client.deliver_message()
    LOG.debug("received mqtt message: %s", message.publish_packet.payload.data)
    await mqtt_client.publish(topic, b'PLUTO\n', QOS_1)

    response = await ns.msg_receive(channel)

    #print the response from mqtt
    LOG.debug("response from mqtt client: %s", response)

    await mqtt_client.disconnect()
    await ns.junction.shutdown()
    await broker.shutdown()
    cancel_tasks()
