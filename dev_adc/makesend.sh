#!/bin/sh
# make and send to the duo
make && scp -O dev_adc milkv-duo:
