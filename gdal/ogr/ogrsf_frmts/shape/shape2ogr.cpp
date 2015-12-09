/******************************************************************************
 * $Id$
 *
 * Project:  OpenGIS Simple Features Reference Implementation
 * Purpose:  Implements translation of Shapefile shapes into OGR
 *           representation.
 * Author:   Frank Warmerdam, warmerda@home.com
 *
 ******************************************************************************
 * Copyright (c) 1999,  Les Technologies SoftMap Inc.
 * Copyright (c) 2007-2013, Even Rouault <even dot rouault at mines-paris dot org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 ****************************************************************************/

#include "ogrshape.h"
#include "cpl_conv.h"

CPL_CVSID("$Id$");

/************************************************************************/
/*                        RingStartEnd                                  */
/*        set first and last vertex for given ring                      */
/************************************************************************/
static void RingStartEnd ( SHPObject *psShape, int ring, int *start, int *end )
{
    if( psShape->panPartStart == NULL )
    {
	    *start = 0;
        *end = psShape->nVertices - 1;
    }
    else
    {
        if( ring == psShape->nParts - 1 )
            *end = psShape->nVertices - 1;
        else
            *end = psShape->panPartStart[ring+1] - 1;

        *start = psShape->panPartStart[ring];
    }
}
    
/************************************************************************/
/*                        CreateLinearRing                              */
/*                                                                      */
/************************************************************************/
static OGRLinearRing * CreateLinearRing ( SHPObject *psShape, int ring, int bHasZ )
{
    OGRLinearRing *poRing;
    int nRingStart, nRingEnd, nRingPoints;

    poRing = new OGRLinearRing();

    RingStartEnd ( psShape, ring, &nRingStart, &nRingEnd );

    nRingPoints = nRingEnd - nRingStart + 1;

    if (bHasZ)
        poRing->setPoints( nRingPoints, psShape->padfX + nRingStart, 
                           psShape->padfY + nRingStart,
                           psShape->padfZ + nRingStart );
    else
        poRing->setPoints( nRingPoints, psShape->padfX + nRingStart,
                           psShape->padfY + nRingStart );

    return ( poRing );
}


/************************************************************************/
/*                          SHPReadOGRObject()                          */
/*                                                                      */
/*      Read an item in a shapefile, and translate to OGR geometry      */
/*      representation.                                                 */
/************************************************************************/

