#!/usr/local/bin/python

# This is a rudamentary clone of PaletteKnife, designed to output the palette to af ile
# in binary format.

import sys
import re
import math
import os

def adjustGamma(orig, gamma):
    o = orig / 255.0;
    adj = math.pow( o, gamma);
    res = math.floor( adj * 255.0);
    if ((orig != 0) and (res == 0)):
        res = 1;
    return int(res);


infile = sys.argv[1]
outfile = os.path.splitext(infile)[0] + ".bin"

print "Processing file: " + infile

with open(infile) as f:
    content = f.read()
    output_bytes = []

    regex = re.compile('.*\(\s*([0-9]+), *([0-9]+), *([0-9]+)\)\s+([0-9.]+)')

    # RGBA Warning
    if content.find("rgba(") != -1:
        print("WARNING: TRANSPARENCY not supported.");

    count = 0
    for line in content.split('\n'):
        match = regex.match(line)

        if match:
            #print len(match)
            r = int(match.group(1))
            g = int(match.group(2))
            b = int(match.group(3))
            pct = float(match.group(4))
            ndx = int(math.floor( (pct * 255.0) / 100.0 ))
            
            output_bytes.append(ndx)
            output_bytes.append(adjustGamma(r, 2.6))
            output_bytes.append(adjustGamma(g, 2.2))
            output_bytes.append(adjustGamma(b, 2.5))

    f.close()

    newFileByteArray = bytearray(output_bytes)
    with open(outfile,'wb') as newFile:
        newFile.write(newFileByteArray)
        newFile.close()




