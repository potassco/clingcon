#!/usr/bin/env python

from subprocess import *

num = call("./build/bin/testclingcon", shell=True)
rc = 0
if num==0:
    rc = 1

for i in range(0,num):
    rc_call = call("./build/bin/testclingcon {}".format(i), shell=True)
    if rc_call != 0:
        rc = 1
        print("Failed: '{}' with error code {}".format(i, rc_call))
if rc == 0:
    print("Success: {} tests passed".format(num))

calls = [("clingcon examples/bignumbers.lp 99", 99),
         ("clingcon examples/negative.lp 99", 4),
         ("clingcon examples/sendmoney.lp 99", 1),
         ("clingcon examples/sendmoremoney.lp 99", 1),
         ("clingcon examples/unary.lp 99", 0),
         ("clingcon examples/nqueens.lp -c n=7 99", 40)]
for i, rc_expected in calls:
    process = Popen(i.split(), stdout=PIPE,stderr=PIPE)
    (output, err) = process.communicate()
    exit_code = process.wait()
    rc_call = output.count("Answer")
    if rc_call != rc_expected:
        rc = 1
        print("Failed: '{}' with error code {}".format(i, rc_call))
if rc == 0:
    print("Success: {} tests passed".format(len(calls)))

exit(rc)
