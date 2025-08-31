#!/bin/sh
# make and send to the duo
make && scp -O fbdev_test milkv-duo:
