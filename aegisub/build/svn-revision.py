import os, re, datetime
now = datetime.datetime.now().strftime("%Y/%m/%d %H:%M:%S")
log = os.popen("\"C:\\Program Files (x86)\\Git\\bin\\git\" log").read()
status = os.popen("\"C:\\Program Files (x86)\\Git\\bin\\git\" status").read()
rev = re.search("git-svn-id: .*@(\d+)", log)
commit = re.search("commit ([0-9a-f]{40})", log)
commits_over_svn = log.count("commit", 0, log.find("git-svn-id")) - 1
branch = re.search("On branch ([A-z-]+)", status)
if branch:
    branch = branch.group(1)
else:
    branch = "(no branch)"

out = open("..\svn-revision.h", "wt")
for line in open("..\svn-revision-base.h"):
    print >>out, line.replace('$WCREV$', rev.group(1)).replace('$WCDATE$', now).replace('$WCMODS?true:false$', 'true').replace('$GITVERSIONSTR$', 'r%s+%d (%s)' % (rev.group(1), commits_over_svn, branch)),
