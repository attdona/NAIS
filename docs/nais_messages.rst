Protobuf messages
=================

In the following sections are described the built in protobuf messages supported
by NAIS.


Command
-------

.. code-block:: protobuf

  message Command {
    required int32 id = 1;
    optional int32 seq = 2;
    repeated string svals = 3;
    repeated int32 ivals = 4;
  }

A Command is a message that performs some action on the board.

``id`` specifies the command.

There could be some predefined commands implemented with a default
behaviour, for example the cc3200 board implements (item number is the command ``id``):

  (1) REBOOT
  (2) TOGGLE_WIFI_MODE
  (3) FACTORY_RESET
  (4) OTA
  (5) GET_CONFIG

The ``id`` values in the set [1,5] are reserved (for cc3200 board)
to these default commands: a custom command may use a ``id``
starting from 6.

``seq`` is a sequence number garanteed to be unique for each command
message not yet acknowledged (see below).

A command returns an acknowldgement message reporting
the execution status.

.. code-block:: protobuf

  message Ack {
    required int32 id = 1;
    optional int32 seq = 2;
    optional int32 status = 3;
  }

The pair (``id``, ``seq``) identifies a command instance and it is neeeded
for matching an Ack message with the correspondig Command message.

``status`` values are used defined but the suggested rule is to
set ``status`` to 0 for a success command and all other values
for error reporting or abnormal execution.


Event
-----

An ``Event`` is a message originating by an endpoint, typically a board.

``id`` identifies the event type, for example a `Fire Alarm` has id==10 and
a `Movement Detection` event has id==12.

``svals``, ``fvals`` and ``ivals`` are optional fields used for conveying 
specific informations related to the event.

.. code-block:: protobuf

    message Event {
        required int32  id = 1;
        repeated string svals = 2;
        repeated float  fvals = 3;
        repeated sint32 ivals = 4;
    }


Profile
-------

.. code-block:: protobuf

    message Profile {
        required string uid = 1;
        required string pwd  = 2;
    }

A ``Profile`` message embeds a user id and password.


Secret
------

.. code-block:: protobuf

    message Secret {
        required string key = 1;
    }

A ``Secret`` is a message that can be used to transfer a super secret value.


Config
------

.. code-block:: protobuf

    message Config {
        required string network = 1;
        required string board   = 2;
        required string host = 3;
        optional int32  port = 4;
        optional int32  alive_period = 5;
        optional bool   secure = 6;
    }

``Config`` message is used for communicates a well-know identity and 
network configuration parameters to a board/endpoint.

The pair (``board``, ``network``) is a unique identifier.

When the board/endpoint support mqtt and the mqtt client is enabled:

 * ``network/board`` is the subscribe topic
 * ``hb/network/board`` is the publish topic

``host`` is the dns or ip address of a tcp server or a mqtt broker.

``port`` is the mqtt broker/server remote tcp port.

``alive_period`` is specific to mqtt protocol and it is the time interval
between PINGREQ packets.

``secure`` define the type of connection, plain or encrypted.