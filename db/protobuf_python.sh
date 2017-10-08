#!/bin/sh

protoc -I=. --python_out=../pynais nais.proto
protoc -I=. --python_out=../pynais sbapi.proto
