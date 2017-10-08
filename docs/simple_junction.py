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
            a protobuf object or None if unable to transform
            the input packet.
            If return None the packet is silently discarded


    """
    LOG.debug("to_protobuf - input: %s, msg: %s",
              type(in_packet), in_packet)

    obj = json.loads(in_packet.decode())

    packet = None

    # check if json packet is a led command
    if 'leds' in obj:
        if obj['leds'] == 'get':
            packet = ns.marshall(cmd.Leds())
        elif obj['leds'] == 'set':
            print()
            packet = ns.marshall(
                cmd.Leds(obj['red'] if 'red' in obj else None,
                         obj['green'] if 'green' in obj else None,
                         obj['yellow'] if 'yellow' in obj else None))
    else:
        LOG.info("to_protobuf - unable to convert message %s",
                 in_packet)
    return packet


def to_json(in_packet):
    """Convert a protobuf into a json message
    """
    LOG.debug("to_json - input: %s, msg: %s", type(in_packet),
              in_packet)

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
@click.option('--serial/--no-serial', default=True,
              help='serial simulator')
def main(serial):

    web_page = ns.WSLine(port=3000)
    if (serial):
        # the real board
        board = ns.SerialLine()
    else:
        # a software simulator
        board = ns.TcpLine(port=2001)

    ns.junction.route(src=web_page, dst=board,
                      src_to_dst=to_protobuf, dst_to_src=to_json)

    ns.junction.run()


if __name__ == "__main__":
    main()
