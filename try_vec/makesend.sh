#!/bin/sh
# make and send to the duo
make && scp -O try_vec milkv-duo:
