#!/bin/sh
gtimeout 10 ./tcpcbr 1;   sleep 5  # send at 1 MBps
gtimeout 10 ./tcpcbr 1.5; sleep 5  # send at 1.5 MBps
gtimeout 10 ./tcpcbr 2;   sleep 5  # send at 2 MBps
gtimeout 10 ./tcpcbr 3