OGRGeometry *SHPReadOGRObject( SHPHandle hSHP, int iShape, SHPObject *psShape )
{
    // CPLDebug( "Shape", "SHPReadOGRObject( iShape=%d )\n", iShape );

    OGRGeometry *poOGR = NULL;

    if( psShape == NULL )
        psShape = SHPReadObject( hSHP, iShape );

    if( psShape == NULL )
    {
        return NULL;
    }

/* -------------------------------------------------------------------- */
/*      Point.                                                          */
/* -------------------------------------------------------------------- */
    else if( psShape->nSHPType == SHPT_POINT )
    {
        poOGR = new OGRPoint( psShape->padfX[0], psShape->padfY[0] );
    }
    else if(psShape->nSHPType == SHPT_POINTZ )
    {
        poOGR = new OGRPoint( psShape->padfX[0], psShape->padfY[0],
                              psShape->padfZ[0] );
    }
    else if(psShape->nSHPType == SHPT_POINTM )
    {
        // Read XYM as XYZ
        poOGR = new OGRPoint( psShape->padfX[0], psShape->padfY[0],
                              psShape->padfM[0] );
    }
/* -------------------------------------------------------------------- */
/*      Multipoint.                                                     */
/* -------------------------------------------------------------------- */
    else if( psShape->nSHPType == SHPT_MULTIPOINT
             || psShape->nSHPType == SHPT_MULTIPOINTM
             || psShape->nSHPType == SHPT_MULTIPOINTZ )
    {
        if (psShape->nVertices == 0)
        {
            poOGR = NULL;
        }
        else
        {
            OGRMultiPoint *poOGRMPoint = new OGRMultiPoint();
            int             i;

            for( i = 0; i < psShape->nVertices; i++ )
            {
                OGRPoint    *poPoint;

                if( psShape->nSHPType == SHPT_MULTIPOINTZ )
                    poPoint = new OGRPoint( psShape->padfX[i], psShape->padfY[i],
                                            psShape->padfZ[i] );
                else
                    poPoint = new OGRPoint( psShape->padfX[i], psShape->padfY[i] );

                poOGRMPoint->addGeometry( poPoint );

                delete poPoint;
            }

            poOGR = poOGRMPoint;
        }
    }

/* -------------------------------------------------------------------- */
/*      Arc (LineString)                                                */
/*                                                                      */
/*      I am ignoring parts though they can apply to arcs as well.      */
/* -------------------------------------------------------------------- */
    else if( psShape->nSHPType == SHPT_ARC
             || psShape->nSHPType == SHPT_ARCM
             || psShape->nSHPType == SHPT_ARCZ )
    {
        if( psShape->nParts == 0 )
        {
            poOGR = NULL;
        }
        else if( psShape->nParts == 1 )
        {
            OGRLineString *poOGRLine = new OGRLineString();

            if( psShape->nSHPType == SHPT_ARCZ )
                poOGRLine->setPoints( psShape->nVertices,
                                    psShape->padfX, psShape->padfY, psShape->padfZ );
            else if( psShape->nSHPType == SHPT_ARCM )
                // Read XYM as XYZ
                poOGRLine->setPoints( psShape->nVertices,
                                    psShape->padfX, psShape->padfY, psShape->padfM );
            else
                poOGRLine->setPoints( psShape->nVertices,
                                    psShape->padfX, psShape->padfY );

            poOGR = poOGRLine;
        }
        else
        {
            int iRing;
            OGRMultiLineString *poOGRMulti;
        
            poOGR = poOGRMulti = new OGRMultiLineString();
            
            for( iRing = 0; iRing < psShape->nParts; iRing++ )
            {
                OGRLineString   *poLine;
                int     nRingPoints;
                int     nRingStart;

                poLine = new OGRLineString();

                if( psShape->panPartStart == NULL )
                {
                    nRingPoints = psShape->nVertices;
                    nRingStart = 0;
                }
                else
                {

                    if( iRing == psShape->nParts - 1 )
                        nRingPoints =
                            psShape->nVertices - psShape->panPartStart[iRing];
                    else
                        nRingPoints = psShape->panPartStart[iRing+1]
                            - psShape->panPartStart[iRing];
                    nRingStart = psShape->panPartStart[iRing];
                }

                if( psShape->nSHPType == SHPT_ARCZ )
                    poLine->setPoints( nRingPoints,
                                    psShape->padfX + nRingStart,
                                    psShape->padfY + nRingStart,
                                    psShape->padfZ + nRingStart );
                else if( psShape->nSHPType == SHPT_ARCM )
                    // Read XYM as XYZ
                    poLine->setPoints( nRingPoints,
                                    psShape->padfX + nRingStart,
                                    psShape->padfY + nRingStart,
                                    psShape->padfM + nRingStart );
                else
                    poLine->setPoints( nRingPoints,
                                    psShape->padfX + nRingStart,
                                    psShape->padfY + nRingStart );

                poOGRMulti->addGeometryDirectly( poLine );
            }
        }
    }

/* -------------------------------------------------------------------- */
/*      Polygon                                                         */
/*                                                                      */
/* As for now Z coordinate is not handled correctly                     */
/* -------------------------------------------------------------------- */
    else if( psShape->nSHPType == SHPT_POLYGON
             || psShape->nSHPType == SHPT_POLYGONM
             || psShape->nSHPType == SHPT_POLYGONZ )
    {
        int iRing;
        int bHasZ = ( psShape->nSHPType == SHPT_POLYGONZ );
        
        //CPLDebug( "Shape", "Shape type: polygon with nParts=%d \n", psShape->nParts );

        if ( psShape->nParts == 0 )
        {
            poOGR = NULL;
        }
        else if ( psShape->nParts == 1 )
        {
            /* Surely outer ring */
            OGRPolygon *poOGRPoly = NULL;
            OGRLinearRing *poRing = NULL;

            poOGR = poOGRPoly = new OGRPolygon();
            poRing = CreateLinearRing ( psShape, 0, bHasZ );
            poOGRPoly->addRingDirectly( poRing );
        }

        else
        {
            OGRPolygon** tabPolygons = new OGRPolygon*[psShape->nParts];
            for( iRing = 0; iRing < psShape->nParts; iRing++ )
            {
                tabPolygons[iRing] = new OGRPolygon();
                tabPolygons[iRing]->addRingDirectly(CreateLinearRing ( psShape, iRing, bHasZ ));
            }

            int isValidGeometry;
            const char* papszOptions[] = { "METHOD=ONLY_CCW", NULL };
            poOGR = OGRGeometryFactory::organizePolygons( 
                (OGRGeometry**)tabPolygons, psShape->nParts, &isValidGeometry, papszOptions );

            if (!isValidGeometry)
            {
                CPLError(CE_Warning, CPLE_AppDefined, 
                        "Geometry of polygon of fid %d cannot be translated to Simple Geometry. "
                        "All polygons will be contained in a multipolygon.\n",
                        iShape);
            }

            delete[] tabPolygons;
        }
    }

/* -------------------------------------------------------------------- */
/*      MultiPatch                                                      */
/* -------------------------------------------------------------------- */
    else if( psShape->nSHPType == SHPT_MULTIPATCH )
    {
        OGRMultiPolygon *poMP = new OGRMultiPolygon();
        int iPart;
        OGRPolygon *poLastPoly = NULL;

        for( iPart = 0; iPart < psShape->nParts; iPart++ )
        {
            int nPartPoints, nPartStart;

            // Figure out details about this part's vertex list.
            if( psShape->panPartStart == NULL )
            {
                nPartPoints = psShape->nVertices;
                nPartStart = 0;
            }
            else
            {
                
                if( iPart == psShape->nParts - 1 )
                    nPartPoints =
                        psShape->nVertices - psShape->panPartStart[iPart];
                else
                    nPartPoints = psShape->panPartStart[iPart+1]
                        - psShape->panPartStart[iPart];
                nPartStart = psShape->panPartStart[iPart];
            }

            if( psShape->panPartType[iPart] == SHPP_TRISTRIP )
            {
                int iBaseVert;

                if( poLastPoly != NULL )
                {
                    poMP->addGeometryDirectly( poLastPoly );
                    poLastPoly = NULL;
                }

                for( iBaseVert = 0; iBaseVert < nPartPoints-2; iBaseVert++ )
                {
                    OGRPolygon *poPoly = new OGRPolygon();
                    OGRLinearRing *poRing = new OGRLinearRing();
                    int iSrcVert = iBaseVert + nPartStart;

                    poRing->setPoint( 0, 
                                      psShape->padfX[iSrcVert], 
                                      psShape->padfY[iSrcVert], 
                                      psShape->padfZ[iSrcVert] );
                    poRing->setPoint( 1, 
                                      psShape->padfX[iSrcVert+1], 
                                      psShape->padfY[iSrcVert+1], 
                                      psShape->padfZ[iSrcVert+1] );

                    poRing->setPoint( 2, 
                                      psShape->padfX[iSrcVert+2], 
                                      psShape->padfY[iSrcVert+2], 
                                      psShape->padfZ[iSrcVert+2] );
                    poRing->setPoint( 3, 
                                      psShape->padfX[iSrcVert], 
                                      psShape->padfY[iSrcVert], 
                                      psShape->padfZ[iSrcVert] );
                        
                    poPoly->addRingDirectly( poRing );
                    poMP->addGeometryDirectly( poPoly );
                }
            }
            else if( psShape->panPartType[iPart] == SHPP_TRIFAN )
            {
                int iBaseVert;

                if( poLastPoly != NULL )
                {
                    poMP->addGeometryDirectly( poLastPoly );
                    poLastPoly = NULL;
                }

                for( iBaseVert = 0; iBaseVert < nPartPoints-2; iBaseVert++ )
                {
                    OGRPolygon *poPoly = new OGRPolygon();
                    OGRLinearRing *poRing = new OGRLinearRing();
                    int iSrcVert = iBaseVert + nPartStart;

                    poRing->setPoint( 0, 
                                      psShape->padfX[nPartStart], 
                                      psShape->padfY[nPartStart],
                                      psShape->padfZ[nPartStart] );
                    poRing->setPoint( 1, 
                                      psShape->padfX[iSrcVert+1], 
                                      psShape->padfY[iSrcVert+1], 
                                      psShape->padfZ[iSrcVert+1] );

                    poRing->setPoint( 2, 
                                      psShape->padfX[iSrcVert+2], 
                                      psShape->padfY[iSrcVert+2], 
                                      psShape->padfZ[iSrcVert+2] );
                    poRing->setPoint( 3, 
                                      psShape->padfX[nPartStart], 
                                      psShape->padfY[nPartStart], 
                                      psShape->padfZ[nPartStart] );

                    poPoly->addRingDirectly( poRing );
                    poMP->addGeometryDirectly( poPoly );
                }
            }
            else if( psShape->panPartType[iPart] == SHPP_OUTERRING
                     || psShape->panPartType[iPart] == SHPP_INNERRING
                     || psShape->panPartType[iPart] == SHPP_FIRSTRING
                     || psShape->panPartType[iPart] == SHPP_RING )
            {
                if( poLastPoly != NULL 
                    && (psShape->panPartType[iPart] == SHPP_OUTERRING
                        || psShape->panPartType[iPart] == SHPP_FIRSTRING) )
                {
                    poMP->addGeometryDirectly( poLastPoly );
                    poLastPoly = NULL;
                }

                if( poLastPoly == NULL )
                    poLastPoly = new OGRPolygon();

                poLastPoly->addRingDirectly( 
                    CreateLinearRing( psShape, iPart, TRUE ) );
            }
            else
                CPLDebug( "OGR", "Unrecognized parttype %d, ignored.",
                          psShape->panPartType[iPart] );
        }

        if( poLastPoly != NULL )
        {
            poMP->addGeometryDirectly( poLastPoly );
            poLastPoly = NULL;
        }

        poOGR = poMP;
    }

/* -------------------------------------------------------------------- */
/*      Otherwise for now we just ignore the object.                    */
/* -------------------------------------------------------------------- */
    else
    {
        if( psShape->nSHPType != SHPT_NULL )
        {
            CPLDebug( "OGR", "Unsupported shape type in SHPReadOGRObject()" );
        }

        /* nothing returned */
    }

/* -------------------------------------------------------------------- */
/*      Cleanup shape, and set feature id.                              */
/* -------------------------------------------------------------------- */
    SHPDestroyObject( psShape );

    return poOGR;
}

