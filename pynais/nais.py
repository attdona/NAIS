import logging
import google.protobuf.pyext.cpp_message
from pynais import SYNC_START, SYNC_END, SLINE, DLINE
from pynais.msg import Ack

LOG = logging.getLogger(__name__)
#LOG.setLevel(logging.DEBUG)

#protobuf_map = [
#    (1, nais_pb2.Profile, Profile)
#]

cid_to_classes = {}
#for (cid, pobj, cls) in protobuf_map:
#    cid_to_classes[cid] = (pobj, cls)

proto_from_class = {}
#for (cid, pobj, cls) in protobuf_map:
#    proto_from_class[cls] = (cid, pobj)


class NaisException(Exception):
    def __init__(self, msg):
        super().__init__(msg)

class UniqueObjectId(NaisException):
    pass

class InvalidObjectId(NaisException):
    pass

class InvalidProtoclass(NaisException):
    def __init__(self, msg):
        super().__init__(msg)

class InvalidModel(NaisException):
    def __init__(self, elem):
        super().__init__("[%s]: model unknown (not registered/wrong type)" % elem)

class ConnectionClosed(NaisException):
    def __init__(self, msg):
        super().__init__(msg)
    

def add_protobuf_model(oid, ProtobufClass, ModelClass):
    """Add a protobuf description class and its model class

       Add a protobuf description class and its model class to the list of
       managed protobuf related entities

       Args:
            oid (int): the unique model identifier
            ProbufClass (Class): a *_pb2 protoc generated class
            ModelClass  (Class): the domain model related to ProtobufClass 

    """
    if not type(ProtobufClass) is google.protobuf.pyext.cpp_message.GeneratedProtocolMessageType:
        raise InvalidProtoclass("expected protobug generated class, got: %r" % type(ProtobufClass))

    if oid in cid_to_classes:
        raise UniqueObjectId("duplicated object id")

    cid_to_classes[oid] = (ProtobufClass, ModelClass)
    proto_from_class[ModelClass] = (oid, ProtobufClass)  

def msg_id(message):
    """return the message identifier
    """
    return proto_from_class[type(message)][0]
        
def msg_type(id):
    """return the message type from ``id``
    """
    try:
        return cid_to_classes[id][1]
    except KeyError:
        raise InvalidObjectId('invalid message id [{}]'.format(id))

def ack(message, sts=None):
    """ build a ``message`` ack with response status ``sts``
    """
    response = Ack(msg_id(message), sts=sts)

    try:
        response.dline = message.sline
    except AttributeError:
        pass

    return response

def is_ack(response_obj, request_pkt=None, command_type=None):
    if request_pkt:
        return isinstance(response_obj, Ack) and request_pkt[1] == response_obj.id
    elif command_type:
        try:
            return cid_to_classes[response_obj.id][1] == command_type
        except AttributeError:
            return False
    else:
        return isinstance(response_obj, Ack)

def is_protobuf(pkt):
    """Check if ``pkt`` is a protobuf bytearray
    """
    try:
        return (type(pkt) == bytearray or type(pkt) == bytes) and pkt[0] == SYNC_START
    except IndexError:
        return False

def entity_from(packet):
    if not is_protobuf(packet):
        return packet
    e = unmarshall(packet)
    e.sline = packet[SLINE]
    return e


def setCommand(id, model):
    """Declare a Command identified by ``id`` and implemented by ``model``  
    """
    from .protobuf import messages_pb2
    add_protobuf_model(id, messages_pb2.Command, model)

def unmarshall(packet):
    """ return a nais object from a protobuf bytearray ``packet``
    """
    LOG.debug("unmarshall |%s| ...", packet)
    hlen = 6
    meta_t = cid_to_classes[packet[1]]
    pobj = meta_t[0]()

    multiplier = 1
    i = 5
    len = int(packet[i] & 127)
    while packet[i] & 128:
        len += int(packet[i+1] & 127) * multiplier
        multiplier *= 128
        i += 1
        hlen += 1

    #LOG.debug("payload |%r|" % packet[6:-1])
    pobj.ParseFromString(packet[hlen:-1])
    o = meta_t[1]().build_from_protobuf(pobj)
    LOG.debug("... %s", o)
    return o

def marshall(obj):
    """ return the binary buffer to be transported
    """
    if not type(obj) in proto_from_class:
        raise InvalidModel(obj)

    LOG.debug("marshall |%s| ...", obj)
    t = proto_from_class[type(obj)]

    try:
        dline = obj.dline
    except AttributeError:
        dline = 0

    packet = bytearray([SYNC_START, t[0], 0, dline, 0])
    if (t):
        proto_obj = t[1]()
        obj.set_protobuf(proto_obj)

        payload = proto_obj.SerializeToString()

        #n = len(payload).to_bytes(2, 'big')
        n = len(payload)
        multiplier = 1
        while n:
            if (n//128):
                packet += bytes([128+n%128])
            else:
                packet += bytes([n])
            n = n//128

        packet += payload
        packet += bytes([SYNC_END])
        LOG.debug("... (len %d) %s", len(packet), packet)
        return packet
