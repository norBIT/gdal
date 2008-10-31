#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test Overview Support (mostly a GeoTIFF issue).
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2004, Frank Warmerdam <warmerdam@pobox.com>
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

import os
import sys
import gdal
import shutil
import string

sys.path.append( '../pymod' )

import gdaltest


###############################################################################
# Check the overviews

def tiff_ovr_check(src_ds):
    for i in (1, 2, 3):
        if src_ds.GetRasterBand(i).GetOverviewCount() != 2:
            gdaltest.post_reason( 'overviews missing' )
            return 'fail'

        ovr_band = src_ds.GetRasterBand(i).GetOverview(0)
        if ovr_band.XSize != 10 or ovr_band.YSize != 10:
            msg = 'overview wrong size: band %d, overview 0, size = %d * %d,' % (i, ovr_band.XSize, ovr_band.YSize)
            gdaltest.post_reason( msg )
            return 'fail'

        if ovr_band.Checksum() != 1087:
            msg = 'overview wrong checkum: band %d, overview 0, checksum = %d,' % (i, ovr_band.Checksum())
            gdaltest.post_reason( msg )
            return 'fail'

        ovr_band = src_ds.GetRasterBand(i).GetOverview(1)
        if ovr_band.XSize != 5 or ovr_band.YSize != 5:
            msg = 'overview wrong size: band %d, overview 1, size = %d * %d,' % (i, ovr_band.XSize, ovr_band.YSize)
            gdaltest.post_reason( msg )
            return 'fail'

        if ovr_band.Checksum() != 328:
            msg = 'overview wrong checkum: band %d, overview 1, checksum = %d,' % (i, ovr_band.Checksum())
            gdaltest.post_reason( msg )
            return 'fail'
    return 'success'

###############################################################################
# Create a 3 band floating point GeoTIFF file so we can build overviews on it
# later.  Build overviews on it. 

def tiff_ovr_1():

    gdaltest.tiff_drv = gdal.GetDriverByName( 'GTiff' )

    src_ds = gdal.Open('data/mfloat32.vrt')

    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    gdaltest.tiff_drv.CreateCopy( 'tmp/mfloat32.tif', src_ds,
                                  options = ['INTERLEAVE=PIXEL'] )
    src_ds = None

    ds = gdal.Open( 'tmp/mfloat32.tif' )
    
    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    err = ds.BuildOverviews( overviewlist = [2, 4] )

    if err != 0:
        gdaltest.post_reason('BuildOverviews reports an error' )
        return 'fail'

    ret = tiff_ovr_check(ds)

    ds = None

    return ret


###############################################################################
# Open file and verify some characteristics of the overviews. 

def tiff_ovr_2():

    src_ds = gdal.Open( 'tmp/mfloat32.tif' )

    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'


    ret = tiff_ovr_check(src_ds)

    src_ds = None

    return ret

###############################################################################
# Open target file in update mode, and create internal overviews.

def tiff_ovr_3():

    os.unlink( 'tmp/mfloat32.tif.ovr' )

    src_ds = gdal.Open( 'tmp/mfloat32.tif', gdal.GA_Update )

    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    err = src_ds.BuildOverviews( overviewlist = [2, 4] )
    if err != 0:
        gdaltest.post_reason('BuildOverviews reports an error' )
        return 'fail'

    ret = tiff_ovr_check(src_ds)

    src_ds = None

    return ret

###############################################################################
# Re-open target file and check overviews

def tiff_ovr_3bis():
    return tiff_ovr_2()

###############################################################################
# Test generation 

def tiff_ovr_4():

    shutil.copyfile( 'data/oddsize_1bit2b.tif', 'tmp/ovr4.tif' )

    wrk_ds = gdal.Open('tmp/ovr4.tif',gdal.GA_Update)
    
    if wrk_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    wrk_ds.BuildOverviews( 'AVERAGE_BIT2GRAYSCALE', overviewlist = [2,4] )
    wrk_ds = None

    wrk_ds = gdal.Open('tmp/ovr4.tif')

    ovband = wrk_ds.GetRasterBand(1).GetOverview(1)
    md = ovband.GetMetadata()
    if not md.has_key('RESAMPLING') \
       or md['RESAMPLING'] != 'AVERAGE_BIT2GRAYSCALE':
        gdaltest.post_reason( 'Did not get expected RESAMPLING metadata.' )
        return 'fail'

    # compute average value of overview band image data.
    ovimage = ovband.ReadRaster(0,0,ovband.XSize,ovband.YSize)

    pix_count = ovband.XSize * ovband.YSize
    sum = 0.0
    for i in range(pix_count):
        sum = sum + ord(ovimage[i])
    average = sum / pix_count
    exp_average = 154.8144
    if abs(average - exp_average) > 0.1:
        print average
        gdaltest.post_reason( 'got wrong average for overview image' )
        return 'fail'

    # read base band as overview resolution and verify we aren't getting
    # the grayscale image.

    frband = wrk_ds.GetRasterBand(1)
    ovimage = frband.ReadRaster(0,0,frband.XSize,frband.YSize,
                                ovband.XSize,ovband.YSize)

    pix_count = ovband.XSize * ovband.YSize
    sum = 0.0
    for i in range(pix_count):
        sum = sum + ord(ovimage[i])
    average = sum / pix_count
    exp_average = 0.6096
    
    if abs(average - exp_average) > 0.01:
        print average
        gdaltest.post_reason( 'got wrong average for downsampled image' )
        return 'fail'

    wrk_ds = None

    return 'success'
    

###############################################################################
# Test average overview generation with nodata.

def tiff_ovr_5():

    shutil.copyfile( 'data/nodata_byte.tif', 'tmp/ovr5.tif' )

    wrk_ds = gdal.Open('tmp/ovr5.tif',gdal.GA_ReadOnly)
    
    if wrk_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    wrk_ds.BuildOverviews( 'AVERAGE', overviewlist = [2] )

    cs = wrk_ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 1130

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'
    
###############################################################################
# Same as tiff_ovr_5 but with USE_RDD=YES to force external overview

def tiff_ovr_6():

    shutil.copyfile( 'data/nodata_byte.tif', 'tmp/ovr6.tif' )
    
    oldOption = gdal.GetConfigOption('USE_RRD', 'NO')
    gdal.SetConfigOption('USE_RRD', 'YES')
    
    wrk_ds = gdal.Open('tmp/ovr6.tif',gdal.GA_Update)
    
    if wrk_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    wrk_ds.BuildOverviews( 'AVERAGE', overviewlist = [2] )
    
    gdal.SetConfigOption('USE_RRD', oldOption)
    
    try:
        os.stat('tmp/ovr6.aux')
    except:
        gdaltest.post_reason( 'no external overview.' )
        return 'fail'
    
    cs = wrk_ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 1130

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'


###############################################################################
# Check nearest resampling on a dataset with a raster band that has a color table

def tiff_ovr_7():

    shutil.copyfile( 'data/test_average_palette.tif', 'tmp/test_average_palette.tif' )

    # This dataset is a black&white chessboard, index 0 is black, index 1 is white.
    # In nearest resampling, we are expecting a uniform black image.
    ds = gdal.Open('tmp/test_average_palette.tif', gdal.GA_Update)
    
    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds.BuildOverviews( 'NEAREST', overviewlist = [2] )

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 0

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'

###############################################################################
# Check average resampling on a dataset with a raster band that has a color table

def tiff_ovr_8():

    shutil.copyfile( 'data/test_average_palette.tif', 'tmp/test_average_palette.tif' )

    # This dataset is a black&white chessboard, index 0 is black, index 1 is white.
    # So the result of averaging (0,0,0) and (255,255,255) is (127,127,127), which is
    # index 2. So the result of the averaging is a uniform grey image.
    ds = gdal.Open('tmp/test_average_palette.tif', gdal.GA_Update)
    
    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds.BuildOverviews( 'AVERAGE', overviewlist = [2] )

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 200

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'

