#!/bin/sh
# make and send to the duo
make && scp -O dspod_app milkv-duo:
