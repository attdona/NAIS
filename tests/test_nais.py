import pytest
import logging
import sys
import pynais as ns
import tests.test_pb2 as pb2
import commands as cmd

logging.basicConfig(
    level=logging.DEBUG,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()


class MyCmd:
    ID = 80

    def set_protobuf(self, obj):
        obj.id = 100
        for i in range(20):
            obj.svals.append('very long string')

    def build_from_protobuf(self, obj):
        self.id = obj.id

ns.setCommand(MyCmd.ID, MyCmd)

class DemoMsg:
    def __init__(self, name=None, rank=None):
        self.name = name
        self.rank = rank

    def __str__(self):
        return "profile name: %s, rank: %s" % (self.name, self.rank)

    def set_protobuf(self, obj):
        obj.name = self.name
        obj.rank = self.rank

    def build_from_protobuf(self, obj):
        self.name = obj.name
        self.rank = obj.rank
        return self

def test_ack():
    src = ns.msg.Profile(uid='pippo', pwd='secret')
    target = ns.ack(src)

    msg = ns.marshall(src)

    assert (ns.is_ack(target, msg))

def test_add_protobuf():
    pobj = pb2.TMsg()
    pobj.name = "pino"
    pobj.rank = 1
    ns.add_protobuf_model(51, pb2.TMsg, DemoMsg)

    packet = ns.marshall(DemoMsg("bonzo", 1))

    assert(ns.is_protobuf(packet))

def test_add_protobuf_invalid_class():
    with pytest.raises(ns.InvalidProtoclass) as e:
        ns.add_protobuf_model(51, object, DemoMsg)

def test_unique_id():

    #Leds is already registered
    with pytest.raises(ns.UniqueObjectId) as e:
        ns.setCommand(cmd.LEDS, cmd.Leds)

def test_is_protobuf():

    assert(ns.is_protobuf(b'') == False)

def test_long_message():
    cmd = MyCmd()

    m = ns.marshall(cmd)
    print(m)
    um = ns.unmarshall(m)

def test_print_commands():
    config = ns.msg.Config()
    print(config)

    config.alive_period = 100
    ns.marshall(config)

    profile = ns.msg.Profile()
    print(profile)

    with pytest.raises(ns.InvalidObjectId):
        print(ns.msg.Ack(id=100))
