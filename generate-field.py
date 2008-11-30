#!/usr/bin/env python

from random import randint, uniform, seed
from sys import stdout, argv, exit

def smooth(nums, smoothness):
    res = []
    N = len(nums)
    for i in range(N):
        x = 0
        y = 0
        for j in range(N):
            w = smoothness**abs(i -j)
            x += w*nums[j]
            y += w
        res.append(x/y)
    return res

def gen_heights(width, min_height, max_height, smoothness):
    heights = smooth([ uniform(0, 1) for _ in range(width) ], smoothness)
    mn = min(heights)
    mx = max(heights)
    height_range = max_height - min_height + 1
    if mn == mx: return [ int((min_height + max_height)/2) for _ in heights ]
    return [ min_height + int((x - mn)*height_range/(mx - mn)) for x in heights ]

if len(argv) <> 2:
    print 'Usage: generate-case.py <seed>'
    exit(1)

seed(int(argv[1]))

field_width  = randint(5, 50)
field_height = randint(5, 50)
block_height = randint(0, field_height)
smoothness   = uniform(0.3, 0.9)
block_types  = randint(3, 10)

heights = gen_heights(field_width, field_height - block_height, field_height, smoothness)

print "field width:   %6d" % field_height
print "field height:  %6d" % field_width
print "block height:  %6d" % block_height
print "smoothness:    %6.3f" % smoothness
print "block types:   %6d" % block_types

fields = file('speelveld.txt', 'wt')
for r in range(field_height):
    print >>fields, ''.join('10'[r + 1 < heights[c]] for c in range(field_width))

columns = file('kolommen.txt', 'wt')
for c in range(field_width):
    cnt = randint(10, 200)
    print >>columns, ''.join([ str(randint(0, block_types - 1)) for _ in range(cnt) ])
