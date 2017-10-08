""" Junction is a NAIS component that routes, multiplexes and eventually
    transforms messages between applications.
"""
import asyncio
import signal
import socket
import logging
import sys
import traceback
import concurrent
import websockets

from hbmqtt.client import MQTTClient
from hbmqtt.mqtt.constants import QOS_1

from pynais import SYNC_START_B, SYNC_END_B, DLINE
from pynais.nais import is_protobuf, entity_from, is_ack, marshall, unmarshall, ConnectionClosed
from pynais.packet import Packet

A_STREAM = 0  # ascii stream
B_STREAM = 1  # binary stream
LEN_ENTER = 2  # reading len byte state
LEN_DONE = 3

#
# binary packet format
#
# |SYNC_START(8)|
#     PKT_TYPE(8)|SLINE(8)|DLINE(8)|FLAGS(8)|PAYLOAD_LEN(16)|PAYLOAD
# |SYNC_END(8)|
#
# SYNC_START = 0x1E
# SYNC_END   = 0x17

# ascii strings are messages \n terminated

# json messages

# PKT_TYPE values
PROTOBUF_T = 0x01

LOG = logging.getLogger(__name__)
LOG.setLevel(logging.DEBUG)

def my_ip_address():
    """return the public ip of the local host
    """
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.connect(("8.8.8.8", 80))
    return sock.getsockname()[0]

class Channel:
    """encapsulate the (reader, writer) stream pair
    """

    def __init__(self, reader, writer):
        self.reader = reader
        self.writer = writer

    def close(self):
        """close the channel
        """
        self.writer.close()

def pretty_print_peers(peers):
    """return the peers list formatted as a readable string
    """
    return ["{}".format(peer) for peer in peers]


class MessageTape:
    """state machine for processing input bytes
    """
    def __init__(self):
        self.expected_len = 65536
        self.cursor = 0
        self.mult = 1
        self.sts_len = 0
        self.hlen = 6
        self.plen = 0
        self.sts = A_STREAM
        self.msg = bytearray()

    def add_char(self, channel):
        """process an input byte.

        Returns:
            bool: False if the input byte is the last of a complete record
        """
        if self.sts == B_STREAM:
            self.cursor += 1
            if self.cursor == 5 or self.sts_len == LEN_ENTER:
                len_byte = int.from_bytes(channel, 'little')

                if len_byte & 0x80:
                    self.plen += self.mult * (len_byte & 0x7f)
                    self.mult *= 128
                    self.hlen += 1
                    self.sts_len = LEN_ENTER
                else:
                    self.expected_len = self.hlen + self.plen + self.mult*len_byte
                    self.sts_len = LEN_DONE

        #LOG.debug("read |%r|", channel)
        self.msg += channel

        if channel == SYNC_START_B:
            self.cursor = 0
            self.plen = 0
            self.sts = B_STREAM
            self.sts_len = 0

        elif self.sts == B_STREAM and self.cursor == self.expected_len:
            if channel == SYNC_END_B:
                self.sts = A_STREAM
                return False
        elif self.sts == A_STREAM and channel == b'\n':
            return False

        return True

#    def message(self):
#        return self.msg

def receive_from(reader, expect_json=False):
    """return a message from a channel

        Detect the type of message, currently protobuf encoded or ascii
        strings \n terminated.
        If expect_json is True return the received json message with
        a leap of faith.

    """

    parser = MessageTape()

    if expect_json:
        msg = reader(2048)
        return msg

    read_tape = True

    while read_tape:

        channel = reader(1)
        if (channel == b''):
            raise ConnectionClosed("connection closed: {}".format(reader))

        read_tape = parser.add_char(channel)

    return parser.msg

#pylint: disable=C0103
class create_connection:
    """wrapper class around a network socket
    """
    def __init__(self, address):
        self.address = address
        self.sock = None

    def __enter__(self, timeout=socket._GLOBAL_DEFAULT_TIMEOUT, source_address=None):
        self.sock = socket.create_connection(
            self.address, timeout, source_address)
        return self

    def __exit__(self, *args):
        self.sock.close()

    def proto_send(self, msg):
        """send a protobuf `msg`
        """
        self.sock.send(marshall(msg))

    def ascii_send(self, msg):
        """send an ascii `msg`
        """
        if type(msg) == str:
            msg = msg.encode('utf-8')

        n = self.sock.send(msg)

    def ascii_receive(self, expect_json=False):
        """block until an ascii message is received

        An ascii message is \n terminated

        Returns:
            bytes: a \n terminated ascii message
        """
        while True:
            msg = receive_from(self.sock.recv, expect_json)

            if not is_protobuf(msg):
                return msg

    def proto_receive(self):
        """block until a protobuf message is received

        Returns:
            object: a protobuf instance
        """
        while True:
            msg = receive_from(self.sock.recv)

            if is_protobuf(msg):
                return unmarshall(msg)

#pylint: enable=C0103



async def connect(host, port):
    """coroutine connect as soon as the server is available. return a Channel
    """
    while True:
        try:
            reader, writer = await asyncio.open_connection(host, port)
            #await asyncio.sleep(0.1)
            return Channel(reader, writer)
        except ConnectionRefusedError:
            pass


async def wsconnect(host, port):
    """coroutine connect to a websocket as soon as the server is available. return a Channel
    """
    while True:
        try:
            sock = await websockets.client.connect('ws://{}:{}'.format(host, port))
            LOG.debug('connected to ws://%s:%d', host, port)
            return sock
        except ConnectionRefusedError:
            pass


async def proto_send(channel, message, wait_ack=False):
    """coroutine send a protobuf encoded ``message`` and optionally wait for an ack

    Raises:
        InvalidMessage: if ``message`` is not marshallable
    """
    return await msg_send(channel, marshall(message), wait_ack)


async def msg_send(channel, message, wait_ack=False):
    """coroutine send an array of bytes or a string ``message``
    """
    if isinstance(message, str):
        message = message.encode('utf-8')
    channel.writer.write(message)
    while wait_ack:
        data = await msg_receive(channel)

        LOG.debug("msg_send (waiting for ack): recv |%r|", data)
        response = entity_from(data)
        if not is_ack(response, message):
            LOG.debug("ACK expected, ignoring msg: |%s|", response)
        else:
            wait_ack = False


async def proto_receive(channel, expect_json=False):
    """coroutine block until a protobuf message is received
    """
    while True:
        msg = await msg_receive(channel, expect_json)
        if is_protobuf(msg):
            return unmarshall(msg)


async def msg_receive(channel, expect_json=False):
    """coroutine block until a message string/json/protobuf is received

        Detect the type of message, currently protobuf encoded or ascii
        strings \n terminated.
        If expect_json is True return the received json message with
        a leap of faith.

    """
    parser = MessageTape()

    if expect_json:
        msg = await channel.reader.read(2048)
        return msg

    read_tape = True

    while read_tape:

        chan = await channel.reader.read(1)
        if chan == b'':
            raise ConnectionClosed("connection closed: {}".format(channel.reader))

        read_tape = parser.add_char(chan)

    return parser.msg



class Line:
    """ Base Line

        A Line has a unique identifier, set automatically

    """

    ln = 0

    sid = 0

    def __init__(self, type):
        self.expect_json = False
        self.srv = None
        self.peers = []
        self.writers = []
        self.transform = {}
        self.id = Line.ln
        Line.ln += 1
        self.type = type
        junction.attach(self)
        self.msg_queue = asyncio.Queue()

    def alloc_sid(self):
        """return a unique line instance id
        """
        Line.sid += 1
        return Line.sid

    def free_sid(self, id):
        """do nothing

           To be implemented if a previous line id may be reused by a new line
           instance
        """
        pass

    async def close(self):
        LOG.debug("closing clients")
        for c in self.writers:
            c.close()
        if self.srv:
            LOG.debug("%s: closing server", self)
            self.srv.close()
            await self.srv.wait_closed()

    async def read_msg(self, channel):
        """Detect the format of input bytes

            if no input parser are injected defaults to ascii string \n terminated
        """
        return await msg_receive(channel, self.expect_json)

    async def line_handler(self, channel):

        if not self.writers:
            # first connected client, start dispatching
            asyncio.ensure_future(self.dispatch())

        self.writers.append(channel.writer)
        channel.reader.line = self.alloc_sid()
        channel.writer.line = channel.reader.line
        try:
            reader_task = await self.reader_handler(channel)
        except (asyncio.CancelledError, concurrent.futures._base.CancelledError):
            pass
        else:
            # if the remote endpoint is a server try to reconnect
            if self.remote_is_server:
                asyncio.ensure_future(self.open())

        channel.writer.close()
        self.free_sid(channel.reader.line)
        self.writers.remove(channel.writer)


    async def deliver_to_peers(self, message, src_line):
        """Deliver the message to related peers
        """
        LOG.info("%s:line:%s <-- |%s|", self, src_line, message)
        LOG.debug("%s peers: %s", self, pretty_print_peers(self.peers))

        dline = 0
        if is_protobuf(message):
            dline = message[DLINE]

        for target in self.peers:

            #LOG.debug("queueing into %s", target)
            if target in self.transform:
                msg = Packet(message, src_line,
                             transformer=self.transform[target])
            else:
                msg = Packet(message, src_line)

            await target.msg_queue.put(msg)

    async def reader_handler(self, channel):
        """Read a message from the client peer

           The message may be a bytes array or a string, for example a json
           formatted string
        """
        try:
            while True:
                message = await self.read_msg(channel)
                if not message:
                    # return a null message if the connection is unexpectedly closed
                    break
                await self.deliver_to_peers(message, channel.reader.line)
        except asyncio.CancelledError:
            LOG.debug("task was cancelled")
            raise
        except ConnectionClosed:
            pass

        LOG.debug("connection closed: %s", self)

    async def dispatch(self):
        try:
            while True:
                LOG.debug("%s: ... waiting for messages", self)
                message = await self.msg_queue.get()

                # if (type(message) == str):
                #    message = bytes(message, 'utf-8')

                #LOG.debug("%s message to deliver: |%s|", self, message)
                for w in self.writers:
                    if w.line == message.dline or message.dline == 0:
                        pkt = message.packet
                        LOG.debug("%s:line:%s --> |%s| (PKT.DLINE %s)",
                                  self, w.line, pkt, message.dline)
                        w.write(pkt)

        except asyncio.CancelledError:
            LOG.debug("%s dispatch task: executed cancellation request", self)
        except Exception as e:
            LOG.error("dispatch error: %s", e)
            traceback.print_exc(file=sys.stderr)

    def add_peer(self, peer, transform=None):
        self.peers.append(peer)

        if transform:
            self.transform[peer] = transform

    def __str__(self):
        return "%s:%s" % (self.type, self.id)


class SerialLine(Line):
    def __init__(self, device='/dev/ttyUSB0',
                 port=2000,
                 state='raw',
                 timeout=0,
                 options='115200 8DATABITS NONE 1STOPBIT'):
        super().__init__('ser')
        self.host = 'localhost'
        self.port = port
        self.state = state
        self.timeout = timeout
        self.device = device
        self.options = options
        self.remote_is_server = False

    async def errors_from_serial(self):
        """Serial errors come from ser2net stderr and always triggers a reconnect
        """
        while True:
            msg = await self._serial.stderr.readline()
            LOG.error("%s", msg.decode().strip('\n'))
            await asyncio.sleep(2)
            await self.open_serial_client()

    async def open_serial_client(self):
        LOG.debug("connecting %s to serial server |%s|", self, self.host)
        reader, writer = await asyncio.open_connection(self.host, self.port)
        asyncio.ensure_future(self.line_handler(Channel(reader, writer)))

    async def open(self):
        """Open the serial line connection

            The reader receives the incoming messages from the server app and deliver it to the junction
            The writer sends a message received from serial port to the sever app
        """

        cline = ":".join((str(self.port), self.state, str(
            self.timeout), self.device, self.options))
        LOG.debug("ser2net C line: |%s|", cline)
        create = asyncio.create_subprocess_exec(
            'ser2net', '-d', '-C', cline, stderr=asyncio.subprocess.PIPE)

        self._serial = await create
        LOG.debug("serial channel |%s| started", self)

        asyncio.ensure_future(self.errors_from_serial())

        # open the net socket proxing the serial port
        asyncio.ensure_future(self.open_serial_client())

    async def close(self):
        LOG.debug("%s: closing serial line", self)
        self._serial.send_signal(signal.SIGTERM)
        await self._serial.wait()


class MqttLine(Line):

    def __init__(self, topic, host="localhost", port=1883):
        super().__init__('mqtt')
        self.topic = topic
        self.host = host
        self.port = port

    async def open(self):
        """Connect to mqtt broker
        """
        config = {
            'ping_delay': 0,
            'keep_alive': 60
        }
        self.mqtt = MQTTClient(config=config)

        await self.mqtt.connect('mqtt://{}:{}'.format(self.host, self.port))

        self.line = self.alloc_sid()

        asyncio.ensure_future(self.dispatch())

        await self.mqtt.subscribe([(self.topic, QOS_1)])

        asyncio.ensure_future(self.mqtt_reader_handler())

    async def close(self):
        LOG.debug("%s: closing mqtt", self)
        await self.mqtt.disconnect()

    async def mqtt_reader_handler(self):
        """Wait for messages on subscribed topics
        """
        while True:
            mqtt_msg = await self.mqtt.deliver_message()
            message = mqtt_msg.publish_packet.payload.data

            await self.deliver_to_peers(message, self.line)

    async def dispatch(self):
        """Deliver messages to mqtt broker

            Route messages enqueued by related endpoints
        """
        topic = 'hb/' + self.topic
        while True:
            LOG.debug("MQTT: dequeueing from %s", self)
            message = await self.msg_queue.get()

            # if (type(message) == str):
            #     message = bytes(message, 'utf-8')

            LOG.debug("topic: |%s| - message to deliver: |%r|",
                      topic, message)
            await self.mqtt.publish(topic, message.data)


