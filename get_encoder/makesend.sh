#!/bin/sh
# make and send to the duo
make && scp -O get_encoder milkv-duo:
