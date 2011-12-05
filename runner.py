#! /usr/bin/env python

# Adapted from http://readevalprint.github.com/blog/python-sandbox-with-pypy-part2.html

import sys, shutil, os, tempfile
from cStringIO import StringIO

TIMEOUT = 5

tempfile.tempdir = '/tmp/'

SANDBOX_BIN = os.path.abspath("./runner/pypy_src/pypy/translator/goal/pypy-c")
SANDBOX_DIR = os.path.abspath("./runner/pypy_src/")
sys.path.insert(0, SANDBOX_DIR)

# import pypy now that it's added to the path
from pypy.translator.sandbox import pypy_interact

def exec_sandbox(code):
    try:
        tmpdir = tempfile.mkdtemp()
        tmpscript = open(os.path.join(tmpdir, "script.py"),'w') # script.py is the name of the code passed in
        tmpscript.write(code)
        tmpscript.close()
        sandproc = pypy_interact.PyPySandboxedProc(
            SANDBOX_BIN,
            ['-c', code], # un comment this and comment the next line if you want to run code directly
#            ['/tmp/script.py','--timeout',str(TIMEOUT),],
           tmpdir # this is dir we jsut made that will become /tmp in the sandbox
        )
        try:
            code_output = StringIO()
            code_log = StringIO()
            sandproc.interact(stdout=code_output, stderr=code_log)
            return code_output.getvalue(), code_log.getvalue()
        except Exception, e:
            sandproc.kill()
        finally:
            sandproc.kill()

        shutil.rmtree(tmpdir)
    except Exception, e:
        pass
    return 'Error, could not evaluate', e

if __name__ == "__main__":
  from client import ComputeCloud, rname

  s = ComputeCloud("134.10.30.239", 11111)

  code = '''
print 'Hi there!'
'''

  f = s.add_string(rname(), code)
  print f

  out, err = exec_sandbox(f.get())
  print '=' *10
  print 'OUTPUT\n%s' % out
  print '=' *10
  print 'ERRORS\n%s' % err
