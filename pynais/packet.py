import logging
import json
from pynais.nais import is_protobuf, unmarshall
from pynais import SLINE, DLINE

LOG = logging.getLogger(__name__)
#LOG.setLevel(logging.DEBUG)

class Packet:
    """Encapsulate an application message
    """

    def __init__(self, data=None, src_line=0, dst_line=0, transformer=None):
        self.data = data
        self.src_line = src_line
        self.transformer = transformer

        if is_protobuf(data):
            self.dst_line = data[DLINE]
        else:
            self.dst_line = dst_line

    def to_json(self):
        """Convert message to json string
        """
        obj = self.packet
        LOG.debug("to_json: target obj is |%s|", obj)
        if obj == None:
            return 
        if is_protobuf(obj):
            obj = unmarshall(obj)
        try:
            return json.dumps(obj.__dict__)
        except AttributeError:
            return obj.decode()

    @property
    def dline(self):
        return self.dst_line

    @property
    def packet(self):
        
        if self.transformer:
            pkt = self.transformer(self.data)
        else:
            pkt = self.data

        if is_protobuf(pkt):
           pkt[SLINE] = self.src_line
        return pkt
