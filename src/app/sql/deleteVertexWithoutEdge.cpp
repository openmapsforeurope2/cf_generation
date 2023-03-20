#include <app/sql/deleteVertexWithoutEdge.h>

//EPG
#include <epg/Context.h>

//APP
#include <app/params/ThemeParameters.h>

namespace app{
namespace sql{

void deleteVertexWithoutEdge( )
{

	epg::Context* context=epg::ContextS::getInstance() ;


	std::string const& edgeTableName=context->getEpgParameters().getValue(EDGE_TABLE).toString() ;

	std::string const& vertexTableName=context->getEpgParameters().getValue(VERTEX_TABLE).toString();

	std::string const& idName=context->getEpgParameters().getValue(ID).toString();

	std::string const& edgeSourceName=context->getEpgParameters().getValue(EDGE_SOURCE).toString();

	std::string const& edgeTargetName=context->getEpgParameters().getValue(EDGE_TARGET).toString();


    context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY "+vertexTableName+" where "+vertexTableName+"."+idName+" IN (select "+context->getEpgParameters().getValue(ID).toString()+" from " +vertexTableName+" except(select "+vertexTableName+"."+idName+" from "+vertexTableName+","+edgeTableName+" where "+vertexTableName+"."+idName+"="+edgeTableName+"."+edgeSourceName+" or "+vertexTableName+"."+idName+"="+edgeTableName+"."+edgeTargetName+"))");

}


}
}