/************************************************************************/
/*                         SHPWriteOGRObject()                          */
/************************************************************************/
static
OGRErr SHPWriteOGRObject( SHPHandle hSHP, int iShape, OGRGeometry *poGeom,
                          int bRewind)

{
    int nReturnedShapeID;
/* ==================================================================== */
/*      Write "shape" with no geometry or with empty geometry           */
/* ==================================================================== */
    if( poGeom == NULL || poGeom->IsEmpty() )
    {
        SHPObject       *psShape;

        psShape = SHPCreateSimpleObject( SHPT_NULL, 0, NULL, NULL, NULL );
        nReturnedShapeID = SHPWriteObject( hSHP, iShape, psShape );
        SHPDestroyObject( psShape );
        if( nReturnedShapeID == -1 )
        {
            //Assuming error is reported by SHPWriteObject()
            return OGRERR_FAILURE;
        }
    }

/* ==================================================================== */
/*      Write point geometry.                                           */
/* ==================================================================== */
    else if( hSHP->nShapeType == SHPT_POINT
             || hSHP->nShapeType == SHPT_POINTM
             || hSHP->nShapeType == SHPT_POINTZ )
    {
        SHPObject       *psShape;
        double          dfX, dfY, dfZ = 0;

        if( poGeom->getGeometryType() != wkbPoint
            && poGeom->getGeometryType() != wkbPoint25D )        
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Attempt to write non-point (%s) geometry to"
                      " point shapefile.",
                      poGeom->getGeometryName() );

            return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
        }

        OGRPoint        *poPoint = (OGRPoint *) poGeom;
        dfX = poPoint->getX();
        dfY = poPoint->getY();
        dfZ = poPoint->getZ();
        
        psShape = SHPCreateSimpleObject( hSHP->nShapeType, 1,
                                         &dfX, &dfY, &dfZ );
        nReturnedShapeID = SHPWriteObject( hSHP, iShape, psShape );
        SHPDestroyObject( psShape );
        if( nReturnedShapeID == -1 )
            return OGRERR_FAILURE;
    }
/* ==================================================================== */
/*      MultiPoint.                                                     */
/* ==================================================================== */
    else if( hSHP->nShapeType == SHPT_MULTIPOINT
             || hSHP->nShapeType == SHPT_MULTIPOINTM
             || hSHP->nShapeType == SHPT_MULTIPOINTZ )
    {
        double          *padfX, *padfY, *padfZ;
        int             iPoint;
        SHPObject       *psShape;

        if( wkbFlatten(poGeom->getGeometryType()) != wkbMultiPoint )
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Attempt to write non-multipoint (%s) geometry to "
                      "multipoint shapefile.",
                      poGeom->getGeometryName() );

            return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
        }

        OGRMultiPoint   *poMP = (OGRMultiPoint *) poGeom;
        padfX = (double *) CPLMalloc(sizeof(double)*poMP->getNumGeometries());
        padfY = (double *) CPLMalloc(sizeof(double)*poMP->getNumGeometries());
        padfZ = (double *) CPLCalloc(sizeof(double),poMP->getNumGeometries());

        int iDstPoints = 0;
        for( iPoint = 0; iPoint < poMP->getNumGeometries(); iPoint++ )
        {
            OGRPoint    *poPoint = (OGRPoint *) poMP->getGeometryRef(iPoint);

            /* Ignore POINT EMPTY */
            if (poPoint->IsEmpty() == FALSE)
            {
                padfX[iDstPoints] = poPoint->getX();
                padfY[iDstPoints] = poPoint->getY();
                padfZ[iDstPoints] = poPoint->getZ();
                iDstPoints ++;
            }
            else
                CPLDebug( "OGR", 
                              "Ignore POINT EMPTY inside MULTIPOINT in shapefile writer." );
        }

        psShape = SHPCreateSimpleObject( hSHP->nShapeType,
                                         iDstPoints,
                                         padfX, padfY, padfZ );
        nReturnedShapeID = SHPWriteObject( hSHP, iShape, psShape );
        SHPDestroyObject( psShape );
        
        CPLFree( padfX );
        CPLFree( padfY );
        CPLFree( padfZ );
        if( nReturnedShapeID == -1 )
            return OGRERR_FAILURE;
    }

