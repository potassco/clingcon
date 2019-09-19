#!/usr/bin/env python

from subprocess import call

executable = "build/bin/testclingcon"

num = call(executable, shell=True)
rc = 0
if num==0:
    rc = 1

for i in range(0,num):
    rc_call = call("{} {}".format(executable,i), shell=True)
    if rc_call != 0:
        rc = 1
        print("Failed: '{}' with error code {}".format(i, rc_call))
if rc == 0:
    print("Success: {} tests passed".format(num))

exit(ret)

calls = [("clingcon examples/bignumbers.lp 99 --outf=3", 99),
         ("clingcon examples/negative.lp 99 --outf=3", 4),
         ("clingcon examples/sendmoney.lp 99 --outf=3", 1),
         ("clingcon examples/sendmoremoney.lp 99 --outf=3", 1),
         ("clingcon examples/unary.lp 99 --outf=3", 0),
         ("clingcon examples/nqueens.lp -c n=7 99 --outf=3", 40)]
for i, rc_expected in calls:
    rc_call = call('{} | grep -c "Answer" | read -r models && exit $models'.format(i), shell=True)
    if rc_call != rc_expected:
        rc = 1
        print("Failed: '{}' with error code {}".format(i, rc_call))
if rc == 0:
    print("Success: {} tests passed".format(len(calls)))

exit(rc)
