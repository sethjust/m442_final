#!/usr/bin/env python

import sys,socket
import time

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("cerberus.sethjust.com", 26411));
print s.send("test\0")
time.sleep(.5)
print s.send("STOP\0")
