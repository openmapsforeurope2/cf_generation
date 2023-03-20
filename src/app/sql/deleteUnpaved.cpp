#include <app/sql/deleteUnpaved.h>

//EPG
#include <epg/Context.h>

//APP
#include <app/params/ThemeParameters.h>

namespace app{
namespace sql{

void deleteUnpaved( )
{

	epg::Context* context=epg::ContextS::getInstance() ;

	std::string const& edgeTableName=context->getEpgParameters().getValue(EDGE_TABLE).toString() ;

	std::string const& netTypeName=context->getEpgParameters().getValue(NET_TYPE).toString() ;


	context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY " +edgeTableName+" WHERE etat_physique = 'Non revêtu' AND "+netTypeName+" < 10 ");

}


}
}
