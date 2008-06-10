#!/usr/bin/env python
###############################################################################
# $Id$
#
# Project:  GDAL/OGR Test Suite
# Purpose:  Test functioning of the PAM metadata support.
# Author:   Frank Warmerdam <warmerdam@pobox.com>
# 
###############################################################################
# Copyright (c) 2003, Frank Warmerdam <warmerdam@pobox.com>
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

sys.path.append( '../pymod' )

import gdaltest
import gdal

###############################################################################
# Check that we can read PAM metadata for existing PNM file.

def pam_md_1():

    gdaltest.pam_setting = gdal.GetConfigOption( 'GDAL_PAM_ENABLED', "NULL" )
    gdal.SetConfigOption( 'GDAL_PAM_ENABLED', 'YES' )

    ds = gdal.Open( "data/byte.pnm" )

    base_md = ds.GetMetadata()
    if len(base_md) != 2 or base_md['other'] != 'red' \
       or base_md['key'] != 'value':
        gdaltest.post_reason( 'Default domain metadata missing' )
        return 'fail'

    xml_md = ds.GetMetadata( 'xml:test' )

    if len(xml_md) != 1:
        gdaltest.post_reason( 'xml:test metadata missing' )
        return 'fail'

    if type(xml_md) != type(['abc']):
        gdaltest.post_reason( 'xml:test metadata not returned as list.' )
        return 'fail'

    expected_xml = """<?xml version="2.0"?>
<TestXML>Value</TestXML>
"""

    if xml_md[0] != expected_xml:
        gdaltest.post_reason( 'xml does not match' )
        print xml_md
        return 'fail'
    
    return 'success' 

###############################################################################
# Verify that we can write XML to a new file. 

def pam_md_2():

    driver = gdal.GetDriverByName( 'PNM' )
    ds = driver.Create( 'tmp/pam_md.pnm', 10, 10 )
    band = ds.GetRasterBand( 1 )

    band.SetMetadata( { 'other' : 'red', 'key' : 'value' } )

    expected_xml = """<?xml version="2.0"?>
<TestXML>Value</TestXML>
"""

    band.SetMetadata( [ expected_xml ], 'xml:test' )

    band.SetNoDataValue( 100 )

    ds = None

    return 'success' 

###############################################################################
# Check that we can read PAM metadata for existing PNM file.

def pam_md_3():

    ds = gdal.Open( "tmp/pam_md.pnm" )

    band = ds.GetRasterBand(1)
    base_md = band.GetMetadata()
    if len(base_md) != 2 or base_md['other'] != 'red' \
       or base_md['key'] != 'value':
        gdaltest.post_reason( 'Default domain metadata missing' )
        return 'fail'

    xml_md = band.GetMetadata( 'xml:test' )

    if len(xml_md) != 1:
        gdaltest.post_reason( 'xml:test metadata missing' )
        return 'fail'

    if type(xml_md) != type(['abc']):
        gdaltest.post_reason( 'xml:test metadata not returned as list.' )
        return 'fail'

    expected_xml = """<?xml version="2.0"?>
<TestXML>Value</TestXML>
"""

    if xml_md[0] != expected_xml:
        gdaltest.post_reason( 'xml does not match' )
        print xml_md
        return 'fail'

    if band.GetNoDataValue() != 100:
        gdaltest.post_reason( 'nodata not saved via pam' )
        return 'fail'
    
    return 'success' 

###############################################################################
# Cleanup.

def pam_md_cleanup():
    gdaltest.clean_tmp()
    if gdaltest.pam_setting != 'NULL':
	gdal.SetConfigOption( 'GDAL_PAM_ENABLED', gdaltest.pam_setting )
    else:
	gdal.SetConfigOption( 'GDAL_PAM_ENABLED', None )
    return 'success'

gdaltest_list = [
    pam_md_1,
    pam_md_2,
    pam_md_3,
    pam_md_cleanup ]

if __name__ == '__main__':

    gdaltest.setup_run( 'pam_md' )

    gdaltest.run_tests( gdaltest_list )

    gdaltest.summarize()

