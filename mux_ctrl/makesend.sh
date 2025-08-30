#!/bin/sh
# make and send to the duo
make && scp -O mux_ctrl milkv-duo:
