#include <app/sql/deleteBuildingAndProjected.h>

//EPG
#include <epg/Context.h>

//APP
#include <app/params/ThemeParameters.h>

namespace app{
namespace sql{

void deleteBuildingAndProjected( )
{

	epg::Context* context=epg::ContextS::getInstance() ;

	std::string const& edgeTableName=context->getEpgParameters().getValue(EDGE_TABLE).toString() ;

	std::string const& netTypeName=context->getEpgParameters().getValue(NET_TYPE).toString() ;


	context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY " +edgeTableName+" WHERE etat_de_l_objet='En construction' AND "+netTypeName+" < 10 ");
	//context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY " +edgeTableName+" WHERE (etat_de_l_objet='En construction' or etat_de_l_objet='En projet') AND "+netTypeName+" < 10 ");

}


}
}
