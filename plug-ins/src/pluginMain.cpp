#include <maya/MFnPlugin.h>
#include "tensionNode.h"

MStatus initializePlugin( MObject obj )
{
    MStatus status;
    MFnPlugin plugin( obj, "", "", "Any");
    status = plugin.registerNode( "tensionNode", tensionNode::id, tensionNode::creator, tensionNode::initialize );
    if ( !status ) {
        status.perror( "registerNode" );
        return status; }
    return status;
}

MStatus uninitializePlugin( MObject obj )
{
    MStatus status;
    MFnPlugin plugin( obj );
    status = plugin.deregisterNode( tensionNode::id );
    if ( !status ) {
        status.perror( "deregisterNode" );
        return status; }
    return status;
}
