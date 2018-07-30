#include "tensionNode.h"
#include <maya/MGlobal.h>

const MString origAttrName( "origShape" );
const MString deformedAttrName( "deformedShape" );

MTypeId tensionNode::id( 0x00001 );
MObject tensionNode::aOrigShape;
MObject tensionNode::aDeformedShape;
MObject tensionNode::aOutShape;
MObject tensionNode::aColorRamp;

bool tensionNode::isOrigDirty;
bool tensionNode::isDeformedDirty;
MDoubleArray tensionNode::origEdgeLenArray;
MDoubleArray tensionNode::deformedEdgeLenArray;

MStatus tensionNode::initialize()
{
    MFnTypedAttribute tAttr;
    // MFnRampAttribute rAttr;

    aOrigShape = tAttr.create( origAttrName, origAttrName, MFnMeshData::kMesh );
    tAttr.setStorable( true );

    aDeformedShape = tAttr.create( deformedAttrName, deformedAttrName, MFnMeshData::kMesh );
    tAttr.setStorable( true );

    aOutShape = tAttr.create( "out", "out", MFnMeshData::kMesh );
    tAttr.setWritable( false );
    tAttr.setStorable( false );

    // aColorRamp = rAttr.createColorRamp( "color", "color" );
    aColorRamp = MRampAttribute::createColorRamp("color", "color");

    addAttribute( aOrigShape );
    addAttribute( aDeformedShape );
    addAttribute( aOutShape );
    addAttribute( aColorRamp );

    attributeAffects( aOrigShape, aOutShape );
    attributeAffects( aDeformedShape, aOutShape );
    attributeAffects( aColorRamp, aOutShape );

    return MStatus::kSuccess;
}

MStatus tensionNode::compute( const MPlug& plug, MDataBlock& data )
{
    MStatus status;

    if ( plug == aOutShape )
    {
        MObject thisObj = thisMObject();
        MFnDependencyNode nodeFn( thisObj );
        MDataHandle origHandle = data.inputValue( aOrigShape, &status );
        MDataHandle deformedHandle = data.inputValue( aDeformedShape, &status );
        MDataHandle outHandle = data.outputValue( aOutShape, &status );

        MRampAttribute colorAttribute( thisObj, aColorRamp, &status );
        float rampPosition = 0.25f;
        MColor color;

        colorAttribute.getColorAtPosition(rampPosition, color, &status);

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

        MObject outMesh = outHandle.asMesh();
        MFnMesh meshFn( outMesh, &status );

        int numVerts = meshFn.numVertices( &status );

        MColorArray vertColors;
        vertColors.setLength( numVerts );
        MIntArray vertIds;
        vertIds.setLength( numVerts );

        for ( int i = 0; i < numVerts; ++i)
        {
            double delta;
            MColor vertColor;
            if ( origEdgeLenArray.length() == deformedEdgeLenArray.length() )
            {
                delta = ( ( origEdgeLenArray[i] - deformedEdgeLenArray[i] ) / origEdgeLenArray[i] ) + 0.5;
            }
            else
            {
                delta = 0.5;
            }
            colorAttribute.getColorAtPosition(delta, vertColor, &status);
            vertColors.set( vertColor, i );
            vertIds.set( i, i );
        }
        meshFn.setVertexColors( vertColors, vertIds );
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
        lengthSum = lengthSum / connectedEdges.length();

        edgeLenArray.append( lengthSum );
        vertIter.next();
    }

    return edgeLenArray;
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