/* ==================================================================== */
/*      Arcs from simple line strings.                                  */
/* ==================================================================== */
    else if( (hSHP->nShapeType == SHPT_ARC
              || hSHP->nShapeType == SHPT_ARCM
              || hSHP->nShapeType == SHPT_ARCZ)
             && wkbFlatten(poGeom->getGeometryType()) == wkbLineString )
    {
        OGRLineString   *poArc = (OGRLineString *) poGeom;
        double          *padfX, *padfY, *padfZ;
        int             iPoint;
        SHPObject       *psShape;

        padfX = (double *) CPLMalloc(sizeof(double)*poArc->getNumPoints());
        padfY = (double *) CPLMalloc(sizeof(double)*poArc->getNumPoints());
        padfZ = (double *) CPLCalloc(sizeof(double),poArc->getNumPoints());

        for( iPoint = 0; iPoint < poArc->getNumPoints(); iPoint++ )
        {
            padfX[iPoint] = poArc->getX( iPoint );
            padfY[iPoint] = poArc->getY( iPoint );
            padfZ[iPoint] = poArc->getZ( iPoint );
        }

        psShape = SHPCreateSimpleObject( hSHP->nShapeType,
                                         poArc->getNumPoints(),
                                         padfX, padfY, padfZ );
        nReturnedShapeID = SHPWriteObject( hSHP, iShape, psShape );
        SHPDestroyObject( psShape );
        
        CPLFree( padfX );
        CPLFree( padfY );
        CPLFree( padfZ );
        if( nReturnedShapeID == -1 )
            return OGRERR_FAILURE;
    }
/* ==================================================================== */
/*      Arcs - Try to treat as MultiLineString.                         */
/* ==================================================================== */
    else if( hSHP->nShapeType == SHPT_ARC
             || hSHP->nShapeType == SHPT_ARCM
             || hSHP->nShapeType == SHPT_ARCZ )
    {
        OGRGeometry     *poForcedGeom;
        double          *padfX=NULL, *padfY=NULL, *padfZ=NULL;
        int             iGeom, iPoint, nPointCount = 0;
        SHPObject       *psShape;
        int             *panRingStart;
        int             nParts = 0;

        poForcedGeom = OGRGeometryFactory::forceToMultiLineString( poGeom->clone() );

        if( wkbFlatten(poForcedGeom->getGeometryType()) != wkbMultiLineString )
        {
            delete poForcedGeom;
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Attempt to write non-linestring (%s) geometry to "
                      "ARC type shapefile.",
                      poGeom->getGeometryName() );

            return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
        }
        OGRMultiLineString *poML = (OGRMultiLineString *)poForcedGeom;

        panRingStart = (int *) 
            CPLMalloc(sizeof(int) * poML->getNumGeometries());

        for( iGeom = 0; iGeom < poML->getNumGeometries(); iGeom++ )
        {
            OGRLineString *poArc = (OGRLineString *)
                poML->getGeometryRef(iGeom);
            int nNewPoints = poArc->getNumPoints();

            /* Ignore LINESTRING EMPTY */
            if (nNewPoints == 0)
            {
                CPLDebug( "OGR", 
                          "Ignore LINESTRING EMPTY inside MULTILINESTRING in shapefile writer." );
                continue;
            }

            panRingStart[nParts ++] = nPointCount;

            padfX = (double *) 
                CPLRealloc( padfX, sizeof(double)*(nNewPoints+nPointCount) );
            padfY = (double *) 
                CPLRealloc( padfY, sizeof(double)*(nNewPoints+nPointCount) );
            padfZ = (double *) 
                CPLRealloc( padfZ, sizeof(double)*(nNewPoints+nPointCount) );

            for( iPoint = 0; iPoint < nNewPoints; iPoint++ )
            {
                padfX[nPointCount] = poArc->getX( iPoint );
                padfY[nPointCount] = poArc->getY( iPoint );
                padfZ[nPointCount] = poArc->getZ( iPoint );
                nPointCount++;
            }
        }

        CPLAssert(nParts != 0);

        psShape = SHPCreateObject( hSHP->nShapeType, iShape, 
                                    nParts, 
                                    panRingStart, NULL,
                                    nPointCount, padfX, padfY, padfZ, NULL);
        nReturnedShapeID = SHPWriteObject( hSHP, iShape, psShape );
        SHPDestroyObject( psShape );

        CPLFree( panRingStart );
        CPLFree( padfX );
        CPLFree( padfY );
        CPLFree( padfZ );

        delete poML;
        if( nReturnedShapeID == -1 )
            return OGRERR_FAILURE;
    }

