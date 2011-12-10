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

  # we now use the cloud for an embarassingly parallel computation

  # Set size of render in rows and columns
  s_width = 80
  s_height = 24

  width = 3

  x = -1
  y = 0

  # Ensure values are floats
  width = float(width)
  x = float(x)
  y = float(y)

  # Caluculate width so that view is square
  height = s_height*(width/s_width)*2

  # Calculate coordinate offsets
  x_off = (width/2)-x
  y_off = (height/2)-y

  code = '''def symbol(iter, max_iter):
#  gradient = ['.', ':', 'o', 'O', '8', '@']
  gradient = ['\033[95m.\033[0m','\033[95m:\033[0m','\033[95mo\033[0m','\033[95mO\033[0m','\033[95m8\033[0m','\033[95m@\033[0m','\033[94m.\033[0m','\033[94m:\033[0m','\033[94mo\033[0m','\033[94mO\033[0m','\033[94m8\033[0m','\033[94m@\033[0m','\033[92m.\033[0m','\033[92m:\033[0m','\033[92mo\033[0m','\033[92mO\033[0m','\033[92m8\033[0m','\033[92m@\033[0m','\033[93m.\033[0m','\033[93m:\033[0m','\033[93mo\033[0m','\033[93mO\033[0m','\033[93m8\033[0m','\033[93m@\033[0m','\033[91m.\033[0m','\033[91m:\033[0m','\033[91mo\033[0m','\033[91mO\033[0m','\033[91m8\033[0m','\033[91m@\033[0m']
  
  return gradient[int(iter*(len(gradient)-1)/max_iter)]

# Calculates whether a number diverges after max_iter iterations
def mandel(a,max_iter):
	z = complex(0)
	iter = 0
	while iter < max_iter:
		iter+=1
		z = z**2 + a
		if abs(z) >= 2: break
	return symbol(iter,max_iter)

f=open('/net/pts', 'r')
l=[]
for pt in f.readlines():
  s = pt.strip().split(',')
  l.append(mandel(complex(float(s[0]),float(s[1])), 150))
print ''.join(l)
'''

  # Initialize a list of outputs
  l = []
  # Outer loop iterates over rows
  for i in range(1, s_height):
    # Set imaginary value based on the row's number
    comp = -1*(((i/float(s_height))*height)-y_off)
    
    pts = ''
    # Inner loop iterates across the columns of the row
    for j in range(1, s_width):
      # Set the real value based on the column's number
      real = (((j/float(s_width))*width)-x_off)
      pts = pts+str(real)+','+str(comp)+'\n'
    	
    f = s.add_string("pts", pts)
    j, k = s.add_job("mandel"+str(i), code, "mandout"+str(i), [f])
    l.append(k)
  
  import os
  os.system("./runner.py " + HOST + " " + str(PORT))

  for f in l:
    print f.get().strip()
