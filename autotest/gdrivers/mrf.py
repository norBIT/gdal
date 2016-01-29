#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test read/write functionality for MRF driver.
# Author:   Even Rouault, <even.rouault at spatialys.com>
#
###############################################################################
# Copyright (c) 2016, Even Rouault, <even.rouault at spatialys.com>
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
###############################################################################

import sys

sys.path.append( '../pymod' )

import gdaltest

init_list = [
    ('byte.tif', 1, 4672, None),
    ('byte.tif', 1, 4672, ['COMPRESS=DEFLATE']),
    ('byte.tif', 1, 4672, ['COMPRESS=NONE']),
    ('byte.tif', 1, 4672, ['COMPRESS=LERC']),
    ('int16.tif', 1, 4672, None),
    ('../../gcore/data/uint16.tif', 1, 4672, None),
    ('../../gcore/data/int32.tif', 1, 4672, ['COMPRESS=TIF']),
    ('../../gcore/data/uint32.tif', 1, 4672, ['COMPRESS=TIF']),
    ('../../gcore/data/float32.tif', 1, 4672, ['COMPRESS=TIF']),
    ('../../gcore/data/float64.tif', 1, 4672, ['COMPRESS=TIF']),
    ('../../gcore/data/utmsmall.tif', 1, 50054, None),
    ('small_world_pct.tif', 1, 14890, ['COMPRESS=PPNG']) ]

gdaltest_list = []

for item in init_list:
    options = []
    if item[3]:
        options = item[3]
    ut = gdaltest.GDALTest( 'MRF', item[0], item[1], item[2], options = options )
    if ut is None:
        print( 'MRF tests skipped' )
    gdaltest_list.append( (ut.testCreateCopy, item[0] + ' ' + str(options)) )

if __name__ == '__main__':

    gdaltest.setup_run( 'mrf' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()
