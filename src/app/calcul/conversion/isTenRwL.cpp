#include<app/calcul/conversion/isTenRwL.h>

// BOOST
#include <boost/progress.hpp>

//SOCLE
#include <ign/graph/algorithm/DijkstraShortestPaths.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/tools/StringTools.h>
#include <epg/graph/EpgGraph.h>
#include <epg/graph/concept/EpgGraphSpecializations.h>


//APP
#include <app/params/ThemeParameters.h>


namespace app{
namespace calcul{
namespace conversion{

	void isTenRwL(ign::feature::sql::FeatureStorePostgis* fsRwLOld, ign::feature::sql::FeatureStorePostgis * fsRailRdCOld, ign::feature::sql::FeatureStorePostgis* fsConnectingPoint, ign::feature::sql::FeatureStorePostgis* fsRailRdC)
	{

		epg::log::ScopeLogger log("[ app::calcul::conversion::isTenRwL ]") ;
		boost::timer timer;

		epg::Context* context=epg::ContextS::getInstance();

		epg::params::EpgParameters & params=context->getEpgParameters();

		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		std::string edgeCodeName = params.getValue( EDGE_CODE ).toString();
		std::string linkedFeatureidName = params.getValue( LINKED_FEATURE_ID ).toString();
		std::string edgeSourceName = params.getValue( EDGE_SOURCE ).toString();
		std::string edgeTargetName = params.getValue( EDGE_TARGET ).toString();

		ign::feature::sql::FeatureStorePostgis * fsEdge= context->getFeatureStore( epg::EDGE);


		std::set<std::string> sEdgesOnTen;

		std::map<std::string/*idGare*/,ign::feature::Feature/*Feature*/> mRailRdCOldOnTen;



		//recuperation des gares du reseau TEN 

		ign::feature::FeatureFilter filterTen("ten=1" );

		ign::feature::FeatureIteratorPtr itTenOld = fsRwLOld->getFeatures( filterTen ) ;

		int nb_iter=epg::sql::tools::numFeatures(*fsRwLOld,filterTen);

		boost::progress_display display(nb_iter, std::cout, " Attribute TEN RailwaiL part 1/3 % complete ]\n") ;

		while ( itTenOld->hasNext() ) {
			++display;

			ign::feature::Feature fEdgeOld = itTenOld->next() ;

			ign::feature::FeatureFilter filter;
			filter.setExtent(fEdgeOld.getGeometry().getEnvelope());
			ign::feature::FeatureIteratorPtr itRailRdCOldOnTen= fsRailRdCOld->getFeatures( filter ) ;

			while (itRailRdCOldOnTen->hasNext() ) {
			
				ign::feature::Feature fRailRdCOldOnTen=itRailRdCOldOnTen->next();

				if( mRailRdCOldOnTen.find( fRailRdCOldOnTen.getId() ) != mRailRdCOldOnTen.end() )
					continue ;

				if( fEdgeOld.getGeometry().distance( fRailRdCOldOnTen.getGeometry() ) < 1 ) 

					mRailRdCOldOnTen[ fRailRdCOldOnTen.getId() ] = fRailRdCOldOnTen ;

			}
			
		}



		//appariement des gares appartenant au reseau TEN a l'annee n-1

		std::set<std::string/*idGare*/> sRailRdCApparieOnTen;

		IGN_ASSERT (fsRailRdC->getFeatureType ().hasAttribute("namn1"));

		double distMaxAppariement = 10000;

		boost::progress_display display2(mRailRdCOldOnTen.size(), std::cout, " Attribute TEN RailwaiL part 2/3 % complete ]\n") ;

		for( std::map<std::string,ign::feature::Feature>::iterator mit = mRailRdCOldOnTen.begin(); mit != mRailRdCOldOnTen.end(); ++mit ) {
			++display2;

			std::string idRailRdCApparie;

			IGN_ASSERT( mit->second.hasAttribute("namn1") );

			std::string namn1Attribute;

			std::vector<std::string> vNamn1AttributeParts;

			epg::tools::StringTools::Split( mit->second.getAttribute("namn1").toString(), "'", vNamn1AttributeParts);

			for(size_t i=0;i<vNamn1AttributeParts.size();++i) {

				IGN_ASSERT(!vNamn1AttributeParts[i].empty());

				if( vNamn1AttributeParts[i] == "'" ) 

					namn1Attribute+="'";

				namn1Attribute+=vNamn1AttributeParts[i];

			}

			ign::feature::FeatureFilter filterRailRdCApparie( "LOWER(namn1) = LOWER('"+namn1Attribute+"')" );
			
			ign::feature::FeatureIteratorPtr itRailRdC=fsRailRdC->getFeatures(filterRailRdCApparie);

			double distMin=distMaxAppariement;
			
			while (itRailRdC->hasNext()){
				
				ign::feature::Feature fRailRdC=itRailRdC->next();

				if( sRailRdCApparieOnTen.find(fRailRdC.getId())!= sRailRdCApparieOnTen.end())
					continue;

				double dist=mit->second.getGeometry().distance(fRailRdC.getGeometry());

				if(dist<distMin) {
					distMin=dist;
					idRailRdCApparie=fRailRdC.getId();
				}

			}
			
			if( distMin != distMaxAppariement)
				sRailRdCApparieOnTen.insert(idRailRdCApparie);

		}
	

		//ajout des connectings points du reseau ten comme point a relie par le reseau ten
		ign::feature::FeatureIteratorPtr itCPOnTen = fsConnectingPoint->getFeatures( filterTen ) ;

		while(itCPOnTen->hasNext()) {

			ign::feature::Feature fCPOnTen = itCPOnTen->next();

			IGN_ASSERT(fCPOnTen.hasAttribute(linkedFeatureidName));

			std::string idEdgeLinked= fCPOnTen.getAttribute(linkedFeatureidName).toString();

			ign::feature::Feature fEdge;
			fsEdge->getFeatureById(idEdgeLinked,fEdge);

			if(fEdge.getId().empty()) continue;

			sRailRdCApparieOnTen.insert(fEdge.getAttribute(edgeTargetName).toString());

			sRailRdCApparieOnTen.insert(fEdge.getAttribute(edgeSourceName).toString());
			
		}
		



		//calcul du reseau TEN entre les gares appartenant a ce reseau
		
		//creation du graphe
		epg::graph::EpgGraph graph;
		itTenOld = fsRwLOld->getFeatures( filterTen ) ;
		while ( itTenOld->hasNext() ) {

			ign::feature::Feature fEdgeOld = itTenOld->next() ;

			ign::feature::FeatureFilter filterEdgeGraph;
			filterEdgeGraph.setExtent(fEdgeOld.getGeometry().getEnvelope());
			ign::feature::FeatureIteratorPtr itEdgeGraph= fsEdge->getFeatures( filterEdgeGraph ) ;

			while ( itEdgeGraph->hasNext()) 
				graph.addFeatureEdge( itEdgeGraph->next() );
		}


		//Calcul des itineraires reliant les gares sur ce graphe
		boost::progress_display display3(sRailRdCApparieOnTen.size(), std::cout, " Attribute TEN RailwaiL part 3/3 % complete ]\n") ;
		std::set<std::string> sGareVisited;

		for( std::set<std::string>::iterator sit=sRailRdCApparieOnTen.begin();sit!=sRailRdCApparieOnTen.end();++sit) {
			++display3;

			sGareVisited.insert(*sit);

			if(! graph.getVertexById(*sit).first ) {
				log.log( epg::log::WARN,"la gare " + *sit + " n'est pas trouve dans le graphe contenant TEN");
				continue;
			}

			epg::graph::EpgGraph::vertex_descriptor vGareOnTen = graph.getVertexById(*sit).second;
				
			ign::graph::algorithm::DijkstraShortestPaths<epg::graph::EpgGraph> dijkOnTen(graph,true);
			dijkOnTen.computePaths( vGareOnTen ) ;

			for( std::set<std::string>::iterator sitToVisit=sRailRdCApparieOnTen.begin();sitToVisit!=sRailRdCApparieOnTen.end();++sitToVisit) {
				
				if( sGareVisited.find(*sitToVisit)!=sGareVisited.end() ) continue;

				if(! graph.getVertexById(*sitToVisit).first ) {
					log.log( epg::log::WARN,"la gare " + *sitToVisit + " n'est pas trouve dans le graphe contenant TEN");
					continue;
				}
				
				epg::graph::EpgGraph::edges_path pathOnTen;
				epg::graph::EpgGraph::vertex_descriptor vGareTarget = graph.getVertexById(*sitToVisit).second;
				dijkOnTen.getPath(vGareOnTen,vGareTarget,pathOnTen);

				for(epg::graph::EpgGraph::edges_path_iterator epit=pathOnTen.begin();epit!=pathOnTen.end();++epit) 
					sEdgesOnTen.insert(graph.getId(epit->descriptor));	
			}
		}


		for( std::set<std::string>::iterator sit = sEdgesOnTen.begin(); sit != sEdgesOnTen.end(); ++sit ){

			ign::feature::Feature fEdge;
			fsEdge->getFeatureById(*sit,fEdge);
			IGN_ASSERT( fEdge.hasAttribute("ten") ) ;
			fEdge.setAttribute("ten",ign::data::Integer(1));
			fsEdge->modifyFeature(fEdge);
		}
	    
		log.log( epg::log::INFO, "nombre d arcs avec un ten=1 :" + ign::data::Integer( sEdgesOnTen.size() ).toString() );
		log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() ) ;

	}

}
}
}