#!/usr/bin/env python3

# from 00123400-0000-0000-0100-000000000000
# produce uuid4_of(0x00, 0x12, 0x34, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00)
# lowercase

import re
import argparse as ap

parser = ap.ArgumentParser('uuid-repr2of')
parser.add_argument('repr')
a = parser.parse_args()

print('uuid4_of(' + re.sub(r'([\dA-Fa-f]{2})-?', r'0x\1, ', a.repr.lower())[:-2] + ')')

