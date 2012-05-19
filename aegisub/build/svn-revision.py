#!/usr/bin/python

import datetime
import os
import re

now = datetime.datetime.now().strftime("%Y/%m/%d")
log = os.popen("git log --pretty=medium").read()
status = os.popen("git status").read()
svnrev = re.search("git-svn-id: .*@(\d+)", log).group(1)
#commits = len(os.popen("git rev-list HEAD").readlines())
commits_over_svn = log.count("commit ", 0, log.find("git-svn-id")) - 1
branch = re.search("On branch ([A-z-]+)", status)
if branch:
    branch = branch.group(1)
else:
    branch = "(no branch)"

basedir = os.path.dirname(os.path.realpath(__file__)) + "/"

try:
	old = open(basedir + "svn-revision.h").read()
except:
	old = ""

new = open(basedir + "svn-revision-base.h").read().replace('$WCREV$', svnrev).replace('$WCDATE$', now).replace('$GITVERSIONSTR$', 'r%s+%d (branch %s)' % (svnrev, commits_over_svn, branch))

if old != new:
    open(basedir + "svn-revision.h", "wt").write(new)

print svnrev
