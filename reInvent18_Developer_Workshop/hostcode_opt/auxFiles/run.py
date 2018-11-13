#!/usr/bin/env python

import re
import subprocess

def extract( fields, line ):
    m = re.match(r"\s*(.*):\s(\d+.\d+|\d+|true|false)", line)
    if m:
        fields[m.group(1)] = m.group(2);


out = open('results.csv', 'w')

out.write('"Bytes per Transfer", "FPGA Throughput"')
out.write("\n")

for i in range(8, 20):
    fields = {}
    buffersize = 1 << i
    print (" Running with argument %s transfers %s bytes" % (i, buffersize*512/8))
    run = subprocess.Popen(['./pass', '../xclbin/pass.hw.xilinx_aws-vu9p-f1-04261818_dynamic_5_0.awsxclbin', str(i)], stdout = subprocess.PIPE)
    for line in iter(run.stdout.readline, ''):
        extract ( fields , line.rstrip())
    s=""
    s += '%s, %s' % ( fields["Bytes per Transfer"], fields["FPGA Throughput"])
    out.write(s)
    out.write("\n")
    

