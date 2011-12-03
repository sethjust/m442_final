#!/usr/bin/env python

import sys,socket
import random,string
import base64

class ComputeCloud:
  def __init__(self, host, port):
    self.host = host
    self.port = port
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

  def call(self, msg):
    self.socket.connect((self.host, self.port))
    res = send_msg(self.socket, msg)
    if (send_msg(self.socket, "STOP") != "ACK"): #TODO: reuse connections if needed
      raise ComputeCloud.ServerError
    self.socket.close()
    return res

  def add_string(self, name, string):
    res = self.call("ADD:"+name+":"+base64.b64encode(string))
    if res[:3] == "ACK":
      return ComputeCloud.FileObject(self, res[4:])
    else: return None

  def add_text_file(self, name, path):
    f = open(path, 'r')
    ls = f.readlines()
    return self.add_string(name, ''.join(ls))

  class ServerError (Exception):
    pass

  class FileObject:
    def __init__(self, cloud, key):
      self.cloud = cloud

      if (self.invalid(key)):
        print key
        raise ComputeCloud.FileObject.ParseError
      self.key = key

    def __repr__(self):
      return "<FileObject with hash "+self.key+">"

    def get(self):
      res = self.cloud.call("GET:"+self.key)
      if res[:3] == "ACK":
        return base64.b64decode(res[4:])
      else: return None

    def invalid(self, key):
      for char in key:
        if not char in string.hexdigits:
          return True
      return False

    class ParseError (Exception):
      pass

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

  if ( length != len(d[6:]) or s != csum(d[6:]) ):
    raise RecvError

  return d[6:]

def csum(msg):
 return reduce(lambda x,y: (x+y)%256, [ord(msg[i]) for i in range(0,len(msg))])

def send_msg(dest, msg):
  if len("%x"%(len(msg)))>4:
    return "NACK:msg too long"
  header = "%04X%02X"%(len(msg),csum(msg))
  dest.send(header+msg+"\0")
  return recv(dest) #TODO: error handling

def rname(len=8):
  return ''.join(random.choice(string.ascii_lowercase) for x in range(len))

if __name__ == '__main__':
  HOST = "localhost"
  PORT = int(sys.argv[1]) if len(sys.argv)>1 else 11111

  s = ComputeCloud(HOST, PORT)
  f = s.add_string("name", "this is a test")
  print f
  print f.get()


#  s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#  s.connect((HOST, PORT));
#
#  for i in range(3,17):
#  print send_msg(s, "ADD:"+rname(2**i-4))
#    print send_msg(s, "ADD:"+rname(10))
#
#print send_msg(s, "SADD:127.0.0.1:11112:0")
#print send_msg(s, "GETS")
#
#  print send_msg(s, "STOP")
#  s.close()
