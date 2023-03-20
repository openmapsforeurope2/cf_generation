#include <app/sql/loadLinkable.h>

#include <epg/Context.h>
#include <app/params/ThemeParameters.h>

namespace app{
namespace sql{


void loadLinkable( )
{
	epg::Context* context=epg::ContextS::getInstance();

	std::string const& edgeTableName=context->getEpgParameters().getValue(EDGE_TABLE).toString();

	std::string const& vertexTableName=context->getEpgParameters().getValue(VERTEX_TABLE).toString();

	std::string const& idName=context->getEpgParameters().getValue(ID).toString();

	std::string const& edgeSourceName=context->getEpgParameters().getValue(EDGE_SOURCE).toString();

	std::string const& edgeTargetName=context->getEpgParameters().getValue(EDGE_TARGET).toString();


    context->getDataBaseManager().getConnection()->update("UPDATE "+edgeTableName+" SET isLinkable='1' WHERE isLinkable!='1' AND vocation != 'Type autoroutier' AND vocation != 'Bretelle' AND acces ='Libre' AND position_par_rapport_au_sol='Au sol' AND etat_physique !='En construction' AND etat_physique != 'Non revêtu' and etat_de_l_objet!='En construction' ");

	//rattachement que sur un troncon maintenant donc les vertex n'on plus besoin d'attribut isLinkable
/*
    context->getDataBaseManager().getConnection()->update("UPDATE "+vertexTableName+" SET isLinkable='1' WHERE isLinkable!='1' AND nature !='Barrière' AND nature !='Changement d''attribut' AND nature !='Noeud de communication restreinte' AND nature !='Echangeur partiel' AND nature !='Coupure arbitraire' AND nature !='Embarcadère' AND nature !='Echangeur complet' AND nature !='Barrière de douane' AND nature !='Carrefour avec toboggan ou passage inférieur'");

    context->getDataBaseManager().getConnection()->update("UPDATE "+vertexTableName+" SET isLinkable = '0' from "+edgeTableName+" WHERE "+vertexTableName+".isLinkable='1' AND "+edgeTableName+".isLinkable = '0' AND "+vertexTableName+"."+idName+"="+edgeTableName+"."+edgeSourceName);
    context->getDataBaseManager().getConnection()->update("UPDATE "+vertexTableName+" SET isLinkable = '0' from "+edgeTableName+" WHERE "+vertexTableName+".isLinkable='1' AND "+edgeTableName+".isLinkable = '0' AND "+vertexTableName+"."+idName+"="+edgeTableName+"."+edgeTargetName);
*/
}


}
}

