#!/usr/bin/env python

import sys,socket
import random,string

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

def rname(len=8):
 return ''.join(random.choice(string.ascii_lowercase) for x in range(len))

HOST = "localhost"
PORT = int(sys.argv[1]) if len(sys.argv)>1 else 11111

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT));

for i in range(10):
  print send_msg(s, "ADD:"+rname())

print send_msg(s, "STOP")
