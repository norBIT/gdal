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

from osgeo import gdal

import gdaltest

init_list = [
    ('byte.tif', 1, 4672, None),
    ('byte.tif', 1, 4672, ['COMPRESS=DEFLATE']),
    ('byte.tif', 1, 4672, ['COMPRESS=NONE']),
    ('byte.tif', 1, 4672, ['COMPRESS=LERC']),
#    ('byte.tif', 1, 4672, ['COMPRESS=LERC', 'OPTIONS=V1:YES']),
    ('int16.tif', 1, 4672, None),
    ('int16.tif', 1, 4672, ['COMPRESS=LERC']),
#    ('int16.tif', 1, 4672, ['COMPRESS=LERC', 'OPTIONS=V1:YES']),
    ('../../gcore/data/uint16.tif', 1, 4672, None),
    ('../../gcore/data/uint16.tif', 1, 4672, ['COMPRESS=LERC']),
#    ('../../gcore/data/uint16.tif', 1, 4672, ['COMPRESS=LERC', 'OPTIONS=V1:YES']),
    ('../../gcore/data/int32.tif', 1, 4672, ['COMPRESS=TIF']),
    ('../../gcore/data/int32.tif', 1, 4672, ['COMPRESS=LERC']),
#    ('../../gcore/data/int32.tif', 1, 4672, ['COMPRESS=LERC', 'OPTIONS=V1:YES']),
    ('../../gcore/data/uint32.tif', 1, 4672, ['COMPRESS=TIF']),
    ('../../gcore/data/uint32.tif', 1, 4672, ['COMPRESS=LERC']),
#    ('../../gcore/data/uint32.tif', 1, 4672, ['COMPRESS=LERC', 'OPTIONS=V1:YES']),
    ('../../gcore/data/float32.tif', 1, 4672, ['COMPRESS=TIF']),
    ('../../gcore/data/float32.tif', 1, 4672, ['COMPRESS=LERC']),
#    ('../../gcore/data/float32.tif', 1, 4672, ['COMPRESS=LERC', 'OPTIONS=V1:YES']),
    ('../../gcore/data/float64.tif', 1, 4672, ['COMPRESS=TIF']),
    ('../../gcore/data/float64.tif', 1, 4672, ['COMPRESS=LERC']),
#    ('../../gcore/data/float64.tif', 1, 4672, ['COMPRESS=LERC', 'OPTIONS=V1:YES']),
    ('../../gcore/data/utmsmall.tif', 1, 50054, None),
    ('small_world_pct.tif', 1, 14890, ['COMPRESS=PPNG']),
    ('byte.tif', 1, [4672, [4652,4603]], ['COMPRESS=JPEG', 'QUALITY=99']),
    ('rgbsmall.tif', 1, [21212, [21137,21223]], ['COMPRESS=JPEG', 'QUALITY=99']),
]

gdaltest_list = []


class myTestCreateCopyWrapper:

    def __init__(self, ut):
        self.ut = ut

    def myTestCreateCopy(self):
        check_minmax = not 'COMPRESS=JPEG' in self.ut.options
        return self.ut.testCreateCopy(check_minmax = check_minmax)

for item in init_list:
    options = []
    if item[3]:
        options = item[3]
    chksum_param = item[2]
    if type(chksum_param) == type([]):
        chksum = chksum_param[0]
        chksum_after_reopening = chksum_param[1]
    else:
        chksum = chksum_param
        chksum_after_reopening = chksum_param
    ut = gdaltest.GDALTest( 'MRF', item[0], item[1], chksum, options = options, chksum_after_reopening = chksum_after_reopening )
    if ut is None:
        print( 'MRF tests skipped' )
    ut = myTestCreateCopyWrapper(ut)
    gdaltest_list.append( (ut.myTestCreateCopy, item[0] + ' ' + str(options)) )



def mrf_overviews():

    out_ds = gdal.Translate('/vsimem/out.mrf', 'data/utm.tif', format = 'MRF', width = 1024, height = 1024)
    out_ds.BuildOverviews('NEAR', [2])
    out_ds = None

    ds = gdal.Open('/vsimem/out.mrf')
    cs= ds.GetRasterBand(1).GetOverview(0).Checksum()
    if cs != 50235:
        gdaltest.post_reason('fail')
        return 'fail'
    ds = None

    gdal.GetDriverByName('MRF').Delete('/vsimem/out.mrf')

    return 'success'

gdaltest_list += [ mrf_overviews ]

if __name__ == '__main__':

    gdaltest.setup_run( 'mrf' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()
