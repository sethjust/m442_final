#!/usr/bin/env python

import sys,socket
import random,string

class RecvError (Exception):
  pass

def recv(s):
  d = ""
  while (1):
    c = s.recv(1)
    if ord(c) == 0:
      break;
    d = d+c

  length = int(d[0:4], 16)
  s = int(d[4:6], 16)

  if ( length != len(d[6:]) | s != csum(d[6:]) ):
    raise RecvError

  return d[6:]

def csum(msg):
 return reduce(lambda x,y: (x+y)%256, [ord(msg[i]) for i in range(0,len(msg))])

def send_msg(dest, msg):
  header = "%04X%02X"%(len(msg),csum(msg))
  dest.send(header+msg+"\0")
  return recv(dest)

def rname(len=8):
  return ''.join(random.choice(string.ascii_lowercase) for x in range(len))

HOST = "localhost"
PORT = int(sys.argv[1]) if len(sys.argv)>1 else 11111

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((HOST, PORT));

for i in range(16):
  print send_msg(s, "ADD:"+rname(2**i if (i<15) else 2**i-4))

print send_msg(s, "STOP")
