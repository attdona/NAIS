wifi provisioning demo
======================

After flashing the cc3200 launchpad a way of setting the wlan credentials
have to be accomplished for connecting to a wifi access point.

The following python script prompt the user the wlan identifier and the 
password and setup the board.

.. literalinclude:: ../recipes/cc3200/profile_board.py
    :linenos:

The script connect to the NAIS junction using tcp port 3002. It is the work
of junction to deliver the message using the serial port to the board.

Just for give a little taste of a NAIS junction use case, you could test
the frontend provisioning script attaching a virtual board at the junction
in case the hardware is not ready for integration.      