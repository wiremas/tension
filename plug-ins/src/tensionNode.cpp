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


MStatus initialize_ramp( MObject parentNode, MObject rampObj, int index, float position, MColor value, int interpolation )
// initialize color ramp values
{
    MStatus status;

    MPlug rampPlug( parentNode, rampObj );

    MPlug elementPlug = rampPlug.elementByLogicalIndex( index, &status );

    MPlug positionPlug = elementPlug.child(0, &status);
    status = positionPlug.setFloat(position);

    MPlug valuePlug = elementPlug.child(1);
    status = valuePlug.child(0).setFloat(value.r);
    status = valuePlug.child(1).setFloat(value.g);
    status = valuePlug.child(2).setFloat(value.b);

    MPlug interpPlug = elementPlug.child(2);
    interpPlug.setInt(interpolation);

    return MS::kSuccess;
}

void tensionNode::postConstructor()
{
    MStatus status;
    initialize_ramp( tensionNode::thisMObject(), aColorRamp, 0, 0.0f, MColor( 0, 1, 0 ), 1 );
    initialize_ramp( tensionNode::thisMObject(), aColorRamp, 1, 0.5f, MColor( 0, 0, 0 ), 1 );
    initialize_ramp( tensionNode::thisMObject(), aColorRamp, 2, 1.0f, MColor( 1, 0, 0 ), 1 );
}

MStatus tensionNode::initialize()
{
    MFnTypedAttribute tAttr;

    aOrigShape = tAttr.create( origAttrName, origAttrName, MFnMeshData::kMesh );
    tAttr.setStorable( true );

    aDeformedShape = tAttr.create( deformedAttrName, deformedAttrName, MFnMeshData::kMesh );
    tAttr.setStorable( true );

    aOutShape = tAttr.create( "out", "out", MFnMeshData::kMesh );
    tAttr.setWritable( false );
    tAttr.setStorable( false );

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
        MDataHandle origHandle = data.inputValue( aOrigShape, &status );
        MCheckStatus( status, "ERR: getting data handle" );
        MDataHandle deformedHandle = data.inputValue( aDeformedShape, &status );
        MCheckStatus( status, "ERR: getting data handle" );
        MDataHandle outHandle = data.outputValue( aOutShape, &status );
        MCheckStatus( status, "ERR: getting data handle" );
        MRampAttribute colorAttribute( thisObj, aColorRamp, &status );
        MCheckStatus( status, "ERR: getting color attribute" );

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
        MCheckStatus( status, "ERR: getting meshfn" );
        int numVerts = meshFn.numVertices( &status );
        MCheckStatus( status, "ERR: getting vert count" );

        MColorArray vertColors;
        MIntArray vertIds;

        MCheckStatus( vertColors.setLength( numVerts ), "ERR: setting array length" );
        MCheckStatus( vertIds.setLength( numVerts ), "ERR: setting array length" );

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
            MCheckStatus( status, "ERR: getting color ramp attribute" );
            vertColors.set( vertColor, i );
            vertIds.set( i, i );
        }
        MCheckStatus( meshFn.setVertexColors( vertColors, vertIds ), "ERR: setting vertex colors" );
    }
    data.setClean( plug );
    return MStatus::kSuccess;
}

MDoubleArray tensionNode::getEdgeLen( const MDataHandle& meshHandle )
// iterate over each vertex, get all connected edge lengths, sum them up and
// append them to the MDoubleArray that will be returned.
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
