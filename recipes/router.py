"""A junction demo setup.
"""
import logging
import sys
import json
import click
import pynais as ns
import commands as cmd

logging.basicConfig(
    level=logging.DEBUG,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr
)
LOG = logging.getLogger()

def to_protobuf(in_packet):
    """ Transform a json packet to a protobuf equivalent

        Args:
            in_packet (str): json string
        Returns:
            a protobuf object or None if unable to transform the input packet.
            If return None the packet is silently discarded


    """
    LOG.debug("to_protobuf - input: %s, msg: %s", type(in_packet), in_packet)

    obj = json.loads(in_packet.decode())

    packet = None

    # check if the received json packet is a profile command
    if 'uid' in obj and 'pwd' in obj:
        packet = ns.marshall(ns.msg.Profile(obj['uid'], obj['pwd']))

    # check if json packet is a led command
    elif 'leds' in obj:
        if obj['leds'] == 'get':
            packet = ns.marshall(cmd.Leds())
        elif obj['leds'] == 'set':
            packet = ns.marshall(
                cmd.Leds(red=obj['red'] if 'red' in obj else None,
                         yellow=obj['yellow'] if 'yellow' in obj else None))
    else:
        LOG.info("to_protobuf - unable to convert message %s", in_packet)
    return packet


def to_json(in_packet):
    """Convert a protobuf into a json message
    """
    LOG.debug("to_json - input: %s, msg: %s", type(in_packet), in_packet)

    # from protobuf to json is just a matter of unmarshalling
    if ns.is_protobuf(in_packet):
        obj = ns.unmarshall(in_packet)
        if ns.is_ack(obj, command_type=cmd.Leds):
            mask = obj.sts
            obj = cmd.Leds()
            obj.set_status(mask)
        return obj
    else:
        LOG.debug("to_json - |%r| > /dev/null", in_packet)
        # do not send the message
        return None


@click.command()
@click.option('--serial/--no-serial', default = True, help = 'serial simulator')
def main(serial):
    """Configure the junction endpoints
    """

    web_page = ns.WSLine(port=3000)
    if serial:
        # the real board
        board = ns.SerialLine()
    else:
        # a software simulator or a tcp connected cc3200 board
        board = ns.TcpLine(port=3001)

    script = ns.TcpLine(port=3002)    # scripting endpoint, eg provisioning

    # ns.junction.route(src=web_page, dst=board, src_to_dst=to_protobuf, dst_to_src=to_json)
    ns.junction.route(src=web_page, dst=board,
                      src_to_dst=to_protobuf, dst_to_src=to_json)

    ns.junction.route(src=board, dst=script)

    ns.junction.run()

# pylint: disable=E1120
if __name__ == "__main__":
    main()
