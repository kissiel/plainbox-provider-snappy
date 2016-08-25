#!/usr/bin/env python3
from unittest import getTestCaseNames

module_name = 'test_kernel_security'
test_class = 'KernelSecurityTest'

module = __import__(module_name)

BLACKLIST = [
    'test_000_make', # binaries are prebuilt, no need to check for make
    'test_010_proc_maps', # because we're not building binaries ad hoc
    'test_091_symlink_following_in_sticky_directories', # requires useradd
    'test_093_ptrace_restriction', # requires gdb
    'test_094_rare_net_autoload', # fails on snappy

]

for name in getTestCaseNames(getattr(module, test_class), 'test'):
    if name in BLACKLIST:
        continue
    print('name: {}'.format(name))
    print('pyobj: {}'.format('.'.join([module_name, test_class, name])))
    print()