class TcpLine(Line):
    """A tcp socket endpoint
    """

    def __init__(self, host=None, port=2000, remote_is_server=False, expect_json=False):
        super().__init__('tcp')
        self.host = host
        self.port = port
        self.remote_is_server = remote_is_server
        self.expect_json = expect_json

    async def cli_handler(self, reader, writer):
        addr = writer.get_extra_info('peername')
        LOG.debug("client connected: %r", addr)
        await self.line_handler(Channel(reader, writer))

    async def open(self):
        """Open the line connection

            The reader receives the incoming messages from the server app and
            deliver it to the junction the writer sends a message received from
            the junction to the sever app
        """

        if (self.remote_is_server):
            try:
                LOG.debug("connecting %s to remote server |%s|",
                          self, self.host)
                reader, writer = await asyncio.open_connection(self.host, self.port)
                asyncio.ensure_future(self.cli_handler(reader, writer))
                LOG.debug("connection to remote ")
            except socket.gaierror as e:
                LOG.info("%s: %s: check if hostname %s is valid",
                         self, e, self.host)
            except (OSError, ConnectionRefusedError) as e:
                LOG.info("server connection refused: %s, retrying ...", self)
                LOG.info(e)
                traceback.print_exc(file=sys.stderr)
                await asyncio.sleep(2)
                await self.open()
                #raise e
        else:
            coro = asyncio.start_server(
                self.cli_handler, self.host, self.port, family=socket.AF_INET)
            #self.srv = asyncio.ensure_future(coro)
            self.srv = await asyncio.wait_for(coro, 3)
            LOG.debug("%s server started: %s", self, self.srv)


class WSLine(Line):
    def __init__(self, host='localhost', port=2000, remote_is_server=False):
        super().__init__('ws')
        self.host = host
        self.port = port
        self.remote_is_server = remote_is_server

    async def read_msg(self, channel):
        """Read a full message in one shot from the socket

            Returns:
                bytes: read string converted to bytes
        """
        try:
            msg = await channel.reader.recv()
        except websockets.exceptions.ConnectionClosed:
            msg = ''
        return bytes(msg, 'utf-8')

    async def dispatch(self):
        """Send a json payload to all websocket clients connected
        """
        try:
            while True:
                LOG.debug("dequeueing from %s", self)
                message = await self.msg_queue.get()
                #LOG.debug("%s message to deliver: %s", self, message.data)

                jmsg = message.to_json()
                if jmsg:
                    for w in self.writers:
                        # if w.line == message.dline:
                        LOG.debug("%s:line:%s --> |%s|",
                                self, w.line, jmsg)
                        await w.send(jmsg)
        except asyncio.CancelledError:
            LOG.debug("%s dispatch task: executed cancellation request", self)

    async def cli_handler(self, websocket, path):
        LOG.debug("ws |%s| client connected: %r", websocket, path)
        await self.line_handler(Channel(websocket, websocket))

    async def open(self):
        """Open the websocket line connection

            The reader receives the incoming messages from the server app and deliver it to the junction
            The writer sends a message received from the junction to the sever app
        """
        if (self.remote_is_server):
            try:
                LOG.debug("connecting %s to remote server: %s", self, 'ws://{}:{}'.format(self.host, self.port))
                websocket = await websockets.connect('ws://{}:{}'.format(self.host, self.port))
                asyncio.ensure_future(self.cli_handler(websocket, "/"))
            except (OSError, ConnectionRefusedError) as e:
                LOG.info("server connection refused: %s, retrying ...", self)
                LOG.info(e)
                await asyncio.sleep(2)
                await self.open()
        else:
            coro = websockets.server.serve(
                self.cli_handler, self.host, self.port, family=socket.AF_INET)
            self.srv = await coro
            LOG.debug("%s server started", self)

    async def close(self):
        for c in self.writers:
            await c.close()
        if self.srv:
            LOG.debug("ws %s: closing server", self)
            self.srv.close()


class Junction:
    """
    """

    def __init__(self):
        self.queue = asyncio.Queue()
        self.loop = asyncio.get_event_loop()
        self.registry = {}

    def attach(self, line):
        """Register the line
        """
        self.registry[line.id] = line

    async def detach(self, line):
        """The inverse of Junction.attach
        """
        # first close gracefully
        await self.registry[line.id].close()
        self.registry.pop(line.id)

    def route(self, src, dst, src_to_dst=None, dst_to_src=None):
        src.add_peer(dst, src_to_dst)
        dst.add_peer(src, dst_to_src)

    async def init(self):
        for l in self.registry:
            asyncio.ensure_future(self.registry[l].open())

    def run(self):
        self.loop.run_until_complete(self.init())

        self.loop.run_forever()

    async def shutdown(self):
        # wait time in case of shutdown invoked before init finishes
        await asyncio.sleep(0.1)
        for line in list(self.registry.values()):
            await self.detach(line)


# the junction singleton
junction = Junction()