###############################################################################
# Test --config COMPRESS_OVERVIEW JPEG --config PHOTOMETRIC_OVERVIEW YCBCR
# --config INTERLEAVE_OVERVIEW PIXEL -ro

def tiff_ovr_9():

    shutil.copyfile( 'data/rgbsmall.tif', 'tmp/ovr9.tif' )

    gdal.SetConfigOption('COMPRESS_OVERVIEW', 'JPEG')
    gdal.SetConfigOption('PHOTOMETRIC_OVERVIEW', 'YCBCR')
    gdal.SetConfigOption('INTERLEAVE_OVERVIEW', 'PIXEL')

    ds = gdal.Open('tmp/ovr9.tif', gdal.GA_ReadOnly)

    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds.BuildOverviews( 'AVERAGE', overviewlist = [2] )

    gdal.SetConfigOption('COMPRESS_OVERVIEW', '')
    gdal.SetConfigOption('PHOTOMETRIC_OVERVIEW', '')
    gdal.SetConfigOption('INTERLEAVE_OVERVIEW', '')

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 5700

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'

###############################################################################
# Similar to tiff_ovr_9 but with internal overviews.

def tiff_ovr_10():

    src_ds = gdal.Open('data/rgbsmall.tif', gdal.GA_ReadOnly)
    
    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds = gdaltest.tiff_drv.CreateCopy('tmp/ovr10.tif', src_ds, options = [ 'COMPRESS=JPEG', 'PHOTOMETRIC=YCBCR' ] )
    src_ds = None

    if ds is None:
        gdaltest.post_reason( 'Failed to apply JPEG compression.' )
        return 'fail'

    ds.BuildOverviews( 'AVERAGE', overviewlist = [2] )

    ds = None
    ds = gdal.Open('tmp/ovr10.tif', gdal.GA_ReadOnly)

    if ds is None:
        gdaltest.post_reason( 'Failed to open copy of test dataset.' )
        return 'fail'

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 5700

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'

###############################################################################
# Overview on a dataset with NODATA_VALUES

def tiff_ovr_11():

    src_ds = gdal.Open('data/test_nodatavalues.tif', gdal.GA_ReadOnly)
    
    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds = gdaltest.tiff_drv.CreateCopy('tmp/ovr11.tif', src_ds )
    src_ds = None

    # The two following lines are necessary with inverted endianness
    # for the moment
    # See http://bugzilla.maptools.org/show_bug.cgi?id=1924 for more details
    ds = None
    ds = gdal.Open('tmp/ovr11.tif', gdal.GA_Update)

    ds.BuildOverviews( 'AVERAGE', overviewlist = [2] )

    ds = None
    ds = gdal.Open('tmp/ovr11.tif', gdal.GA_ReadOnly)

    if ds is None:
        gdaltest.post_reason( 'Failed to open copy of test dataset.' )
        return 'fail'

    cs = ds.GetRasterBand(2).GetOverview(0).Checksum()
    # If NODATA_VALUES was ignored, we would get 2766
    exp_cs = 2792

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'

###############################################################################
# Same as tiff_ovr_11 but with compression to trigger the multiband overview
# code

def tiff_ovr_12():

    src_ds = gdal.Open('data/test_nodatavalues.tif', gdal.GA_ReadOnly)
    
    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds = gdaltest.tiff_drv.CreateCopy('tmp/ovr12.tif', src_ds, options = [ 'COMPRESS=DEFLATE' ] )
    src_ds = None

    # The two following lines are necessary with inverted endianness
    # for the moment
    # See http://bugzilla.maptools.org/show_bug.cgi?id=1924 for more details
    ds = None
    ds = gdal.Open('tmp/ovr12.tif', gdal.GA_Update)

    ds.BuildOverviews( 'AVERAGE', overviewlist = [2] )

    ds = None
    ds = gdal.Open('tmp/ovr12.tif', gdal.GA_ReadOnly)

    if ds is None:
        gdaltest.post_reason( 'Failed to open copy of test dataset.' )
        return 'fail'

    cs = ds.GetRasterBand(2).GetOverview(0).Checksum()
    # If NODATA_VALUES was ignored, we would get 2766
    exp_cs = 2792

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'


