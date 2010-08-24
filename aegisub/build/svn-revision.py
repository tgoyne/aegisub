import os, re, datetime
now = datetime.datetime.now().strftime("%Y/%m/%d %H:%M:%S")
log = os.popen("git log").read()
status = os.popen("git status").read()
svnrev = re.search("git-svn-id: .*@(\d+)", log)
commits = len(os.popen("git rev-list HEAD").readlines())
branch = re.search("On branch ([A-z-]+)", status)
if branch:
    branch = branch.group(1)
else:
    branch = "(no branch)"

out = open("..\svn-revision.h", "wt")
for line in open("..\svn-revision-base.h"):
    print >>out, line.replace('$WCREV$', svnrev.group(1)).replace('$WCDATE$', now).replace('$WCMODS?true:false$', 'true').replace('$GITVERSIONSTR$', 'r%d (%s; SVN r%s)' % (commits, branch, svnrev.group(1))),
