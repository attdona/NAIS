# NAIS

[![Build Status](https://travis-ci.org/attdona/NAIS.svg?branch=master)](https://travis-ci.org/attdona/NAIS)
[![Github All Releases](https://img.shields.io/github/downloads/attdona/NAIS/total.svg)]()

NAIS project started from an attempt to build a tool for multiplexing the
serial port and ended as a micro-router monster.

This is work in progress,
Feedbacks, ideas and contributions are welcome!

Please use github Issues for any inquiry or bug.

NAIS concepts are actually implemented in python language:
pynais is an asyncio based package that implements a micro-router like service
between:

 * tcp connected apps
 * websocket connected apps
 * boards using a serial line (USB)

Some documentation [here](http://nais.readthedocs.io)

## RIOT

The firmware examples will be built on top of [RIOT](http://riot-os.org/)

The burba directory contains the RIOT specific cpu support for [cc3200](http://www.ti.com/product/CC3200) and the [CC3200-LAUNCHXL](http://www.ti.com/tool/cc3200-launchxl) board.


