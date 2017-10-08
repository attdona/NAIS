.. NAIS documentation master file, created by
   sphinx-quickstart on Tue Jun 27 16:06:20 2017.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

NAIS
====

NAIS project originate from an attempt to build a tool for multiplexing the 
serial port between ascii characters (printf strings) and binary encoded structures
for conveying complex requests and responses.

NAIS concepts are actually implemented in python language:
pynais is an asyncio based package that implements a micro-router like service
between:

 * tcp connected apps
 * websocket connected apps
 * boards using a serial line (USB) 


NAIS current version targets `RIOT`_ based firmware 
running on a `cc3200 launchpad`_ development kit.      

NAIS is currently developed and tested on Linux.

.. toctree::
    :maxdepth: 2
    :caption: Contents:

    getting_started
    nais_protocol
    nais_messages
    provision_demo
    leds_demo




A simple example ...
--------------------

A board connected to a USB port and a websocket based UI running in a web
browser are linked together:

.. code-block:: python

    import pynais as ns 

    board = ns.SerialLine(device='/dev/ttyUSB0')
    web_page = ns.WSLine(port=3000)

    ns.junction.route(src=web_page, dst=board
                      src_to_dst = to_protobuf, dst_to_src = to_json)

    ns.junction.run()

``junction.route`` declares the bidirectional link between the board and 
the web page.

The argument ``src_to_dest`` names a callback function that implements the
custom transformation of messages originating from the ``web_page`` (the source)
and the ``board`` (the destination)::

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

The argument ``dst_to_src`` transforms the messages going from the ``board``
(the destination) to the ``wep_page`` (the source)::

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
  
These snippets ends the implementation of the micro-router (see below for the
complete source file).

Now a javascript web component may open a websocket client (port 3000) and
send the json formatted message:

.. code-block:: javascript

    {'leds':'set', 'red': 'on'}


The NAIS junction engine receive the json string, transform to a protobuf encoded
payload and send through the serial port to the cc3200 board.

The red led switches on and a protobuf encoded acknowledge message is send over
the UART port.

The protobuf payload is transformed to a custom json structure and the message 
is finally sent to the web component that get the string::

    {"yellow": "on", "green": "off", "red": "on"}


NAIS and Protocol Buffers
--------------------------

cc3200 firmware and NAIS junction supports the Google `Protocol Buffers`_
message encoding.

For example the Ack protobuf specification is:

.. code-block:: protobuf

    message Ack {
        required int32 id = 1;
        optional int32 seq = 2;
        optional int32 status = 3;
    }

``id`` field value is the request message id.

The bits of the ``status`` field reports the board leds status, 3 bits for red, green
and yellow led.  

Simple junction
---------------

The complete source for the simple junction::

    bash> python simple_junction.py

simple_junction.py:    

.. literalinclude:: simple_junction.py


References
----------

.. target-notes::

.. _`RIOT`: http://riot-os.org
.. _`cc3200 launchpad`: http://www.ti.com/tool/CC3200-LAUNCHXL
.. _`Protocol Buffers`: https://developers.google.com/protocol-buffers

.. Indices and tables
.. ==================

.. * :ref:`genindex`
.. * :ref:`modindex`
.. * :ref:`search`
