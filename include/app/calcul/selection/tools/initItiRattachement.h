#ifndef _APP_CALCUL_SELECTION_TOOLS_INITITIRATTACHEMENT_H_
#define _APP_CALCUL_SELECTION_TOOLS_INITITIRATTACHEMENT_H_

// BOOST
#include<boost/progress.hpp>

//SOCLE
#include <ign/feature/FeatureFilter.h>
#include <ign/feature/sql/FeatureIteratorPostgis.h>


//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/graph/EpgGraph.h>
#include <epg/graph/concept/netType.h>
#include <epg/utils/numFeatures.h>




namespace app{
namespace calcul{
namespace selection{
namespace tools{

template< typename GraphType >
void initItiRattachement(std::string nametablPtInt,std::string idNamePtInt,std::string geomNamePtInt,GraphType & graph,std::set< std::pair< std::string/*vertexStart*/, std::string/*edgeStart*/ > > & sStartIti )
{
    boost::timer timer;

    typedef typename GraphType::vertex_descriptor     vertex_descriptor;
    typedef typename GraphType::edge_descriptor       edge_descriptor;

    epg::Context* context=epg::ContextS::getInstance();
    epg::log::ScopeLogger log = ("[ app::calcul::selection::tools::initItiRattachement "+nametablPtInt+" ]");
	epg::params::EpgParameters params=context->getEpgParameters();

    //recuperation des pts d'interet
    ign::feature::sql::FeatureStorePostgis fsPtInt(*context->getDataBaseManager().getConnection());
    fsPtInt.load(nametablPtInt,idNamePtInt,geomNamePtInt);

    ign::feature::sql::FeatureIteratorPostgis itPtInt(fsPtInt,ign::feature::FeatureFilter());

    int nb_iter = epg::utils::numFeatures( nametablPtInt, nametablPtInt, ign::feature::FeatureFilter() );

    boost::progress_display display(nb_iter, std::cout, "[initItiRattachement "+nametablPtInt+" % complete ]\n") ;

    while(itPtInt.hasNext())
    {
        ++display;

        //recuperation du rattachement
        ign::feature::Feature ptInt=itPtInt.next();
        //ign::geometry::Point geomPtInt=ptInt.getGeometry().asPoint();
        std::string idRattach=ptInt.getAttribute(params.getValue(LINKED_FEATURE_ID)).toString();

        if(idRattach==""){//pas de rattachement
            log.write("pas de rattachement pour le poi "+ptInt.getId());
            continue;
        }

        //cas du rattachement sur du res pr
        if(idRattach.substr(0,8)=="BDCTRORO")//si rattachement sur troncon
        {
            if (! graph.getEdgeById(idRattach).first)//pas dans le graphe dc principal
                continue;

            edge_descriptor edgeRattach=graph.getEdgeById(idRattach).second;
            sStartIti.insert(std::make_pair(graph.getId(graph.source(edgeRattach)),graph.getId(edgeRattach)));
            sStartIti.insert(std::make_pair(graph.getId(graph.target(edgeRattach)),graph.getId(edgeRattach)));
        }

        else//si rattachement sur un noeud
        {
            if(! graph.getVertexById(idRattach).first)//pas dans le graphe dc principal
                continue;


            vertex_descriptor ndRattach=graph.getVertexById(idRattach).second;

            if(epg::graph::concept::netType(graph[ndRattach])>9)//sur du reseau selectionne
                continue;

            std::vector<edge_descriptor> vIncident=graph.incidentEdges(ndRattach);

            for (size_t i=0;i<vIncident.size();++i) {
                edge_descriptor edgeStart=vIncident[i];
                vertex_descriptor ndStart = (graph.source(edgeStart) == ndRattach )? graph.target(edgeStart) : graph.source(edgeStart) ;
                sStartIti.insert(std::make_pair(graph.getId(ndStart),graph.getId(edgeStart)));
            }
        }


        //cas du rattachement sur du res local->calcul iti de rattachement

    }
    log.write ("taille du set de depart :" + ign::data::Integer( sStartIti.size() ).toString() ) ;
    log.write ("execution time :" + ign::data::Integer( timer.elapsed() ).toString() ) ;

};

}
}
}
}

#endif