/* ==================================================================== */
/*      Polygons/MultiPolygons                                          */
/* ==================================================================== */
    else if( hSHP->nShapeType == SHPT_POLYGON
             || hSHP->nShapeType == SHPT_POLYGONM
             || hSHP->nShapeType == SHPT_POLYGONZ )
    {
        OGRLinearRing   *poRing, **papoRings=NULL;
        double          *padfX=NULL, *padfY=NULL, *padfZ=NULL;
        int             iPoint, iRing, nRings, nVertex=0, *panRingStart;

        /* Collect list of rings */

        if( wkbFlatten(poGeom->getGeometryType()) == wkbPolygon )
        {
            OGRPolygon* poPoly =  (OGRPolygon *) poGeom;

            if( poPoly->getExteriorRing() == NULL ||
                poPoly->getExteriorRing()->IsEmpty() )
            {
                CPLDebug( "OGR", 
                          "Ignore POLYGON EMPTY in shapefile writer." );
                nRings = 0;
            }
            else
            {
                int nSrcRings = poPoly->getNumInteriorRings()+1;
                nRings = 0;
                papoRings = (OGRLinearRing **) CPLMalloc(sizeof(void*)*nSrcRings);
                for( iRing = 0; iRing < nSrcRings; iRing++ )
                {
                    if( iRing == 0 )
                        papoRings[nRings] = poPoly->getExteriorRing();
                    else
                        papoRings[nRings] = poPoly->getInteriorRing( iRing-1 );

                    /* Ignore LINEARRING EMPTY */
                    if (papoRings[nRings]->getNumPoints() != 0)
                        nRings ++;
                    else
                        CPLDebug( "OGR", 
                                "Ignore LINEARRING EMPTY inside POLYGON in shapefile writer." );
                }
            }
        }
        else if( wkbFlatten(poGeom->getGeometryType()) == wkbMultiPolygon
                 || wkbFlatten(poGeom->getGeometryType()) 
                                                == wkbGeometryCollection )
        {
            OGRGeometryCollection *poGC = (OGRGeometryCollection *) poGeom;
            int         iGeom;

            nRings = 0;
            for( iGeom=0; iGeom < poGC->getNumGeometries(); iGeom++ )
            {
                OGRGeometry* poSubGeom = poGC->getGeometryRef( iGeom );

                if( wkbFlatten(poSubGeom->getGeometryType()) != wkbPolygon )
                {
                    CPLFree( papoRings );
                    CPLError( CE_Failure, CPLE_AppDefined,
                              "Attempt to write non-polygon (%s) geometry to "
                              "POLYGON type shapefile.",
                              poSubGeom->getGeometryName());

                    return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
                }
                OGRPolygon* poPoly =  (OGRPolygon *) poSubGeom;

                /* Ignore POLYGON EMPTY */
                if( poPoly->getExteriorRing() == NULL ||
                    poPoly->getExteriorRing()->IsEmpty() )
                {
                    CPLDebug( "OGR", 
                              "Ignore POLYGON EMPTY inside MULTIPOLYGON in shapefile writer." );
                    continue;
                }

                papoRings = (OGRLinearRing **) CPLRealloc(papoRings, 
                     sizeof(void*) * (nRings+poPoly->getNumInteriorRings()+1));
                for( iRing = 0; 
                     iRing < poPoly->getNumInteriorRings()+1; 
                     iRing++ )
                {
                    if( iRing == 0 )
                        papoRings[nRings] = poPoly->getExteriorRing();
                    else
                        papoRings[nRings] = 
                            poPoly->getInteriorRing( iRing-1 );

                    /* Ignore LINEARRING EMPTY */
                    if (papoRings[nRings]->getNumPoints() != 0)
                        nRings ++;
                    else
                        CPLDebug( "OGR", 
                              "Ignore LINEARRING EMPTY inside POLYGON in shapefile writer." );
                }
            }
        }
        else 
        {
            CPLError( CE_Failure, CPLE_AppDefined,
                      "Attempt to write non-polygon (%s) geometry to "
                      "POLYGON type shapefile.",
                      poGeom->getGeometryName() );

            return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
        }

/* -------------------------------------------------------------------- */
/*      If we only had emptypolygons or unacceptable geometries         */
/*      write NULL geometry object.                                     */
/* -------------------------------------------------------------------- */
        if( nRings == 0 )
        {
            SHPObject       *psShape;
            
            psShape = SHPCreateSimpleObject( SHPT_NULL, 0, NULL, NULL, NULL );
            nReturnedShapeID = SHPWriteObject( hSHP, iShape, psShape );
            SHPDestroyObject( psShape );

            if( nReturnedShapeID == -1 )
                return OGRERR_FAILURE;

            return OGRERR_NONE;
        }
        
        /* count vertices */
        nVertex = 0;
        for( iRing = 0; iRing < nRings; iRing++ )
            nVertex += papoRings[iRing]->getNumPoints();

        panRingStart = (int *) CPLMalloc(sizeof(int) * nRings);
        padfX = (double *) CPLMalloc(sizeof(double)*nVertex);
        padfY = (double *) CPLMalloc(sizeof(double)*nVertex);
        padfZ = (double *) CPLMalloc(sizeof(double)*nVertex);

        /* collect vertices */
        nVertex = 0;
        for( iRing = 0; iRing < nRings; iRing++ )
        {
            poRing = papoRings[iRing];
            panRingStart[iRing] = nVertex;

            for( iPoint = 0; iPoint < poRing->getNumPoints(); iPoint++ )
            {
                padfX[nVertex] = poRing->getX( iPoint );
                padfY[nVertex] = poRing->getY( iPoint );
                padfZ[nVertex] = poRing->getZ( iPoint );
                nVertex++;
            }
        }

        SHPObject* psShape = SHPCreateObject( hSHP->nShapeType, iShape, nRings,
                                   panRingStart, NULL,
                                   nVertex, padfX, padfY, padfZ, NULL );
        if( bRewind )
            SHPRewindObject( hSHP, psShape );
        nReturnedShapeID = SHPWriteObject( hSHP, iShape, psShape );
        SHPDestroyObject( psShape );
        
        CPLFree( papoRings );
        CPLFree( panRingStart );
        CPLFree( padfX );
        CPLFree( padfY );
        CPLFree( padfZ );
        if( nReturnedShapeID == -1 )
            return OGRERR_FAILURE;
    }
    else
    {
        /* do nothing for multipatch */
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
    }

    return OGRERR_NONE;
}

/************************************************************************/
/*                       SHPReadOGRFeatureDefn()                        */
/************************************************************************/

OGRFeatureDefn *SHPReadOGRFeatureDefn( const char * pszName,
                                       SHPHandle hSHP, DBFHandle hDBF,
                                       const char* pszSHPEncoding,
                                       int bAdjustType )

{
    OGRFeatureDefn      *poDefn = new OGRFeatureDefn( pszName );
    int                 iField;
    int                 nAdjustableFields = 0;
    int                 nFieldCount = (hDBF) ? DBFGetFieldCount(hDBF) : 0;

    poDefn->Reference();

    for( iField = 0; iField < nFieldCount; iField++ )
    {
        char            szFieldName[12] = {};
        int             nWidth, nPrecision;
        DBFFieldType    eDBFType;
        OGRFieldDefn    oField("", OFTInteger);
        char            chNativeType;

        chNativeType = DBFGetNativeFieldType( hDBF, iField );
        eDBFType = DBFGetFieldInfo( hDBF, iField, szFieldName,
                                    &nWidth, &nPrecision );

        if( strlen(pszSHPEncoding) > 0 )
        {
            char *pszUTF8Field = CPLRecode( szFieldName,
                                            pszSHPEncoding, CPL_ENC_UTF8);
            oField.SetName( pszUTF8Field );
            CPLFree( pszUTF8Field );
        }
        else
            oField.SetName( szFieldName );

        oField.SetWidth( nWidth );
        oField.SetPrecision( nPrecision );

        if( chNativeType == 'D' )
        {
            /* XXX - mloskot:
             * Shapefile date has following 8-chars long format: 20060101.
             * OGR splits it as YYYY/MM/DD, so 2 additional characters are
             * required.
             * Is this a correct assumption? What about time part of date?
             * Should this format look as datetime: YYYY/MM/DD HH:MM:SS
             * with 4 additional characters?
             */
            oField.SetWidth( nWidth + 2 );
            oField.SetType( OFTDate );
        }
        else if( eDBFType == FTDouble )
        {
            nAdjustableFields += (nPrecision == 0);
            if( nPrecision == 0 && nWidth < 19 )
                oField.SetType( OFTInteger64 );
            else
                oField.SetType( OFTReal );
        }
        else if( eDBFType == FTInteger )
            oField.SetType( OFTInteger );
        else
            oField.SetType( OFTString );

        poDefn->AddFieldDefn( &oField );
    }

    /* Do an optional past if requested and needed to demote Integer64->Integer */
    /* or Real->Integer64/Integer */
    if( nAdjustableFields && bAdjustType )
    {
        int* panAdjustableField = (int*)CPLCalloc(sizeof(int), nFieldCount);
        for( iField = 0; iField < nFieldCount; iField++ )
        {
            OGRFieldType eType = poDefn->GetFieldDefn(iField)->GetType();
            if( poDefn->GetFieldDefn(iField)->GetPrecision() == 0 &&
               (eType == OFTInteger64 || eType == OFTReal) )
            {
                panAdjustableField[iField] = TRUE;
                poDefn->GetFieldDefn(iField)->SetType(OFTInteger);
                //poDefn->GetFieldDefn(iField)->SetWidth(0);
            }
        }
       
        int nRowCount = DBFGetRecordCount(hDBF);
        for( int iRow = 0; iRow < nRowCount && nAdjustableFields; iRow ++ )
        {
           for( iField = 0; iField < nFieldCount; iField++ )
           {
               if( panAdjustableField[iField] )
               {
                   const char* pszValue = DBFReadStringAttribute( hDBF, iRow, iField );
                   int nValueLength = (int)strlen(pszValue);
                   //if( nValueLength >= poDefn->GetFieldDefn(iField)->GetWidth())
                   //    poDefn->GetFieldDefn(iField)->SetWidth(nValueLength);
                   if( nValueLength >= 10 )
                   {
                       int bOverflow;
                       GIntBig nVal = CPLAtoGIntBigEx(pszValue, FALSE, &bOverflow);
                       if( bOverflow )
                       {
                           poDefn->GetFieldDefn(iField)->SetType(OFTReal);
                           panAdjustableField[iField] = FALSE;
                           nAdjustableFields --;
                           
                           /*char            szFieldName[12] = {};
                           int             nWidth, nPrecision;
                           DBFGetFieldInfo( hDBF, iField, szFieldName,
                                            &nWidth, &nPrecision );
                           poDefn->GetFieldDefn(iField)->SetWidth(nWidth);*/
                       }
                       else if( !CPL_INT64_FITS_ON_INT32(nVal) )
                       {
                           poDefn->GetFieldDefn(iField)->SetType(OFTInteger64);
                           if( poDefn->GetFieldDefn(iField)->GetWidth() <= 18 )
                           {
                               panAdjustableField[iField] = FALSE;
                               nAdjustableFields --;
                               
                               /*char            szFieldName[12] = {};
                               int             nWidth, nPrecision;
                               DBFGetFieldInfo( hDBF, iField, szFieldName,
                                                &nWidth, &nPrecision );
                               poDefn->GetFieldDefn(iField)->SetWidth(nWidth);*/
                           }
                       }
                   }
               }
           }
        }
        
        CPLFree(panAdjustableField);
    }

    if( hSHP == NULL )
        poDefn->SetGeomType( wkbNone );
    else
    {
        switch( hSHP->nShapeType )
        {
          case SHPT_POINT:
            poDefn->SetGeomType( wkbPoint );
            break;

          case SHPT_POINTZ:
          case SHPT_POINTM:
            poDefn->SetGeomType( wkbPoint25D );
            break;

          case SHPT_ARC:
            poDefn->SetGeomType( wkbLineString );
            break;

          case SHPT_ARCZ:
          case SHPT_ARCM:
            poDefn->SetGeomType( wkbLineString25D );
            break;

          case SHPT_MULTIPOINT:
            poDefn->SetGeomType( wkbMultiPoint );
            break;

          case SHPT_MULTIPOINTZ:
          case SHPT_MULTIPOINTM:
            poDefn->SetGeomType( wkbMultiPoint25D );
            break;

          case SHPT_POLYGON:
            poDefn->SetGeomType( wkbPolygon );
            break;

          case SHPT_POLYGONZ:
          case SHPT_POLYGONM:
            poDefn->SetGeomType( wkbPolygon25D );
            break;
            
        }
    }

    return poDefn;
}

/************************************************************************/
/*                         SHPReadOGRFeature()                          */
/************************************************************************/

OGRFeature *SHPReadOGRFeature( SHPHandle hSHP, DBFHandle hDBF,
                               OGRFeatureDefn * poDefn, int iShape,
                               SHPObject *psShape, const char *pszSHPEncoding )

{
    if( iShape < 0 
        || (hSHP != NULL && iShape >= hSHP->nRecords)
        || (hDBF != NULL && iShape >= hDBF->nRecords) )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Attempt to read shape with feature id (%d) out of available"
                  " range.", iShape );
        return NULL;
    }

    if( hDBF && DBFIsRecordDeleted( hDBF, iShape ) )
    {
        CPLError( CE_Failure, CPLE_AppDefined, 
                  "Attempt to read shape with feature id (%d), but it is marked deleted.",
                  iShape );
        if( psShape != NULL )
            SHPDestroyObject(psShape);
        return NULL;
    }

    OGRFeature  *poFeature = new OGRFeature( poDefn );

/* -------------------------------------------------------------------- */
/*      Fetch geometry from Shapefile to OGRFeature.                    */
/* -------------------------------------------------------------------- */
    if( hSHP != NULL )
    {
        if( !poDefn->IsGeometryIgnored() )
        {
            OGRGeometry* poGeometry = NULL;
            poGeometry = SHPReadOGRObject( hSHP, iShape, psShape );

            /*
            * NOTE - mloskot:
            * Two possibilities are expected here (bot hare tested by GDAL Autotests):
            * 1. Read valid geometry and assign it directly.
            * 2. Read and assign null geometry if it can not be read correctly from a shapefile
            *
            * It's NOT required here to test poGeometry == NULL.
            */

            poFeature->SetGeometryDirectly( poGeometry );
        }
        else if( psShape != NULL )
        {
            SHPDestroyObject( psShape );
        }
    }

