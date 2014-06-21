#!/bin/bash

echo Building pebble project
export PEBBLE_SDK=PebbleSDK-2.2
export PATH=~/pebble-dev/${PEBBLE_SDK}/bin:$PATH

pebble build
