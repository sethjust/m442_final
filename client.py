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

def send_msg(dest, msg):
  l = len(msg)
  header = "%04X"%l
  s = reduce(lambda x,y: (x+y)%256, [ord(msg[i])+1 for i in range(0,len(msg))])
  header += "%02X"%s
  dest.send(header+msg+"\0")
  return recv(dest)
[ord("test"[i]) for i in range(0,len("test"))]

def rname(len=8):
  return ''.join(random.choice(string.ascii_lowercase) for x in range(len))

HOST = "localhost"
PORT = int(sys.argv[1]) if len(sys.argv)>1 else 11111

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT));

for i in range(10):
  print send_msg(s, "ADD:"+rname())

print send_msg(s, "STOP")
