#!/usr/bin/env python
#
# This file is part of Cosmonium.
#
# Copyright (C) 2018-2019 Laurent Deru.
#
# Cosmonium is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Cosmonium is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Cosmonium.  If not, see <https://www.gnu.org/licenses/>.
#

from __future__ import print_function
from __future__ import absolute_import

from math import log
import sys
import os

if sys.platform == 'darwin':
    platform_bin = 'OSX_universal_MAC.bin'
elif sys.platform in ("win32", "cli"):
    platform_bin = 'Win32_PC.bin'
else:
    platform_bin = 'Linux_PC.bin'

tool_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'F-TexTools-2.0pre2', platform_bin)

source = None
dirname = None
filebase = None
extension = None
channels = None
width = None

PNG_LEVEL = 1
TILE_SIZE = 1024

ctx = """VirtualTexture
{
        ImageDirectory "."
        BaseSplit 0
        TileSize 1024
        TileType "png"
}"""

def log2(x):
    return log(x, 2)

def convert(source):
    print("Converting %s to %s.png" % (source, filebase))
    os.system("convert '%s' '%s/%s.png'" % (source, filebase, filebase))

def png2bin(source, level):
    print("Converting %s to %s-%d.bin" % (source, filebase, level))
    os.system("%s < '%s' > '%s/%s-%d.bin'" % (os.path.join(tool_path, 'png2bin'), source, filebase, filebase, level))

def tx2half(level):
    print("Splitting %s-%d.bin" % (filebase, level))
    level_width = TILE_SIZE * (1 << (level + 2))
    os.system("%s %d %d < '%s/%s-%d.bin' > '%s/%s-%d.bin'" % (os.path.join(tool_path, 'tx2half'), channels, level_width, filebase, filebase, level + 1, filebase, filebase, level))

def txtiles(level):
    print("Generating tiles")
    level_width = TILE_SIZE * (1 << (level + 1))
    os.system("cd '%s/level%d' && %s %d %d %d %d < '../%s-%d.bin'" % (filebase, level, os.path.join('..', '..', tool_path, 'txtiles'), channels, level_width, level, PNG_LEVEL, filebase, level))

def rmbin(level):
    print("Removing %s-%d.bin" % (filebase, level))
    os.unlink(os.path.join(filebase, "%s-%d.bin" % (filebase, level)))

if len(sys.argv) != 4:
    print("Invalid parameters")
    exit(1)

source = sys.argv[1]
channels = int(sys.argv[2])
width = int(sys.argv[3])
dirname = os.path.dirname(source)
filename = os.path.basename(source)
filebase, extension = os.path.splitext(filename)
extension = extension[1:]

nb_levels = int(log2(width // TILE_SIZE))
print("Number of levels @ %d : %d" % (TILE_SIZE, nb_levels))

if not os.path.exists(filebase):
    os.mkdir(filebase)

if extension.lower() != 'png':
    convert(source)
    png2bin(os.path.join(filebase, filebase + ".png"), nb_levels - 1)
else:
    png2bin(source, nb_levels - 1)

for level in reversed(range(nb_levels)):
    print("Generating level %d" % level)
    level_path = os.path.join(filebase, "level%d" % level)
    if not os.path.exists(level_path):
        os.mkdir(level_path)
    txtiles(level)
    if level > 0:
        tx2half(level - 1)
    rmbin(level)

if extension.lower() != 'png':
    print("Removing %s.png" % filebase)
    os.unlink(os.path.join(filebase, "%s.png" % (filebase)))

with file('%s/%s.ctx' % (filebase, filebase), 'w') as ctx_file:
    ctx_file.write(ctx)
