import struct
import pynais as ns

class Profile:
    def __init__(self, uid=None, pwd=None):
        self.uid = uid
        self.pwd = pwd
    
    def __str__(self):
        return "Profile uid: [%s], pwd: [%s]" % (self.uid, self.pwd)

    def set_protobuf(self, obj):
        obj.uid = self.uid
        obj.pwd = self.pwd

    def build_from_protobuf(self, obj):
        self.uid = obj.uid
        self.pwd = obj.pwd
        return self


class Config:
    """ board configuration items and connection parameters
    """
    def __init__(self, network="local", board="", host='localhost', port=1883,
                alive_period=None, secure=False):
        self.network = network
        self.board = board
        self.host = host
        self.port = port
        self.alive_period = alive_period
        self.secure = secure

    def __str__(self):
        return "Config network: [%s], board: [%s], remote: [%s:%d]" % (
                self.network, self.board, self.host, self.port)

    def set_protobuf(self, obj):
        obj.network = self.network
        obj.board = self.board
        obj.host = self.host
        obj.port = self.port
        if self.alive_period:
            obj.alive_period = self.alive_period
        obj.secure = self.secure

    def build_from_protobuf(self, obj):
        self.network = obj.network
        self.host = obj.host
        self.board = obj.board
        self.port = obj.port
        self.alive_period = obj.alive_period
        self.secure = obj.secure
        return self

class Ack:
    """ a message acknowledgement

        Args:
            id (int): message request identifier (packet.id field value)

    """
    def __init__(self, id=None, sts=None):
        self.id = id
        self.sts = sts
    
    def __str__(self):
        return "Ack ([%s] - sts:[%s])" % (ns.msg_type(self.id), self.sts)

    def set_protobuf(self, obj):
        obj.id = self.id
        if not self.sts==None:
            obj.status = self.sts
    
    def build_from_protobuf(self, obj):
        self.id = obj.id
        if (obj.HasField('status')):
            self.sts = obj.status
        return self

