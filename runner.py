#! /usr/bin/env python

# Adapted from http://readevalprint.github.com/blog/python-sandbox-with-pypy-part2.html

import sys, os
import stat, tempfile, shutil
from cStringIO import StringIO
import cStringIO

SANDBOX_BIN = os.path.abspath("./runner/pypy_src/pypy/translator/goal/pypy-c")
SANDBOX_DIR = os.path.abspath("./runner/pypy_src/")
sys.path.insert(0, SANDBOX_DIR)

# import pypy sandbox now that it's added to the path
#from pypy.translator.sandbox import pypy_interact

# But I have to re-write pypy_interact to support getting virtual files from
#   the cloud, so we have to do all the imports.
from pypy.translator.sandbox import autopath
from pypy.translator.sandbox.sandlib import SimpleIOSandboxedProc
from pypy.translator.sandbox.sandlib import VirtualizedSandboxedProc
from pypy.translator.sandbox.sandlib import log
from pypy.translator.sandbox.vfs import Dir, RealDir, File, RealFile
from pypy.tool.lib_pypy import LIB_ROOT
from pypy.rpython.module.ll_os_stat import s_StatResult

TIMEOUT = 5

tempfile.tempdir = '/tmp/'

###################
## START MY CODE ##
###################

import base64
from client import ComputeCloud, rname

# This class is inheirited by our sandbox code; it hijacks some calls, as well
# as patching an issue with the original program
class VirtualizedRunnerProc(VirtualizedSandboxedProc):
  def __init__(self, *args, **kwds):
    super(VirtualizedRunnerProc, self).__init__(*args, **kwds)

  def get_node(self, vpath):
    if vpath[:5] == '/net/': #network object; special handling
      print "getting file", vpath[5:]
      return File(self.job.get(vpath[5:]))
    dirnode, name = self.translate_path(vpath)
    if name:
      node = dirnode.join(name)
    else:
      node = dirnode
    log.vpath('%r => %r' % (vpath, node))
    return node

  def do_ll_os__ll_os_fstat(self, fd):
    # This call is missing from the other class, preventing us from using open
    f = self.get_file(fd)

    # For File (not RealFile) objects, f will be a StringIO, not a file, so
    # we'll need to cheat.
    if not isinstance(f, file):
      return self.virtual_root.join('tmp').join('file').stat()

    # Now we can do a regular stat
    dirnode = self.virtual_root
    components = [component for component in f.name.split('/')]
    for component in components:
      if component:
        dirnode = dirnode.join(component)
        if dirnode.kind != stat.S_IFDIR:
          raise OSError(errno.ENOTDIR, component)
    return dirnode.stat()
  do_ll_os__ll_os_fstat.resulttype = s_StatResult

#################
## END MY CODE ##
#################
# This class is from the readevalprint example, but the first parent class is
# one we write, so we can hijack some calls
class PyPySandboxedProc(VirtualizedRunnerProc, SimpleIOSandboxedProc): 
  debug = True
  argv0 = '/bin/pypy-c'
  virtual_cwd = '/tmp'
  virtual_env = {}
  virtual_console_isatty = True

  def __init__(self, job, executable, arguments, tmpdir=None):
    self.job = job
    self.executable = executable = os.path.abspath(executable)
    self.tmpdir = tmpdir
    super(PyPySandboxedProc, self).__init__( [self.argv0] + arguments,
      executable=executable)

  def build_virtual_root(self):
    # build a virtual file system:
    # * can access its own executable
    # * can access the pure Python libraries
    # * can access the temporary usession directory as /tmp
    exclude = ['.pyc', '.pyo']
    if self.tmpdir is None:
      tmpdirnode = Dir({})
    else:
      tmpdirnode = RealDir(self.tmpdir, exclude=exclude)
    libroot = str(LIB_ROOT)

    return Dir({
      'bin': Dir({
        'pypy-c': RealFile(self.executable),
        'lib-python': RealDir(os.path.join(libroot, 'lib-python'),
          exclude=exclude), 
        'lib_pypy': RealDir(os.path.join(libroot, 'lib_pypy'),
          exclude=exclude),
        }),
      'tmp': tmpdirnode,
#      'net': Dir({
#          'test': File("this is a test")
#        })
      })

# Code to interact with the readevalprint example class
def exec_sandbox(job):
#  try:
    tmpdir = tempfile.mkdtemp()

    tmp = open(os.path.join(tmpdir, "file"), 'w')
    tmp.write("this space intentionally left blank")
    tmp.close()

    sandproc = PyPySandboxedProc(
        job,
        SANDBOX_BIN,
        ['-c', job.code,'--timeout',str(TIMEOUT),],
        #['/tmp/script.py','--timeout',str(TIMEOUT),],
        tmpdir # this is dir we just made that will become /tmp in the sandbox
        )
    try:
      code_output = StringIO()
      code_log = StringIO()
      sandproc.interact(stdout=code_output, stderr=code_log)
      return code_output.getvalue(), code_log.getvalue()
#    except Exception, e:
#      sandproc.kill()
    finally:
      sandproc.kill()

    shutil.rmtree(tmpdir)
#  except Exception, e:
#    pass
#  return 'Error, could not evaluate', e

##############################################
## ComputeCloud Runner functions start here ##
##############################################

class Job(object):
  def __init__(self, cloud, code, outname, outsalt, inhashes):
    self.cloud = cloud
    self.code = code
    self.outname = outname
    self.outsalt = outsalt
    self.files = dict( [ (name, ComputeCloud.FileObject(self.cloud, key)) for (key, name) in [(inhashes[i], inhashes[i+1]) for i in range(0, len(inhashes), 2)] ] )

  def get(self, key):
    return self.files[key].get()

  def do(self):
    return exec_sandbox(self)

def get_job(cloud):
#    GETJ -> ACK:sourcebytes:outputname:outputsalt{:inputhash}*
  res = cloud.call("GETJ")
  res = res.split(':')
  if res[0]!='ACK':
    return None
  else: return Job(cloud, base64.b64decode(res[1]), res[2], res[3], res[4:])

if __name__ == "__main__":
  host = "localhost"
  port = 11111
  if len(sys.argv)==2:
    port = int(sys.argv[1])
  elif len(sys.argv)==3:
    host = sys.argv[1]
    port = int(sys.argv[2])

  print "Connecting to", host+":"+str(port)

  s = ComputeCloud(host, port)

  out, err = get_job(s).do()

  print '=' *10
  print 'OUTPUT\n%s' % out
  print '=' *10
  print 'ERRORS\n%s' % err
