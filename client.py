#!/usr/bin/env python

import sys,socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#print("connecting to %s %d"%(sys.argv[1], int(sys.argv[2])))
s.connect(("cerberus.sethjust.com", 26411));
print s.send("test")
