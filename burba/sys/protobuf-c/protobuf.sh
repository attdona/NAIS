#!/bin/sh

# create the c interface and implementation from .proto files
#

# @author Attilio Dona'

PB_SPEC_DIR=$2
PB_SRC_DIR=$1

protoc-c --c_out=$PB_SRC_DIR --proto_path=$(dirname $PB_SPEC_DIR) $PB_SPEC_DIR

