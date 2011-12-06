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

from client import ComputeCloud, rname

# This class is inheirited by our sandbox code; it hijacks some calls, as well
# as patching an issue with the original program
class VirtualizedRunnerProc(VirtualizedSandboxedProc):
  def __init__(self, *args, **kwds):
    super(VirtualizedRunnerProc, self).__init__(*args, **kwds)

  def get_node(self, vpath):
    if vpath[:5] == '/net/': #network object; special handling
      print "getting hash", vpath[5:]
      return File(ComputeCloud.FileObject(self.cloud, vpath[5:]).get())
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

    # For File (not RealFile) objects f will be a StringIO, not a file, so
    # we'll need to cheat.
    if not isinstance(f, file):
      return self.virtual_root.join('tmp').join('script.py').stat()

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

  def __init__(self, cloud, executable, arguments, tmpdir=None):
    self.cloud = cloud
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
      'net': Dir({
          'test': File("this is a test")
        })
      })

# Code to interact with the readevalprint example class
def exec_sandbox(cloud, code):
#  try:
    tmpdir = tempfile.mkdtemp()

    tmpscript = open(os.path.join(tmpdir, "script.py"), 'w')
    tmpscript.write(code)
    tmpscript.close()

    sandproc = PyPySandboxedProc(
        cloud,
        SANDBOX_BIN,
        #['-c', code,'--timeout',str(TIMEOUT),],
        ['/tmp/script.py','--timeout',str(TIMEOUT),],
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
if __name__ == "__main__":
  host = "localhost"
  port = 1111
  if len(sys.argv)==2:
    port = int(sys.argv[1])
  elif len(sys.argv)==3:
    host = sys.argv[1]
    port = int(sys.argv[2])

#  s = ComputeCloud("134.10.30.239", 11111)
  s = ComputeCloud("localhost", 11111)

  code = '''
import os
print open("/net/EBEE231F", 'r').read()
'''

  f = s.add_string(rname(), code)
  out, err = exec_sandbox(s, f.get())

  print '=' *10
  print 'OUTPUT\n%s' % out
  print '=' *10
  print 'ERRORS\n%s' % err
