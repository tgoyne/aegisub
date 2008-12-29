#!/usr/bin/env python

import re
import sys
import os
import shutil

is_bad_lib = re.compile(r'(/usr/local|/opt)').match
is_sys_lib = re.compile(r'(/usr|/System)').match
otool_libname_extract = re.compile(r'\s+(/.*?)[\(\s:]').search

def otool(cmdline):
	pipe = os.popen("otool " + cmdline, 'r')
	output = pipe.readlines()
	pipe.close()
	return output

def collectlibs(lib, masterlist, targetdir):
	liblist = otool("-L " + lib)
	locallist = []
	for l in liblist:
		lr = otool_libname_extract(l)
		if not lr: continue
		l = lr.group(1)
		if is_bad_lib(l):
			sys.exit("Linking with library in blacklisted location: " + l)
		if not is_sys_lib(l) and not l in masterlist:
			locallist.append(l)
			print " ...found ", l,
			shutil.copy(l, targetdir)
			print " ...copied to target"
	masterlist.extend(locallist)
	for l in locallist:
		collectlibs(l, masterlist, targetdir)

binname = sys.argv[1]
targetdir = os.path.dirname(binname)
print "Searching for libraries in ", binname, "..."
libs = [binname]
collectlibs(sys.argv[1], libs, targetdir)

print " ...done. Will now fix up library install names..."
in_tool_cmdline = "install_name_tool "
for lib in libs:
	libbase = os.path.basename(lib)
	in_tool_cmdline = in_tool_cmdline + ("-change %s @executable_path/%s " % (lib, libbase))
for lib in libs:
	libbase = os.path.basename(lib)
	os.system("%s -id @executable_path/%s '%s/%s'" % (in_tool_cmdline, libbase, targetdir, libbase))
	print libbase,
	sys.stdout.flush()
print
print "All done!"
