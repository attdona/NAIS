import asyncio
import socket
import sys
import click
import logging
import pynais as ns
import commands as cmd

#from pynais import nais
#from pynais.profile import Leds

logging.basicConfig(
    level=logging.DEBUG,
    format='%(levelname)s:%(name)s:%(lineno)s: %(message)s',
    stream=sys.stderr,
)
log = logging.getLogger('main')

# simulate a periodic log message
def log_message(writer):
    writer.write(b"log message from board\n")
    loop = asyncio.get_event_loop()
    now = loop.time()
    loop.call_at(now+5, log_message, writer)


led_status = cmd.Leds.RED

def command_handler(message):
    global led_status

    if type(message) == cmd.Leds:

        if message.red=='on':
            led_status |= cmd.Leds.RED
        elif message.red=='off':
            led_status &= ~cmd.Leds.RED

        if message.yellow=='on':
            led_status |= cmd.Leds.YELLOW
        elif message.yellow=='off':
            led_status &= ~cmd.Leds.YELLOW

        # reflect source on destination 
        message.dline = message.sline

        return ns.ack(message, sts=led_status)

    return ns.ack(message, sts=0)

    #return None

async def cc3200_client(port):
    reader, writer = await asyncio.open_connection('0.0.0.0', port)
    await cc3200_server(ns.Channel(reader, writer))

async def cc3200_server(channel):
    await ns.msg_send(channel, "Welcome to RIOT python simulator\n")

    loop = asyncio.get_event_loop()
    now = loop.time()
    #loop.call_at(now+1, log_message, channel)

    while True:
        data = await ns.msg_receive(channel)
        if not data:
            break

        if ns.is_protobuf(data):
            obj = ns.entity_from(data)
            response = command_handler(obj)
            if response:
                await ns.msg_send(channel, "sending response {}\n".format(response))
                await ns.proto_send(channel, response)
            else:
                await ns.msg_send(channel, b"PROTOBUF detected, You are a good boy!\n")
        else:
            await ns.msg_send(channel, b"ASCII line detected, You are nasty!!\n")

    writer.close()

@click.command()
@click.option('--port', default=2001, help='junction port')
@click.option('--role', default='server', help='start as server or client')
def main(port, role):
    loop = asyncio.get_event_loop()

    if role == 'server':
        factory = asyncio.start_server(cc3200_server, 'localhost', port, family=socket.AF_INET)
        server =  loop.run_until_complete(factory)

        # Enter the event loop permanently to handle all connections.
        try:
            loop.run_forever()
        except KeyboardInterrupt:
            pass
        finally:
            log.debug('closing server')
            server.close()
            loop.run_until_complete(server.wait_closed())
            log.debug('closing event loop')
            loop.close()
    else:
        loop.run_until_complete(cc3200_client(port))
        loop.close()


if __name__ == '__main__':
    main()
