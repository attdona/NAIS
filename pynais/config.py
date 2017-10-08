import os
import sys
from enum import Enum
import pynais.msg as msg

from pynais.nais import add_protobuf_model
import inspect

class MsgId(Enum):
    profile = 1
    config  = 2
    ack     = 3
    secret  = 4
    command = 5
    event   = 6

if inspect.stack()[-1].filename != 'setup.py':
    from .protobuf import messages_pb2
    add_protobuf_model(MsgId.profile.value, messages_pb2.Profile, msg.Profile)
    add_protobuf_model(MsgId.config.value, messages_pb2.Config, msg.Config)
    add_protobuf_model(MsgId.ack.value, messages_pb2.Ack, msg.Ack)



