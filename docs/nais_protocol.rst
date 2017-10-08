NAIS packet format
==================

========== ===== ====== ====== ==== ==== ======== =========
SYNC_START  TYPE  SLINE  DLINE  RSV  LEN  PAYLOAD  SYNC_END
========== ===== ====== ====== ==== ==== ======== =========
    1        1     1       1     1   M      N        1
========== ===== ====== ====== ==== ==== ======== =========

The second row contains the length in bytes of the field.
``M`` and ``N`` are variable values related by:

    ``N`` = value contained into the ``M`` bytes


Fields summary
--------------

+--------------+---------+-------------------------------------+
| Field        | Values  | Description                         | 
+==============+=========+=====================================+
| SYNC_START   | Ox1E    | Mark the start of a NAIS packet     |
+--------------+---------+-------------------------------------+
| TYPE         |         | Unique id. Map to a protobuf        |
|              |         | message type                        |
+--------------+---------+-------------------------------------+
| SLINE        |         | Not used by clients. Used by NAIS   |
|              |         | junction routing functions          |
+--------------+---------+-------------------------------------+
| DLINE        |         | Set by clients If the outgoing      |
|              |         | packet is a response of an ingoing  |
|              |         | packet . Used by NAIS junction for  |
|              |         | routing functions                   |
+--------------+---------+-------------------------------------+
| RSV          | Ox00    | Reserved for future uses            |
+--------------+---------+-------------------------------------+
| LEN          |         | PAYLOAD length. The MSB bit is a    |
|              |         | continuation bit if payload len     |
|              |         | exceeds 127 bytes                   |
+--------------+---------+-------------------------------------+
| PAYLOAD      |         | Encoded protobuf message            |
|              |         |                                     |
+--------------+---------+-------------------------------------+
| SYNC_END     | Ox17    | Mark the end of a NAIS packet       |
+--------------+---------+-------------------------------------+