###############################################################################
# Test gaussian resampling

def tiff_ovr_13():

    gdaltest.tiff_drv = gdal.GetDriverByName( 'GTiff' )

    src_ds = gdal.Open('data/mfloat32.vrt')

    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    gdaltest.tiff_drv.CreateCopy( 'tmp/mfloat32.tif', src_ds,
                                  options = ['INTERLEAVE=PIXEL'] )
    src_ds = None

    ds = gdal.Open( 'tmp/mfloat32.tif' )

    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    err = ds.BuildOverviews( 'GAUSS', overviewlist = [2, 4] )

    if err != 0:
        gdaltest.post_reason('BuildOverviews reports an error' )
        return 'fail'

    #if ds.GetRasterBand(1).GetOverview(0).Checksum() != 1225:
    #    gdaltest.post_reason( 'bad checksum' )
    #    return 'fail'

    ds = None

    return 'success'

###############################################################################
# Check gauss resampling on a dataset with a raster band that has a color table

def tiff_ovr_14():

    shutil.copyfile( 'data/test_average_palette.tif', 'tmp/test_gauss_palette.tif' )

    ds = gdal.Open('tmp/test_gauss_palette.tif', gdal.GA_Update)

    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds.BuildOverviews( 'GAUSS', overviewlist = [2] )

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 200

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'

###############################################################################
# Same as tiff_ovr_11 but with gauss, and compression to trigger the multiband overview
# code

def tiff_ovr_15():

    src_ds = gdal.Open('data/test_nodatavalues.tif', gdal.GA_ReadOnly)
    
    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds = gdaltest.tiff_drv.CreateCopy('tmp/ovr15.tif', src_ds, options = [ 'COMPRESS=DEFLATE' ] )
    src_ds = None

    # The two following lines are necessary with inverted endianness
    # for the moment
    # See http://bugzilla.maptools.org/show_bug.cgi?id=1924 for more details
    ds = None
    ds = gdal.Open('tmp/ovr15.tif', gdal.GA_Update)

    ds.BuildOverviews( 'GAUSS', overviewlist = [2] )

    ds = None
    ds = gdal.Open('tmp/ovr15.tif', gdal.GA_ReadOnly)

    if ds is None:
        gdaltest.post_reason( 'Failed to open copy of test dataset.' )
        return 'fail'

    cs = ds.GetRasterBand(2).GetOverview(0).Checksum()
    # If NODATA_VALUES was ignored, we would get 2954
    exp_cs = 2987

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'


###############################################################################
# Test mode resampling on non-byte dataset

def tiff_ovr_16():

    gdaltest.tiff_drv = gdal.GetDriverByName( 'GTiff' )

    src_ds = gdal.Open('data/mfloat32.vrt')

    if src_ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    gdaltest.tiff_drv.CreateCopy( 'tmp/ovr16.tif', src_ds,
                                  options = ['INTERLEAVE=PIXEL'] )
    src_ds = None

    ds = gdal.Open( 'tmp/ovr16.tif' )

    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    err = ds.BuildOverviews( 'MODE', overviewlist = [2, 4] )

    if err != 0:
        gdaltest.post_reason('BuildOverviews reports an error' )
        return 'fail'

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 1122
    if cs != exp_cs:
        gdaltest.post_reason( 'bad checksum' )
        print exp_cs, cs
        return 'fail'

    ds = None

    return 'success'


###############################################################################
# Test mode resampling on a byte dataset

def tiff_ovr_17():

    shutil.copyfile( 'data/byte.tif', 'tmp/ovr17.tif' )

    ds = gdal.Open( 'tmp/ovr17.tif' )

    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    err = ds.BuildOverviews( 'MODE', overviewlist = [2, 4] )

    if err != 0:
        gdaltest.post_reason('BuildOverviews reports an error' )
        return 'fail'

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 1122
    if cs != exp_cs:
        gdaltest.post_reason( 'bad checksum' )
        print exp_cs, cs
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Check mode resampling on a dataset with a raster band that has a color table

