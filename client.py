#!/usr/bin/env python

import sys,socket
import random,string
import base64

class ComputeCloud:
  def __init__(self, host, port):
    self.host = host
    self.port = port

  def open(self):
    self.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    self.socket.connect((self.host, self.port))

  def close(self):
    if (self.send_msg("STOP") != "ACK"):
      raise self.ServerError
    self.socket.close()

  def send_msg(self, msg):
    if len("%x"%(len(msg)))>4:
      raise self.MsgLengthError
    header = "%04X%02X"%(len(msg),csum(msg))
    self.socket.send(header+msg+"\0")
    return recv(self.socket) #TODO: error handling

  #TODO: reuse connections if needed
  def call(self, msg):
    self.open()
    res = self.send_msg(msg)
    self.close()
    return res

  def add_string(self, name, string):
    if len(string)==0:
      raise self.MsgLengthError
    res = self.call("ADD:"+name+":"+base64.b64encode(string))
    if res[:3] == "ACK":
      return self.FileObject(self, res[4:], name)
    else: return None

  def add_file(self, name, path):
    f = open(path, 'r') #TODO: catch errors
    return self.add_string(name, f.read())

  def add_job(self, name, code, outname, infiles):
    inhashes = ''.join([":"+f.hash for f in infiles])
#    JADD:name:sourcebytes:outputname{:inputhash}* -> ACK:jobhash:outputhash -- add a job
    res = s.call("JADD:"+name+":"+base64.b64encode(code)+":"+outname+inhashes)
    if res[:3] == "ACK":
      return (self.FileObject(self, res[4:12], name), self.FileObject(self, res[13:], outname))
    else: raise self.FileNotReadyError

  class FileNotReadyError (Exception):
    pass
  class MsgLengthError (Exception):
    pass
  class ServerError (Exception):
    pass
  class RecvError (Exception):
    pass

  class FileObject:
    def __init__(self, cloud, key, name=''):
      self.cloud = cloud
      self.name = name

      if self.invalid(key):
        raise self.ParseError
      self.hash = key

    def __repr__(self):
      return "<FileObject with hash "+self.hash+">"

    def get(self):
      res = self.cloud.call("GET:"+self.hash)
      if res[:3] == "ACK":
        return base64.b64decode(res[4:])
      else: return None

    def invalid(self, key):
      for char in key:
        if not char in string.hexdigits:
          print key
          return True
      return False

    class ParseError (Exception):
      pass

def recv(s):
  d = ""
  while (1):
    c = s.recv(1)
    if len(c) == 0:
      return ''
    if ord(c) == 0:
      break;
    d = d+c

  length = int(d[0:4], 16)
  s = int(d[4:6], 16)

  if ( length != len(d[6:]) or s != csum(d[6:]) ):
    raise ComputeCloud.RecvError

  return d[6:]

def csum(msg):
 return reduce(lambda x,y: (x+y)%256, [ord(msg[i]) for i in range(0,len(msg))])

def rname(len=8):
  return ''.join(random.choice(string.ascii_lowercase+string.ascii_uppercase) for x in range(len))

if __name__ == '__main__':
  HOST = "localhost"
  PORT = 11111
  if len(sys.argv)==2:
    PORT = int(sys.argv[1])
  elif len(sys.argv)==3:
    HOST = sys.argv[1]
    PORT = int(sys.argv[2])

  s = ComputeCloud(HOST, PORT)
  f = s.add_string("test", "this is a test file")
  g = s.add_string("test2", "this is a test gile")
  code = '''print open('/net/test', 'r').read()
print open('/net/test2', 'r').read()'''

  j,o = s.add_job("testjob", code, "testout", [f,g])

  import os, time
  os.system("./runner.py " + HOST + " " + str(PORT))
  time.sleep(1)

  try:
    k,p = s.add_job("testjob211", 'print open("/net/testout", "r").read()', "testout2", [o])
    #name is funky to use initial determinism to ensure that jobs are executed in the right order for testing
    print "First job was done!"
  except s.FileNotReadyError:
    print "First job not done, presumably"

  os.system("./runner.py " + HOST + " " + str(PORT))

  print j.get()
  print o.get()
  print k.get()
  print p.get()
