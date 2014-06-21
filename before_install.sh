#!/usr/bin/env bash
echo 'Installing Pebble SDK Dependencies...'
export PEBBLE_SDK=PebbleSDK-2.2

(
cd ~ 

# Get the Pebble SDK and toolchain
wget https://developer.getpebble.com/2/download/${PEBBLE_SDK}.tar.gz -O PebbleSDK.tar.gz
wget http://assets.getpebble.com.s3-website-us-east-1.amazonaws.com/sdk/arm-cs-tools-ubuntu-universal.tar.gz

# Build the Pebble directory
mkdir ~/pebble-dev

cd ~/pebble-dev

# Extract the SDK
tar -zxf ~/PebbleSDK.tar.gz
cd ~/pebble-dev/${PEBBLE_SDK}

# Extract the toolchain
tar -zxf ~/arm-cs-tools-ubuntu-universal.tar.gz

)
export PATH=~/pebble-dev/${PEBBLE_SDK}/bin:$PATH

