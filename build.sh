#!/bin/bash

set -e

gcc -Wall utils.c ffosm.c main.c -I$PWD/ffhttpd/include -L$PWD/ffhttpd/lib -lffhttpd -lsqlite3 -lpthread -o ffosm