/* -------------------------------------------------------------------- */
/*      Fetch feature attributes to OGRFeature fields.                  */
/* -------------------------------------------------------------------- */

    for( int iField = 0; hDBF != NULL && iField < poDefn->GetFieldCount(); iField++ )
    {
        OGRFieldDefn* poFieldDefn = poDefn->GetFieldDefn(iField);
        if (poFieldDefn->IsIgnored() )
            continue;

        switch( poFieldDefn->GetType() )
        {
          case OFTString:
          {
              const char *pszFieldVal = 
                  DBFReadStringAttribute( hDBF, iShape, iField );
              if( pszFieldVal != NULL && pszFieldVal[0] != '\0' )
              {
                if( pszSHPEncoding[0] != '\0' )
                {
                    char *pszUTF8Field = CPLRecode( pszFieldVal,
                                                    pszSHPEncoding, CPL_ENC_UTF8);
                    poFeature->SetField( iField, pszUTF8Field );
                    CPLFree( pszUTF8Field );
                }
                else
                    poFeature->SetField( iField, pszFieldVal );
              }
          }
          break;

          case OFTInteger:
          case OFTInteger64:
          case OFTReal:
            if( !DBFIsAttributeNULL( hDBF, iShape, iField ) )
                poFeature->SetField( iField,
                                    DBFReadStringAttribute( hDBF, iShape,
                                                            iField ) );
            break;

          case OFTDate:
          {
              OGRField sFld;
              if( DBFIsAttributeNULL( hDBF, iShape, iField ) )
                  continue;

              const char* pszDateValue = 
                  DBFReadStringAttribute(hDBF,iShape,iField);

              /* Some DBF files have fields filled with spaces */
              /* (trimmed by DBFReadStringAttribute) to indicate null */
              /* values for dates (#4265) */
              if (pszDateValue[0] == '\0')
                  continue;

              memset( &sFld, 0, sizeof(sFld) );

              if( strlen(pszDateValue) >= 10 &&
                  pszDateValue[2] == '/' && pszDateValue[5] == '/' )
              {
                  sFld.Date.Month = (GByte)atoi(pszDateValue+0);
                  sFld.Date.Day   = (GByte)atoi(pszDateValue+3);
                  sFld.Date.Year  = (GInt16)atoi(pszDateValue+6);
              }
              else
              {
                  int nFullDate = atoi(pszDateValue);
                  sFld.Date.Year = (GInt16)(nFullDate / 10000);
                  sFld.Date.Month = (GByte)((nFullDate / 100) % 100);
                  sFld.Date.Day = (GByte)(nFullDate % 100);
              }
              
              poFeature->SetField( iField, &sFld );
          }
          break;

          default:
            CPLAssert( FALSE );
        }
    }

    if( poFeature != NULL )
        poFeature->SetFID( iShape );

    return( poFeature );
}

/************************************************************************/
/*                             GrowField()                              */
/************************************************************************/

static OGRErr GrowField(DBFHandle hDBF, int iField, OGRFieldDefn* poFieldDefn,
                        int nNewSize)
{
    char            szFieldName[20] = {};
    int             nOriWidth, nPrecision;
    char            chNativeType;
    /* DBFFieldType    eDBFType; */

    chNativeType = DBFGetNativeFieldType( hDBF, iField );
    /* eDBFType = */ DBFGetFieldInfo( hDBF, iField, szFieldName,
                                      &nOriWidth, &nPrecision );

    CPLDebug("SHAPE", "Extending field %d (%s) from %d to %d characters",
                iField, poFieldDefn->GetNameRef(), nOriWidth, nNewSize);

    if ( !DBFAlterFieldDefn( hDBF, iField, szFieldName,
                             chNativeType, nNewSize, nPrecision ) )
    {
        CPLError(CE_Failure, CPLE_AppDefined,
                    "Extending field %d (%s) from %d to %d characters failed",
                    iField, poFieldDefn->GetNameRef(), nOriWidth, nNewSize);
        return OGRERR_FAILURE;
    }
    else
    {
        poFieldDefn->SetWidth(nNewSize);
        return OGRERR_NONE;
    }
}

/************************************************************************/
/*                         SHPWriteOGRFeature()                         */
/*                                                                      */
/*      Write to an existing feature in a shapefile, or create a new    */
/*      feature.                                                        */
/************************************************************************/

OGRErr SHPWriteOGRFeature( SHPHandle hSHP, DBFHandle hDBF,
                           OGRFeatureDefn * poDefn, 
                           OGRFeature * poFeature,
                           const char *pszSHPEncoding,
                           int* pbTruncationWarningEmitted,
                           int bRewind )

