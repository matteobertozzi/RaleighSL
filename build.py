#!/usr/bin/env python
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

from multiprocessing import cpu_count
from contextlib import contextmanager
from threading import Thread
from itertools import chain
from Queue import Queue

import shutil
import string
import time
import sys
import os
import re

#cpu_count = lambda: 1
BASE_DIR = os.path.split(os.path.abspath(__file__))[0]
ZCL_COMPILER = os.path.join(BASE_DIR, 'rpc-compiler.py')

def execCommand(cmd, no_output=True):
  import subprocess
  try:
    process = subprocess.Popen(cmd.split(), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    output, outerr = process.communicate()
    if output: output = output.rstrip()
    if outerr: outerr = outerr.rstrip()
    retcode = process.poll()
    if not no_output:
      if outerr: print outerr
      if output: print output
  except Exception as e:
    return 1, str(e)
  return retcode, '\n'.join([output, outerr])

# =============================================================================
#  Job Executor
# =============================================================================
class JobExecutor:
  __shared_state = {}

  class Worker:
    def __init__(self, jobq):
      self.jobq = jobq
      self.reset()

    def reset(self):
      self.job_results = []
      self.job_failures = []

    def run(self):
      while True:
        job = self.jobq.get()
        try:
          result = job()
          self.job_results.append(result)
        except Exception as e:
          self.job_failures.append(e)
        finally:
          self.jobq.task_done()

  def __init__(self):
    self.__dict__ = self.__shared_state
    self._jobq = Queue()
    self._workers = [self.Worker(self._jobq) for _ in xrange(cpu_count())]
    for worker in self._workers:
      t = Thread(target=worker.run)
      t.setDaemon(True)
      t.start()

  def add_job(self, func, *args, **kwargs):
    self._jobq.put(lambda: func(*args, **kwargs))

  def wait_jobs(self, raise_failure=True):
    self._jobq.join()
    if not self._jobq.empty():
      raise Exception("NOT EMPTY")

    job_results = []
    job_failures = []
    for worker in self._workers:
      job_results.extend(worker.job_results)
      job_failures.extend(worker.job_failures)
      worker.reset()
    if raise_failure and len(job_failures) > 0:
      for e in job_failures:
        print e
      raise Exception('%d Job Failures' % len(job_failures))
    return sorted(job_results)

# =============================================================================
#  Output
# =============================================================================
NO_OUTPUT = False
def msg_write(*args):
  if not NO_OUTPUT:
    data = ' '.join([str(x) for x in args]) if len(args) > 0 else ''
    sys.stdout.write('%s\n' % data)

@contextmanager
def bench(format):
  st = time.time()
  try:
    yield
  finally:
    et = time.time()
    msg_write(format, (et - st))

# =============================================================================
#  I/O
# =============================================================================
def writeFile(filename, content):
  fd = open(filename, 'w')
  try:
    fd.write(content)
  finally:
    fd.close()

def _removeDirectory(path):
  if os.path.exists(path):
    for root, dirs, files in os.walk(path, topdown=False):
      for name in files:
        os.remove(os.path.join(root, name))
      for name in dirs:
        os.rmdir(os.path.join(root, name))
    os.removedirs(path)

def removeDirectoryContents(path):
  for root, dirs, files in os.walk(path, topdown=False):
    for name in dirs:
      _removeDirectory(os.path.join(root, name))

def filesWalk(regex, dir_src, func):
  for root, dirs, files in os.walk(dir_src, topdown=False):
    for name in files:
      if regex.match(name):
        func(os.path.join(root, name))

def cFilesWalk(dir_src, func):
  pattern = re.compile('[A-Za-z0-9_-]+\.c$')
  return filesWalk(pattern, dir_src, func)

# =============================================================================
#  Build System
# =============================================================================
def findCompiler():
  paths = os.getenv('PATH', '/usr/bin').split(':')
  compilers = ['clang', 'gcc', 'distcc']

  for compiler in compilers:
    for path in paths:
      compiler_path = os.path.join(path, compiler)
      if os.path.exists(compiler_path):
        return compiler_path

  raise Exception('Compiler Not Found!')

def ldLibraryPathUpdate(ldlibs):
  env_name = 'LD_LIBRARY_PATH'
  env = os.environ.get(env_name, '')
  env_libs = ':'.join([lib for lib in ldlibs if lib not in env])
  if env_libs:
    env = '%s:%s' % (env, env_libs) if env else env_libs
    os.environ[env_name] = env

def compileZclStructs(src_dir, out_dir, dump_error=True):
  def _compile(source):
    cmd = '%s %s %s' % (ZCL_COMPILER, out_dir, source)
    msg_write(' [CG]', source)
    exit_code, output = execCommand(cmd, no_output=False)
    if exit_code != 0:
      if dump_error:
        msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
      raise RuntimeError("Linking Failure!")

  pattern = re.compile('[A-Za-z0-9_-]+\.rpc$')
  filesWalk(pattern, src_dir, _compile)
  msg_write()

def runTool(tool, verbose=True):
  exit_code, output = execCommand(tool, no_output=not verbose)

  tool_output = []
  if verbose:
    tool_output.append(output)

  if exit_code != 0:
    tool_output.append(' [FAIL] %s exit code %d' % (tool, exit_code))
  else:
    tool_output.append(' [ OK ] %s' % tool)

  msg_write('\n'.join(tool_output))

def runTools(name, tools, verbose=True):
  if not tools:
    return
  msg_write('Run %s:' % name)
  msg_write('-' * 60)

  job_exec = JobExecutor()
  for tool in tools:
    job_exec.add_job(runTool, tool, verbose)
  job_exec.wait_jobs()

  msg_write()

def runTests(name, tests, verbose=False):
  if tests:
    tests = [t for t in tests if os.path.basename(t).startswith('test-')]
    runTools(name, tests, verbose)

class BuildOptions(object):
  def __init__(self):
    self.cc = findCompiler()
    self.ldlibs = set()
    self.cflags = set()
    self.defines = set()
    self.includes = set()
    self.pedantic = False
    self.no_output = False

  def setCompiler(self, cc):
    self.cc = cc

  def addCFlags(self, cflags):
    self.cflags |= set(cflags)

  def addDefines(self, defines):
    self.defines |= set(defines)

  def addIncludePaths(self, includes):
    self.includes |= set(includes)

  def addLdLibs(self, ldlibs):
    self.ldlibs |= set(ldlibs)

  def setPedantic(self, pedantic):
    self.pedantic = pedantic

  def addVersionInfo(self, name, version, buildnr, gitrev):
    v_maj, v_min, v_rev = (int(x) for x in version.split('.'))
    assert v_maj <= 0xff, "Maj Version %x" % v_maj
    assert v_min <= 0xff, "Min Version %x" % v_min
    assert v_rev <= 0xff, "Rev Version %x" % v_rev
    macro_name = name.replace('-', '_').upper()
    self.addDefines([
      '-D%s_NAME=\"%s\"' % (macro_name, name),
      '-D%s_VERSION=0x%02x%02x%02x' % (macro_name, v_maj, v_min, v_rev),
      '-D%s_VERSION_STR=\"%s\"' % (macro_name, version),
      '-D%s_BUILD_NR=\"%s\"' % (macro_name, buildnr),
      '-D%s_GIT_REV=\"%s\"' % (macro_name, gitrev),
    ])

  def clone(self):
    opts = BuildOptions()
    opts.setCompiler(self.cc)
    opts.addCFlags(self.cflags)
    opts.addDefines(self.defines)
    opts.addIncludePaths(self.includes)
    opts.addLdLibs(self.ldlibs)
    opts.setPedantic(self.pedantic)
    return opts

class Build(object):
  DEFAULT_BUILD_DIR = 'build'
  HEADER_TITLE = 'Building'

  def __init__(self, name, options=[]):
    def _setDefaultFunc(value, default_f):
      return value if value else default_f()

    def _setDefaultValue(value, default_v):
      return value if value else default_v

    self.name = name
    self._options = _setDefaultFunc(options, BuildOptions)
    self._makeBuildDirs(Build.DEFAULT_BUILD_DIR)
    self._print_header()

  def build(self, *args, **kwargs):
    self.cleanup()
    os.makedirs(self._dir_obj)
    os.makedirs(self._dir_lib)
    os.makedirs(self._dir_inc)
    return self._build(*args, **kwargs)

  def _print_header(self):
    if self.HEADER_TITLE:
      msg_write(self.HEADER_TITLE, self.name)
      msg_write('-' * 60)

  def _build(self):
    raise NotImplementedError

  def cleanup(self, full=True):
    if full:
      _removeDirectory(self._dir_out)
    else:
      _removeDirectory(self._dir_obj)

  def _makeBuildDirs(self, build_dir):
    self._dir_out = os.path.join(build_dir, self.name)
    self._dir_obj = os.path.join(self._dir_out, 'objs')
    self._dir_lib = os.path.join(self._dir_out, 'libs')
    self._dir_inc = os.path.join(self._dir_out, 'include')

  def updateLdPath(self):
    ldLibraryPathUpdate([self._dir_lib])

  def compileFile(self, filename, dump_error=True):
    obj_name, obj_path = self._objFilePath(filename)

    cmd = '%s -c %s %s %s %s %s -o %s' %                \
           (self._options.cc,                           \
            string.join(self._options.cflags, ' '),     \
            string.join(self._options.defines, ' '),    \
            string.join(self._options.includes, ' '),   \
            '-I%s' % self._dir_inc,                     \
            filename,                                   \
            obj_path)

    msg_write(' [CC]', filename)
    exit_code, output = execCommand(cmd, no_output=self._options.no_output)
    if exit_code != 0:
      if dump_error:
        msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
      raise RuntimeError("Compilation Failure! %s\n%s" % (filename, output))

    if self._options.pedantic and len(output) > 0:
      msg_write(output)

  def compileDirectories(self, dirs_src):
    job_exec = JobExecutor()
    compileFunc = lambda f: job_exec.add_job(self.compileFile, f)
    for src in dirs_src:
      cFilesWalk(src, compileFunc)
    results = job_exec.wait_jobs()
    return len(results)

  def linkFile(self, filename, dump_error=True):
    _, obj_path = self._objFilePath(filename)
    app_name, app_path = self._appFilePathFromObj(obj_path)

    cmd = '%s -o %s %s %s' %                            \
            (self._options.cc, app_path, obj_path,      \
             string.join(self._options.ldlibs, ' '))
    msg_write(' [LD]', app_name)
    exit_code, output = execCommand(cmd, no_output=False)
    if exit_code != 0:
      if dump_error:
        msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
      raise RuntimeError("Linking Failure!")

    return app_path

  def _objFilePath(self, filename):
    if filename.endswith('.o'):
      return os.path.basename(filename), filename
    objname = os.path.normpath(filename).replace('/', '_')
    objname = objname[:objname.rindex('.')] + '.o'
    objpath = os.path.join(self._dir_obj, objname)
    return objname, objpath

  def _appFilePath(self, filename):
    obj_path = self._objFilePath(filename)
    return self._appFilePathFromObj(obj_path)

  def _appFilePathFromObj(self, obj_path):
    app_name = obj_path[obj_path.rfind('_') + 1:-2]
    app_path = os.path.join(self._dir_out, app_name)
    return app_name, app_path

  @staticmethod
  def platformIsMac():
    return os.uname()[0] == 'Darwin'

  @staticmethod
  def platformIsLinux():
    return os.uname()[0] == 'Linux'

class BuildApp(Build):
  def __init__(self, name, src_dirs, **kwargs):
    super(BuildApp, self).__init__(name, **kwargs)
    self.src_dirs = src_dirs

  def _build(self):
    if not self.compileDirectories(self.src_dirs):
      return

    obj_list = os.listdir(self._dir_obj)
    obj_list = [os.path.join(self._dir_obj, f) for f in obj_list]
    app_path = os.path.join(self._dir_out, self.name)

    cmd = '%s -o %s %s %s' %                            \
            (self._options.cc, app_path,                \
             string.join(obj_list, ' '),                \
             string.join(self._options.ldlibs, ' '))

    msg_write(' [LD]', self.name)
    exit_code, output = execCommand(cmd, no_output=False)
    if exit_code != 0:
      msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
      sys.exit(1)
    msg_write()

class BuildMiniTools(Build):
  def __init__(self, name, src_dirs, **kwargs):
    super(BuildMiniTools, self).__init__(name, **kwargs)
    self.src_dirs = src_dirs

  def _build(self):
    if not self.compileDirectories(self.src_dirs):
      return []

    job_exec = JobExecutor()
    for obj_name in os.listdir(self._dir_obj):
      job_exec.add_job(self.linkFile, os.path.join(self._dir_obj, obj_name))
    tools = job_exec.wait_jobs()

    msg_write()
    return sorted(tools)

class BuildConfig(Build):
  HEADER_TITLE = 'Running Build.Config'

  def __init__(self, name, src_dirs, **kwargs):
    super(BuildConfig, self).__init__(name, **kwargs)
    self.src_dirs = src_dirs

  def _build(self, config_file, config_head, debug=False, dump_error=False):
    job_exec = JobExecutor()
    test_func = lambda f: job_exec.add_job(self._testApp, f, dump_error)
    for src_dir in self.src_dirs:
      cFilesWalk(src_dir, test_func)
    config = job_exec.wait_jobs(raise_failure=False)

    self._writeConfigFile(config_file, config_head, config, debug)

  def _testApp(self, filename, dump_error):
    try:
      self.compileFile(filename, dump_error=dump_error)
      self.linkFile(filename, dump_error=dump_error)
    except Exception:
      msg_write(' [!!]', filename)
      raise Exception('Config Test %s failed' % filename)

    obj_name, obj_path = self._objFilePath(filename)
    app_name, app_path = self._appFilePathFromObj(obj_path)

    ldLibraryPathUpdate([self._dir_lib])
    exit_code, output = execCommand(app_path, no_output=True)
    if exit_code != 0:
      msg_write(' [!!]', filename)
      raise Exception('Config Test %s failed' % app_name)

    return app_name

  def _writeConfigFile(self, config_file, config_head, config, debug):
    msg_write(' [WR] Write config', config_file)
    fd = open(config_file, 'w')
    fd.write('/* File autogenerated, do not edit */\n')
    fd.write('#ifndef _%s_BUILD_CONFIG_H_\n' % config_head)
    fd.write('#define _%s_BUILD_CONFIG_H_\n' % config_head)
    fd.write('\n')
    fd.write('/* C++ needs to know that types and declarations are C, not C++. */\n')
    fd.write('#ifdef __cplusplus\n')
    fd.write('  #define __%s_BEGIN_DECLS__         extern "C" {\n' % config_head)
    fd.write('  #define __%s_END_DECLS__           }\n' % config_head)
    fd.write('#else\n')
    fd.write('  #define __%s_BEGIN_DECLS__\n' % config_head)
    fd.write('  #define __%s_END_DECLS__\n' % config_head)
    fd.write('#endif\n')
    fd.write('\n')

    if debug:
      fd.write('/* Debugging Mode on! Print as much as you can! */\n')
      fd.write('#define __Z_DEBUG__      1\n')
      fd.write('\n')

    if len(config) > 0:
      fd.write("/* You've support for this things... */\n")

    for define in config:
      fd.write('#define %s_%s\n' % (config_head, define.upper().replace('-', '_')))

    fd.write('\n')
    fd.write('#endif /* !_%s_BUILD_CONFIG_H_ */\n' % config_head)
    fd.flush()
    fd.close()

    msg_write()

class BuildLibrary(Build):
  SKIP_HEADER_ENDS = ('_p.h', 'private.h')
  HEADER_TITLE = None

  def __init__(self, name, version, src_dirs, copy_dirs=None, **kwargs):
    super(BuildLibrary, self).__init__(name, **kwargs)
    self.version = version
    self.src_dirs = src_dirs
    self.copy_dirs = copy_dirs or []

  def moveBuild(self, dst):
    _removeDirectory(dst)
    os.makedirs(dst)
    os.rename(self._dir_inc, os.path.join(dst, 'include'))
    os.rename(self._dir_lib, os.path.join(dst, 'libs'))

  def _build(self):
    msg_write('Copy %s %s Library Headers' % (self.name, self.version))
    msg_write('-' * 60)
    self.copyHeaders()

    msg_write('Building %s %s Library' % (self.name, self.version))
    msg_write('-' * 60)

    if not self.compileDirectories(self.src_dirs):
      return

    obj_list = os.listdir(self._dir_obj)
    obj_list = [os.path.join(self._dir_obj, f) for f in obj_list]

    if not os.path.exists(self._dir_lib):
      os.makedirs(self._dir_lib)

    libversion_maj = self.version[:self.version.index('.')]
    lib_ext = 'dylib' if self.platformIsMac() else 'so'
    lib_name = 'lib%s.%s' % (self.name, lib_ext)
    lib_name_maj = 'lib%s.%s.%s' % (self.name, lib_ext, libversion_maj)
    lib_name_full = 'lib%s.%s.%s' % (self.name, lib_ext, self.version)
    lib_path = os.path.join(self._dir_lib, lib_name_full)

    if self.platformIsMac():
      cmd = '%s -dynamiclib -current_version %s -o %s %s %s' %        \
              (self._options.cc, self.version, lib_path,              \
               string.join(obj_list, ' '),                            \
               string.join(self._options.ldlibs, ' '))
    elif self.platformIsLinux():
      cmd = '%s -shared -Wl,-soname,%s -o %s %s %s' %                 \
              (self._options.cc, lib_name_maj, lib_path,              \
               string.join(obj_list, ' '),                            \
               string.join(self._options.ldlibs, ' '))
    else:
      raise RuntimeError("Unsupported Platform %s" % ' '.join(os.uname()))

    msg_write()
    msg_write(' [LD]', lib_name_full)
    exit_code, output = execCommand(cmd, no_output=False)
    if exit_code != 0:
      msg_write(' * Failed with Status %d\n * %s\n%s' % (exit_code, cmd, output))
      sys.exit(1)

    cwd = os.getcwd()
    os.chdir(self._dir_lib)
    for name in (lib_name, lib_name_maj):
      msg_write(' [LN]', name)
      execCommand('ln -s %s %s' % (lib_name_full, name), no_output=False)
    os.chdir(cwd)

    msg_write()

  def copyHeaders(self):
    self.copyHeadersFromTo(None, self.src_dirs)
    for hname, hdirs in self.copy_dirs:
      self.copyHeadersFromTo(hname, hdirs)
    msg_write()

  def copyHeadersFromTo(self, name, src_dirs):
    dir_dst = os.path.join(self._dir_inc, self.name)
    if name is not None:
      dir_dst = os.path.join(dir_dst, name)
    _removeDirectory(dir_dst)
    os.makedirs(dir_dst)

    for dir_src in src_dirs:
      for root, dirs, files in os.walk(dir_src, topdown=False):
        for name in files:
          if not name.endswith('.h'):
            continue

          for s in self.SKIP_HEADER_ENDS:
            if name.endswith(s):
                break
          else:
            src_path = os.path.join(root, name)
            dst_path = os.path.join(dir_dst, name)
            shutil.copyfile(src_path, dst_path)
            msg_write(' [CP]', dst_path)

# =============================================================================
#  Project
# =============================================================================
_ldlib = lambda name: '-L%s/%s/libs -l%s' % (Build.DEFAULT_BUILD_DIR, name, name)
_inclib = lambda name: '-I%s/%s/include' % (Build.DEFAULT_BUILD_DIR, name)

class Project(object):
  VERSION = None
  NAME = None

  def __init__(self, options):
    self.options = options

    DEFAULT_CFLAGS = ['-Wall', '-Wmissing-field-initializers', '-msse4.2']
    DEFAULT_RELEASE_CFLAGS = ['-O3']
    DEFAULT_DEBUG_CFLAGS = ['-g']
    DEFAULT_DEFINES = ['-D_GNU_SOURCE', '-D__USE_FILE_OFFSET64']
    DEFAULT_LDLIBS = ['-lpthread', '-lm']

    # Default Build Options
    default_opts = BuildOptions()
    self.default_opts = default_opts
    default_opts.addDefines(DEFAULT_DEFINES)
    default_opts.addLdLibs(DEFAULT_LDLIBS)
    default_opts.addCFlags(DEFAULT_CFLAGS)
    default_opts.setPedantic(options.pedantic)

    self.build_rev = self._gitRevision()
    self.build_nr = self._buildNumber()
    default_opts.addVersionInfo(self.NAME, self.VERSION, self.build_nr, self.build_rev)

    if options.compiler is not None:
      default_opts.setCompiler(options.compiler)

    if options.release:
      default_opts.addCFlags(DEFAULT_RELEASE_CFLAGS)
    else:
      default_opts.addCFlags(DEFAULT_DEBUG_CFLAGS)

    #default_opts.addCFlags(['-Wconversion'])

    if options.pedantic:
      default_opts.addCFlags(['-pedantic', '-Wignored-qualifiers', '-Wsign-compare',
          '-Wtype-limits', '-Wuninitialized', '-Winline', '-Wpacked', '-Wcast-align',
          '-Wconversion', '-Wuseless-cast', '-Wsign-conversion'])
    else:
      default_opts.addCFlags(['-Werror'])

    # Default Library Build Options
    default_lib_opts = default_opts.clone()
    self.default_lib_opts = default_lib_opts
    default_lib_opts.addCFlags(['-fPIC', '-fno-strict-aliasing'])

  def __repr__(self):
    return '%s %s (build-nr: %s git-rev: %s)' % (self.NAME, self.VERSION, self.build_nr, self.build_rev)

  def build_config(self):
    print "No build-config step for '%s'" % self.NAME

  def build_auto_generated(self):
    print "No build auto-generated step for '%s'" % self.NAME

  def setup_library(self):
    return None

  @classmethod
  def get_includes(self):
    return [_inclib(self.NAME)]

  @classmethod
  def get_ldlibs(self):
    return [_ldlib(self.NAME)]

  def build_tools(self):
    print "No tools step for '%s'" % self.NAME

  def build_tests(self):
    print "No tests step for '%s'" % self.NAME
    return None

  def _buildNumber(self, inc=1):
    mx = 'ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789'

    self._file_buildnr = os.path.join(Build.DEFAULT_BUILD_DIR, '.buildnr-%s' % self.NAME)
    try:
      os.makedirs(Build.DEFAULT_BUILD_DIR)
      build = int(file(self._file_buildnr, 'r').read()) + inc
    except:
      build = 0

    if inc > 0:
      file(self._file_buildnr, 'w').write('%d' % build)

    v_maj, v_min, v_rev = (int(x) for x in self.VERSION.split('.'))
    return '%d%s%04X' % (v_maj, mx[v_min], build)

  def _gitRevision(self):
    _, rev = execCommand('git rev-parse HEAD')
    return rev.strip()

# =============================================================================
#  Project Targets
# =============================================================================
class Zcl(Project):
  VERSION = '0.5.0'
  NAME = 'zcl'

  def build_config(self):
    build_opts = self.default_opts.clone()
    build_opts.no_output = True
    build_opts.addCFlags(['-Werror'])
    build = BuildConfig('zcl-config', ['build.config'], options=build_opts)
    build.build('src/zcl/config.h', 'Z',
                debug=not self.options.release and not self.options.no_assert,
                dump_error=self.options.verbose)
    build.cleanup()

  def setup_library(self):
    return BuildLibrary(self.NAME, self.VERSION,
                        ['src/zcl'],
                        options=self.default_lib_opts)

  def build_tests(self):
    build_opts = self.default_opts.clone()
    build_opts.addLdLibs(self.get_ldlibs())
    build_opts.addIncludePaths(self.get_includes())

    build = BuildMiniTools('zcl-test', ['tests/zcl'], options=build_opts)
    return build.build()

class R5L(Project):
  VERSION = '0.5.0'
  NAME = 'r5l'

  def setup_library(self):
    build_opts = self.default_lib_opts.clone()
    build_opts.addLdLibs(Zcl.get_ldlibs())
    build_opts.addIncludePaths(Zcl.get_includes())

    copy_dirs = [
                 ('devices', ['src/r5l/devices']),
                 ('space', ['src/r5l/space']),
                 #('key', ['src/r5l/plugins/key']),
                 ('objects', ['src/r5l/objects']),
                 #('oid', ['src/r5l/plugins/oid']),
                 ('semantics', ['src/r5l/semantics']),
                 #('format', ['src/r5l/plugins/format']),
                ]

    return BuildLibrary(self.NAME, self.VERSION,
              ['src/r5l/core'] + list(chain(*[x for _, x in copy_dirs])),
               copy_dirs=copy_dirs, options=build_opts)

  @classmethod
  def get_includes(self):
    return [_inclib(self.NAME)] + Zcl.get_includes()

  @classmethod
  def get_ldlibs(self):
    return [_ldlib(self.NAME)] + Zcl.get_ldlibs()

  def build_tools(self):
    build_opts = self.default_opts.clone()
    build_opts.addLdLibs(self.get_ldlibs())
    build_opts.addIncludePaths(self.get_includes())
    build = BuildMiniTools('r5l-tools', ['src/r5l/tools'], options=build_opts)
    tools = build.build()

  def build_tests(self):
    build_opts = self.default_opts.clone()
    build_opts.addLdLibs(self.get_ldlibs())
    build_opts.addIncludePaths(self.get_includes())

    build = BuildMiniTools('r5l-test', ['tests/r5l'], options=build_opts)
    return build.build()

class R5LServer(Project):
  VERSION = '0.5.0'
  NAME = 'r5l-server'

  def build_auto_generated(self):
    compileZclStructs('src/r5l-server/rpc',
                      'src/r5l-server/rpc/generated',
                      dump_error=self.options.verbose)

  def build_tools(self):
    build_opts = self.default_opts.clone()
    build_opts.addLdLibs(R5L.get_ldlibs())
    build_opts.addIncludePaths(R5L.get_includes())

    build = BuildApp(self.NAME, ['src/r5l-server/'], options=build_opts)
    if not self.options.xcode:
      build.build()

class R5LClient(Project):
  VERSION = '0.5.0'
  NAME = 'r5l-client'

  def setup_library(self):
    build_opts = self.default_lib_opts.clone()
    build_opts.addLdLibs(R5L.get_ldlibs())
    build_opts.addIncludePaths(Zcl.get_includes())

    return BuildLibrary(self.NAME, self.VERSION,
                        ['src/r5l-client/c-r5l'],
                        options=build_opts)

  @classmethod
  def get_includes(self):
    return [_inclib(self.NAME)] + Zcl.get_includes()

  @classmethod
  def get_ldlibs(self):
    return [_ldlib(self.NAME)] + Zcl.get_ldlibs()

  def build_tests(self):
    build_opts = self.default_opts.clone()
    build_opts.addLdLibs(self.get_ldlibs())
    build_opts.addIncludePaths(self.get_includes())

    build = BuildMiniTools('%s-test' % self.NAME, ['tests/r5l-client'], options=build_opts)
    return build.build()

def main(options):
  dependencies = [Zcl]
  if not options.no_r5l:
    dependencies.extend([R5L, R5LServer, R5LClient])

  for project in dependencies:
    target = project(options)
    print '=' * 105
    print ' Building Target: %s' % target
    print '=' * 105
    target.build_config()
    target.build_auto_generated()
    library = target.setup_library()
    if library is not None:
      library.copyHeaders()
      if not options.xcode:
        library.build()
        library.updateLdPath()
    if not options.xcode:
      target.build_tools()
      tests = target.build_tests()
      #runTests('%s Test' % target.NAME, tests, verbose=options.verbose)

def _parse_cmdline():
  try:
    from argparse import ArgumentParser
  except ImportError:
    from optparse import OptionParser
    class ArgumentParser(OptionParser):
      def add_argument(self, *args, **kwargs):
        return self.add_option(*args, **kwargs)

      def parse_args(self):
        options, args = OptionParser.parse_args(self)
        return options

  parser = ArgumentParser()
  parser.add_argument(dest='clean', nargs='?',
                      help='Clean the build directory and exit')

  parser.add_argument('-c', '--compiler', dest='compiler', action='store',
                      help='Compiler to use')
  parser.add_argument('-x', '--xcode', dest='xcode', action='store_true', default=False,
                      help="Use XCode to build everything (copy headers only)")
  parser.add_argument('-r', '--release', dest='release', action='store_true', default=False,
                      help="Use release flags during compilation")
  parser.add_argument('--no-assert', dest='no_assert', action='store_true', default=False,
                      help="Disable the asserts even in debug mode")
  parser.add_argument('-p', '--pedantic', dest='pedantic', action='store_true', default=False,
                      help="Issue all the warnings demanded by strict ISO C")
  parser.add_argument('-v', '--verbose', dest='verbose', action='store_true', default=False,
                      help="Show traceback infomation if something fails")
  parser.add_argument('--no-output', dest='no_output', action='store_true', default=False,
                      help='Do not print messages')
  parser.add_argument('--no-r5l', dest='no_r5l', action='store_true', default=False,
                      help='Do not build R5L')


  return parser.parse_args()

if __name__ == '__main__':
  options = _parse_cmdline()
  NO_OUTPUT = options.no_output

  if options.clean:
    removeDirectoryContents(Build.DEFAULT_BUILD_DIR)
    sys.exit(0)

  try:
    with bench('[T] Build Time'):
      main(options)
  except Exception as e:
    print e
    sys.exit(1)

  sys.exit(0)