def tiff_ovr_18():

    shutil.copyfile( 'data/test_average_palette.tif', 'tmp/ovr18.tif' )

    ds = gdal.Open('tmp/ovr18.tif', gdal.GA_Update)

    if ds is None:
        gdaltest.post_reason( 'Failed to open test dataset.' )
        return 'fail'

    ds.BuildOverviews( 'MODE', overviewlist = [2] )

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    exp_cs = 100

    ds = None

    if cs != exp_cs:
        gdaltest.post_reason( 'got wrong overview checksum.' )
        print exp_cs, cs
        return 'fail'

    return 'success'

###############################################################################
# Check that we can create overviews on a newly create file (#2621)
# Will cause older libtiff versions (<=3.8.2 for sure) to crash, so skip it
# if BigTIFF is not supported (this is a sign of an older libtiff...)

def tiff_ovr_19():

    drv = gdal.GetDriverByName( 'GTiff' )
    md = drv.GetMetadata()
    if string.find(md['DMD_CREATIONOPTIONLIST'],'BigTIFF') == -1:
        return 'skip'

    ds=gdal.GetDriverByName('GTiff').Create('tmp/ovr19.tif',100,100,1)
    ds.GetRasterBand(1).Fill(1)

    # The flush is important to simulate the behaviour that wash it by #2621
    ds.FlushCache()
    ds.BuildOverviews('NEAR', overviewlist = [2])
    ds.FlushCache()
    ds.BuildOverviews('NEAR', overviewlist = [2,4])

    if ds.GetRasterBand(1).GetOverviewCount() != 2 is None:
        print 'Overview could not be generated'
        return 'fail'

    cs = ds.GetRasterBand(1).GetOverview(0).Checksum()
    if cs != 2500:
        print cs
        return 'fail'

    cs = ds.GetRasterBand(1).GetOverview(1).Checksum()
    if cs != 625:
        print cs
        return 'fail'

    ds = None

    return 'success'

###############################################################################
# Cleanup

def tiff_ovr_cleanup():
    gdaltest.tiff_drv.Delete( 'tmp/mfloat32.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr4.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr5.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr6.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/test_average_palette.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr9.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr10.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr11.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr12.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/test_gauss_palette.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr15.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr16.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr17.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr18.tif' )
    gdaltest.tiff_drv.Delete( 'tmp/ovr19.tif' )
    gdaltest.tiff_drv = None

    return 'success'

gdaltest_list_internal = [
    tiff_ovr_1,
    tiff_ovr_2,
    tiff_ovr_3,
    tiff_ovr_3bis,
    tiff_ovr_4,
    tiff_ovr_5,
    tiff_ovr_6,
    tiff_ovr_7,
    tiff_ovr_8,
    tiff_ovr_9,
    tiff_ovr_10,
    tiff_ovr_11,
    tiff_ovr_12,
    tiff_ovr_13,
    tiff_ovr_14,
    tiff_ovr_15,
    tiff_ovr_16,
    tiff_ovr_17,
    tiff_ovr_18,
    tiff_ovr_19,
    tiff_ovr_cleanup ]

def tiff_ovr_invert_endianness():
    gdaltest.tiff_endianness = gdal.GetConfigOption( 'GDAL_TIFF_ENDIANNESS', "NATIVE" )
    gdal.SetConfigOption( 'GDAL_TIFF_ENDIANNESS', 'INVERTED' )
    return 'success'

def tiff_ovr_restore_endianness():
    gdal.SetConfigOption( 'GDAL_TIFF_ENDIANNESS', gdaltest.tiff_endianness )
    return 'success'

gdaltest_list = []
for item in gdaltest_list_internal:
    gdaltest_list.append(item)
gdaltest_list.append(tiff_ovr_invert_endianness)
for item in gdaltest_list_internal:
    gdaltest_list.append( (item, item.__name__ + '_interverted') )
gdaltest_list.append(tiff_ovr_restore_endianness)

if __name__ == '__main__':

    gdaltest.setup_run( 'tiff_ovr' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

