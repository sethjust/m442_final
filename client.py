#!/usr/bin/env python

import sys,socket
import time

def recv(s):
  d = ""
  while (1):
    c = s.recv(1)
    if ord(c) == 0:
      break;
    d = d+c
  return d

def send_msg(s, msg):
  s.send(msg+"\0")
  return recv(s)

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect(("cerberus.sethjust.com", 26411));

print send_msg(s, "test")

print send_msg(s, "STOP")
