#!/usr/bin/env python

import random
import math

random.seed(42)

NUM = 64

print("uniform vec3 randOffsets[%d] = vec3[%d](" % (NUM, NUM))

radii = [math.sqrt(r/float(NUM)) for r in range(NUM)]
random.shuffle(radii)

for f in range(NUM):
    angle = 2.0 * math.pi * f/float(NUM)
    r = radii[f]
    x = math.cos(angle) * r
    y = math.sin(angle) * r
    print("        vec3(%f, 0, %f)%s" % (x,y, "," if f < (NUM-1) else "" ))

print(");")