{
#ifdef notdef
/* -------------------------------------------------------------------- */
/*      Don't write objects with missing geometry.                      */
/* -------------------------------------------------------------------- */
    if( poFeature->GetGeometryRef() == NULL && hSHP != NULL )
    {
        CPLError( CE_Failure, CPLE_AppDefined,
                  "Attempt to write feature without geometry not supported"
                  " for shapefile driver." );
        
        return OGRERR_UNSUPPORTED_GEOMETRY_TYPE;
    }
#endif

/* -------------------------------------------------------------------- */
/*      Write the geometry.                                             */
/* -------------------------------------------------------------------- */
    OGRErr      eErr;

    if( hSHP != NULL )
    {
        eErr = SHPWriteOGRObject( hSHP, (int)poFeature->GetFID(),
                                  poFeature->GetGeometryRef(), bRewind );
        if( eErr != OGRERR_NONE )
            return eErr;
    }
    
/* -------------------------------------------------------------------- */
/*      If there is no DBF, the job is done now.                        */
/* -------------------------------------------------------------------- */
    if( hDBF == NULL )
    {
/* -------------------------------------------------------------------- */
/*      If this is a new feature, establish it's feature id.            */
/* -------------------------------------------------------------------- */
        if( hSHP != NULL && poFeature->GetFID() == OGRNullFID )
            poFeature->SetFID( hSHP->nRecords - 1 );

        return OGRERR_NONE;
    }

/* -------------------------------------------------------------------- */
/*      If this is a new feature, establish it's feature id.            */
/* -------------------------------------------------------------------- */
    if( poFeature->GetFID() == OGRNullFID )
        poFeature->SetFID( DBFGetRecordCount( hDBF ) );

/* -------------------------------------------------------------------- */
/*      If this is the first feature to be written, verify that we      */
/*      have at least one attribute in the DBF file.  If not, create    */
/*      a dummy FID attribute to satisfy the requirement that there     */
/*      be at least one attribute.                                      */
/* -------------------------------------------------------------------- */
    if( DBFGetRecordCount( hDBF ) == 0 && DBFGetFieldCount( hDBF ) == 0 )
    {
        CPLDebug( "OGR", 
               "Created dummy FID field for shapefile since schema is empty.");
        DBFAddField( hDBF, "FID", FTInteger, 11, 0 );
    }

/* -------------------------------------------------------------------- */
/*      Write out dummy field value if it exists.                       */
/* -------------------------------------------------------------------- */
    if( DBFGetFieldCount( hDBF ) == 1 && poDefn->GetFieldCount() == 0 )
    {
        DBFWriteIntegerAttribute( hDBF, (int)poFeature->GetFID(), 0, 
                                  (int)poFeature->GetFID() );
    }

/* -------------------------------------------------------------------- */
/*      Write all the fields.                                           */
/* -------------------------------------------------------------------- */
    for( int iField = 0; iField < poDefn->GetFieldCount(); iField++ )
    {
        if( !poFeature->IsFieldSet( iField ) )
        {
            DBFWriteNULLAttribute( hDBF, (int)poFeature->GetFID(), iField );
            continue;
        }

        OGRFieldDefn* poFieldDefn = poDefn->GetFieldDefn(iField);

        switch( poFieldDefn->GetType() )
        {
          case OFTString:
          {
              const char *pszStr = poFeature->GetFieldAsString(iField);
              char *pszEncoded = NULL;
              if( strlen(pszSHPEncoding) > 0 )
              {
                  pszEncoded =
                      CPLRecode( pszStr, CPL_ENC_UTF8, pszSHPEncoding );
                  pszStr = pszEncoded;
              }

              int nStrLen = (int) strlen(pszStr);
              if (nStrLen > OGR_DBF_MAX_FIELD_WIDTH)
              {
                if (!(*pbTruncationWarningEmitted))
                {
                    *pbTruncationWarningEmitted = TRUE;
                    CPLError(CE_Warning, CPLE_AppDefined,
                            "Value '%s' of field %s has been truncated to %d characters.\n"
                            "This warning will not be emitted any more for that layer.",
                            poFeature->GetFieldAsString(iField),
                            poFieldDefn->GetNameRef(),
                            OGR_DBF_MAX_FIELD_WIDTH);
                }

                nStrLen = OGR_DBF_MAX_FIELD_WIDTH;

                if(pszEncoded != NULL && /* to please Coverity */
                   EQUAL(pszSHPEncoding, CPL_ENC_UTF8))
                {
                    const char *p = pszStr + nStrLen;
                    int byteCount = nStrLen;
                    while(byteCount > 0)
                    {
                        if( (*p & 0xc0) != 0x80 )
                        {
                            nStrLen = byteCount;
                            break;
                        }

                        byteCount--;
                        p--;
                    }

                    pszEncoded[nStrLen] = 0;
                }
              }

              if ( nStrLen > poFieldDefn->GetWidth() )
              {
                  if (GrowField(hDBF, iField, poFieldDefn, nStrLen) != OGRERR_NONE)
                  {
                      CPLFree( pszEncoded );
                      return OGRERR_FAILURE;
                  }
              }

              DBFWriteStringAttribute( hDBF, (int)poFeature->GetFID(), iField,
                                              pszStr );

              CPLFree( pszEncoded );
          }
          break;

          case OFTInteger:
          case OFTInteger64:
          {
              char szFormat[20];
              char szValue[32];
              int nFieldWidth = poFieldDefn->GetWidth();
              snprintf(szFormat, sizeof(szFormat), "%%%d" CPL_FRMT_GB_WITHOUT_PREFIX "d", MIN(nFieldWidth, (int)sizeof(szValue)-1));
              snprintf(szValue, sizeof(szValue), szFormat, poFeature->GetFieldAsInteger64(iField) );
              int nStrLen = static_cast<int>(strlen(szValue));
              if( nStrLen > nFieldWidth )
              {
                  if (GrowField(hDBF, iField, poFieldDefn, nStrLen) != OGRERR_NONE)
                  {
                      return OGRERR_FAILURE;
                  }
              }

              DBFWriteAttributeDirectly( hDBF, (int)poFeature->GetFID(), iField, 
                                         szValue );

              break;
          }

          case OFTReal:
          {
            double dfVal = poFeature->GetFieldAsDouble(iField);
            /* IEEE754 doubles can store exact values of all integers below 2^53 */
            if( poFieldDefn->GetPrecision() == 0 && fabs(dfVal) > ((GIntBig)1 << 53) )
            {
                static int nCounter = 0;
                if( nCounter <= 10 )
                {
                    CPLError( CE_Warning, CPLE_AppDefined,
                              "Value %.18g of field %s with 0 decimal of "
                              "feature " CPL_FRMT_GIB " is bigger than 2^53. "
                              "Precision loss likely occurred or going to "
                              "happen.%s",
                              dfVal, poFieldDefn->GetNameRef(),
                              poFeature->GetFID(),
                              (nCounter == 10) ? " This warning will not be "
                              "emitted anymore." : "");
                    nCounter ++;
                }
            }
            int ret = DBFWriteDoubleAttribute( hDBF, (int)poFeature->GetFID(), iField, 
                                               dfVal );
            if( !ret )
            {
                CPLError(CE_Warning, CPLE_AppDefined,
                         "Value %.18g of field %s of feature " CPL_FRMT_GIB " not successfully written. "
                         "Possibly due to too larger number with respect to field width",
                         dfVal, poFieldDefn->GetNameRef(), poFeature->GetFID());
            }
            break;
          }

          case OFTDate:
          {
              const OGRField* psField = poFeature->GetRawFieldRef(iField);

              if( psField->Date.Year < 0 || psField->Date.Year > 9999 )
              {
                  CPLError(CE_Warning, CPLE_NotSupported,
                          "Year < 0 or > 9999 is not a valid date for shapefile");
              }
              else
                  DBFWriteIntegerAttribute( hDBF, (int)poFeature->GetFID(), iField, 
                                            psField->Date.Year*10000 + psField->Date.Month*100 + psField->Date.Day );
          }
          break;

          default:
          {
              /* Ignore fields of other types */
              break;
          }
        }

    }

    return OGRERR_NONE;
}
