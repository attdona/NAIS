Getting Started
===============

Requirements
------------

python 3.5+
^^^^^^^^^^^

NAIS make use of async/await language constructs, so python 3.5+ is needed.

Depending on python packages installed on your machine you may need to install the venv package
if you plan to configure a virtual sandbox for python::

    sudo apt-get install python3-venv 

protobuf 3.3.0+
^^^^^^^^^^^^^^^

Install `protobuf <https://github.com/google/protobuf>`_ from source or pick up a pre-built binary distribution.

For example:


.. literalinclude:: ../install-protobuf.sh
    :language: sh

ser2net
-------

`ser2net`_ is used for serial communication::  

    sudo apt-get install ser2net
    sudo systemctl disable ser2net

`ser2net` service has to be disabled because start/stop of `ser2net` process is managed by NAIS junction app.     

Installation
------------


Get NAIS from github:

.. code-block:: shell
    :linenos:

    git clone https://github.com/attdona/NAIS
    cd NAIS
    . nais.env.sh   # do it if you want a dedicated virtualenv
    pip install wheel
    pip install .

If you feel like modifying NAIS package install it in development mode.
Run last step as::

    pip install -e .[dev]




.. _`ser2net`: http://ser2net.sourceforge.net/

