#include <app/calcul/selection/linkPoi.h>

// BOOST
#include <boost/progress.hpp>

//SOCLE IGN
#include <ign/geometry/Envelope.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/log/ShapeLogger.h>
#include <app/params/ThemeParameters.h>
#include <epg/sql/tools/numFeatures.h>


namespace app{
namespace calcul{
namespace selection{

	void linkPoi(
		std::string poiTable,
		std::string poiId,
		std::string poiGeom, 
		double linkThreshold, 
		std::set< std::pair< std::string, std::string > > & sStartIti,
		int netTypeMainPoi,
		bool verbose
		)
	{

		epg::Context* context=epg::ContextS::getInstance();
		epg::log::ScopeLogger log("[ app::calcul::selection::linkPoi table: "+ poiTable +" threshold: "+ign::data::Double(linkThreshold).toString() +"]");


		epg::params::EpgParameters const& params = context->getEpgParameters();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		std::string const& linkedFeatidName = params.getValue(LINKED_FEATURE_ID).toString();
		std::string edgeLinkableName = params.getValue(EDGE_LINKABLE).toString();
		ign::feature::FeatureFilter filterLinkedFeatId = ign::feature::FeatureFilter( linkedFeatidName + " = ''" );

		ign::feature::sql::FeatureStorePostgis* fsEdge = context->getFeatureStore(epg::EDGE);
		ign::feature::sql::FeatureStorePostgis* fsPoi = context->getDataBaseManager().getFeatureStore( poiTable, poiId, poiGeom);

		int nb_iter = epg::sql::tools::numFeatures(*fsPoi, filterLinkedFeatId);
		log.log(epg::log::INFO, "Nombre d'objets à lier : " + ign::data::Integer(nb_iter).toString());

		// S'il n'y a pas d'objets à lier, on sort
		if (nb_iter == 0)
			return;

		epg::log::ShapeLogger* shapeLogger = epg::log::ShapeLoggerS::getInstance();
		if (verbose) shapeLogger->addShape("notLinkedPoi" + ign::data::Double(linkThreshold).toString(), epg::log::ShapeLogger::POINT);

		ign::feature::FeatureIteratorPtr it = fsPoi->getFeatures( filterLinkedFeatId );
		boost::progress_display display(nb_iter, std::cout, "[link table " + poiTable + " with a threshold of " + ign::data::Double(linkThreshold).toString()+" running % complete ]\n") ;

		while(it->hasNext())
		{
			++display;
			ign::feature::Feature featPoi = it->next();

			std::string idLinkedEdge = "";			
			double distMin = linkThreshold + 1;

			//Recuperation de la zone de recherche des edges en fonction du seuil
			ign::geometry::Envelope envPoi;
			featPoi.getGeometry().envelope(envPoi);
			envPoi.expandBy(linkThreshold);

//2022			ign::feature::FeatureFilter filterSelec( params.getValue(NET_TYPE).toString() + " > 9 and isLinkable = true and etat_physique != 'Non revêtu'" );
			ign::feature::FeatureFilter filterSelec(params.getValue(NET_TYPE).toString() + " > 9 and " + edgeLinkableName + " = true");

			filterSelec.setExtent(envPoi);
			ign::feature::FeatureIteratorPtr eit = fsEdge->getFeatures(filterSelec);

			// Recherche des edges selectionnes a une distance inferieur au seuil
			while(eit->hasNext())
			{				
				ign::feature::Feature edge = eit->next();
				double dist = featPoi.getGeometry().asPoint().distance(edge.getGeometry().asLineString());

				if(dist<distMin)
				{					
					distMin=dist;
					idLinkedEdge=edge.getId();
				}
			}


			if ( idLinkedEdge != "" )
			{
				featPoi.setAttribute( linkedFeatidName , ign::data::String( idLinkedEdge ) ) ;
				fsPoi->modifyFeature(featPoi);

				ign::feature::Feature edgeLinked;
				fsEdge->getFeatureById(idLinkedEdge,edgeLinked);

				edgeLinked.setAttribute( params.getValue(NET_TYPE).toString() , ign::data::Integer( netTypeMainPoi ) ) ;
				fsEdge->modifyFeature(edgeLinked);

				continue;
			}

			// Recherche des edges non selectionne a une distance inferieur au seuil
//			ign::feature::FeatureFilter filterNoSelec( params.getValue(NET_TYPE).toString() + " < 10 and isLinkable = true and etat_physique != 'Non revêtu'");
			ign::feature::FeatureFilter filterNoSelec(params.getValue(NET_TYPE).toString() + " < 10 and " + edgeLinkableName + " = true");

			filterNoSelec.setExtent(envPoi);
			eit = fsEdge->getFeatures(filterNoSelec);

			while(eit->hasNext())
			{			
				ign::feature::Feature edge=eit->next();
				double dist = featPoi.getGeometry().asPoint().distance(edge.getGeometry().asLineString());

				if(dist<distMin)
				{
					distMin=dist;
					idLinkedEdge=edge.getId();
				}
			}

			//pas d'edge trouve a distance du seuil
			if ( idLinkedEdge == "" )
			{
				log.log( epg::log::INFO, "No linkable edge for the poi " + featPoi.getId() + " at a distance of " + ign::data::Double(linkThreshold).toString() );				
				if( verbose ) shapeLogger->writeFeature( "notLinkedPoi" + ign::data::Double(linkThreshold).toString(), featPoi );			
				continue;
			}

			//Ajout de l'edge et de son noeud initial et noeud final dans le set de depart de calcul des itineraires de rattachement
			ign::feature::Feature edge ;

			fsEdge->getFeatureById(idLinkedEdge,edge) ;

			std::string idEdgeSource = edge.getAttribute( params.getValue(EDGE_SOURCE).toString() ).toString() ;
			std::string idEdgeTarget = edge.getAttribute( params.getValue(EDGE_TARGET).toString() ).toString() ;

			sStartIti.insert( make_pair( idEdgeSource, idLinkedEdge ) ) ;
			sStartIti.insert( make_pair( idEdgeTarget, idLinkedEdge ) ) ;

			featPoi.setAttribute( linkedFeatidName , ign::data::String( idLinkedEdge ) );
			fsPoi->modifyFeature(featPoi);			
		}

		if( verbose ) shapeLogger->closeShape( "notLinkedPoi"+ign::data::Double(linkThreshold).toString() );

	}


}
}
}
