#include "tensionNode.h"
#include <maya/MGlobal.h>

const MString origAttrName( "origShape" );
const MString deformedAttrName( "deformedShape" );

MTypeId tensionNode::id( 0x00001 );
MObject tensionNode::origShape;
MObject tensionNode::deformedShape;
MObject tensionNode::outShape;
bool tensionNode::isOrigDirty;
bool tensionNode::isDeformedDirty;
MDoubleArray tensionNode::origEdgeLenArray;
MDoubleArray tensionNode::deformedEdgeLenArray;

MStatus tensionNode::initialize()
{
    MFnTypedAttribute tAttr;

    origShape = tAttr.create( origAttrName, origAttrName, MFnMeshData::kMesh );
    tAttr.setStorable( true );

    deformedShape = tAttr.create( deformedAttrName, deformedAttrName, MFnMeshData::kMesh );
    tAttr.setStorable( true );

    outShape = tAttr.create( "out", "out", MFnMeshData::kMesh );
    tAttr.setWritable( false );
    tAttr.setStorable( false );

    addAttribute( origShape );
    addAttribute( deformedShape );
    addAttribute( outShape );

    attributeAffects(origShape, outShape);
    attributeAffects(deformedShape, outShape);

    return MStatus::kSuccess;
}

MStatus tensionNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus status;

    if ( plug == outShape )
    {
        MFnDependencyNode nodeFn(thisMObject());
        MDataHandle origHandle = data.inputValue( origShape, &status );
        MDataHandle deformedHandle = data.inputValue( deformedShape, &status );
        MDataHandle outHandle = data.outputValue( outShape, &status );

        if ( isOrigDirty == true )
        {
            origEdgeLenArray = getEdgeLen( origHandle );
        }
        if ( isDeformedDirty == true )
        {
            deformedEdgeLenArray = getEdgeLen( deformedHandle );
        }

        outHandle.copy( deformedHandle );
        outHandle.set( deformedHandle.asMesh() );

        if ( origEdgeLenArray.length() == deformedEdgeLenArray.length() )
        {
            MObject outMesh = outHandle.asMesh();
            MFnMesh meshFn( outMesh, &status );

            int numVerts = meshFn.numVertices( &status );

            MColorArray vertColors;
            vertColors.setLength( numVerts );
            MIntArray vertIds;
            vertIds.setLength( numVerts );

            for ( int i = 0; i < numVerts; ++i)
            {
                double delta = origEdgeLenArray[i] - deformedEdgeLenArray[i];
                MColor vertColor;
                if ( delta > 0 )
                {
                    vertColor = MColor( delta, 0, 0 );
                }
                else
                {
                    vertColor = MColor( 0, -delta, 0 );
                }
                vertColors.set( vertColor, i );
                // cout << vertColor << endl;

                vertIds.set( i, i );
            }

            meshFn.setVertexColors( vertColors, vertIds );
        }
    }
    data.setClean( plug );
    return MStatus::kSuccess;
}

MDoubleArray tensionNode::getEdgeLen( const MDataHandle& meshHandle )
{
    MStatus status;
    int dummy;
    MDoubleArray edgeLenArray;

    MObject meshObj = meshHandle.asMesh();
    MItMeshEdge edgeIter( meshObj, &status );
    MItMeshVertex vertIter( meshObj,  &status );

    while ( !vertIter.isDone() )
    {
        double lengthSum = 0.0;
        MIntArray connectedEdges;
        vertIter.getConnectedEdges( connectedEdges );
        for ( int i = 0; i < connectedEdges.length(); i++)
        {
            double length;
            edgeIter.setIndex( connectedEdges[i], dummy );
            edgeIter.getLength( length );
            lengthSum += length;
        }
        edgeLenArray.append( lengthSum );
        vertIter.next();
    }

    // char buffer[256];
    // sprintf( buffer, "le %u", edgeLenArray.length());
    // MGlobal::displayInfo( buffer );

    return edgeLenArray;
}

MStatus tensionNode::setColor( const MObject &outMesh )
{
    MStatus status;

    for ( int i = 0; i < origEdgeLenArray.length(); i++)
    {
        double scaleFactor = origEdgeLenArray[i] / deformedEdgeLenArray[i] - 1;
        cout << "scale factor" << scaleFactor << endl;

    }


    return status;

}

MStatus tensionNode::setDependentsDirty( const MPlug &dirtyPlug, MPlugArray &affectedPlugs )
// set isOrigDirty and/or isDeformedDirty
{
    if ( dirtyPlug.partialName() == deformedAttrName )
    {
        isDeformedDirty = true;
    }
    else
    {
        isDeformedDirty = false;
    }

    if ( dirtyPlug.partialName() == origAttrName )
    {
        isOrigDirty = true;
    }
    else
    {
        isOrigDirty = false;
    }

    return MStatus::kSuccess;

}
