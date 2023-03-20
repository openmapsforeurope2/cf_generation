#include <app/calcul/consistency/planarizeNetwork.h>

// BOOST
#include <boost/progress.hpp>

//EPG
#include <epg/context.h>
#include <epg/utils/splitEdge.h>
#include <epg/utils/createVertex.h>
#include <epg/sql/tools/UidGenerator.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/log/ScopeLogger.h>

//APP
#include <app/params/ThemeParameters.h>


namespace app{
namespace calcul{
namespace consistency{

		
	void planarizeNetwork(
		ign::feature::sql::FeatureStorePostgis* fsFranchissement,
		ign::feature::FeatureFilter filterProcessed
		)
	{

		epg::log::ScopeLogger log( "app::calcul::consistency::planarizeNetwork" );
		boost::timer timer;

		epg::Context* context = epg::ContextS::getInstance();

		epg::params::EpgParameters& params = context->getEpgParameters();
		std::string netTypeName = params.getValue( NET_TYPE ).toString();
		std::string countryCodeName = params.getValue( COUNTRY_CODE ).toString();
		std::string ficitiousName = params.getValue( VERTEX_FICTITIOUS ).toString();
		std::string geomName = params.getValue( GEOM ).toString();

		ign::feature::sql::FeatureStorePostgis* fsVertex = context->getFeatureStore( epg::VERTEX );
		ign::feature::sql::FeatureStorePostgis* fsEdge = context->getFeatureStore( epg::EDGE );

		epg::sql::tools::UidGenerator* idVerticesGenerator = new epg::sql::tools::UidGenerator( context->getPrefix() + "PLANARIZENODE", *fsVertex );
		epg::sql::tools::UidGenerator* idEdgesGenerator = new epg::sql::tools::UidGenerator( context->getPrefix() + "PLANARIZE", *fsEdge );

		ign::feature::FeatureIteratorPtr itFranchissement = fsFranchissement->getFeatures( filterProcessed );

		int nb_iter = epg::sql::tools::numFeatures( *fsFranchissement, filterProcessed );
		boost::progress_display display( nb_iter, std::cout, "planarizeNetwork for " + fsEdge->getTableName() + " % complete ]\n" );


		while ( itFranchissement->hasNext() ) {

			++display;

			ign::feature::Feature fFranchissement = itFranchissement->next();
			ign::geometry::Point ptFranchissement = fFranchissement.getGeometry().asPoint();

			ign::feature::FeatureFilter filterFranchissement;
			filterFranchissement.setExtent( ptFranchissement.getEnvelope());
			ign::feature::FeatureIteratorPtr itEdgeWithFranchissement = fsEdge->getFeatures( filterFranchissement );

			bool bNodeAdded = false;
			bool firstIteration = true;

			std::map<std::string, std::string> mVertexAttributes;
			mVertexAttributes.insert( std::make_pair( ficitiousName, "false" ) );

			while ( itEdgeWithFranchissement->hasNext() ) {

				ign::feature::Feature fEdge = itEdgeWithFranchissement->next();
				ign::geometry::LineString lsEdge = fEdge.getGeometry().asLineString();

				if ( ptFranchissement.distance( lsEdge ) > 0.00001 )
					continue;

				if ( lsEdge.startPoint().distance( ptFranchissement ) < 0.00001 || lsEdge.endPoint().distance( ptFranchissement ) < 0.00001 )
					continue;

				if ( !bNodeAdded ) 
				{					
					// Ajout d'un noeud s'il n'y en a pas
					bNodeAdded = true;					
					int netType = fEdge.getAttribute( netTypeName ).toInteger();
					std::string countryCode = fEdge.getAttribute( countryCodeName ).toString();
					mVertexAttributes.insert( std::make_pair( netTypeName, ign::data::Integer( netType ).toString() ) );
					mVertexAttributes.insert( std::make_pair( countryCodeName, countryCode ) );
					ign::feature::Feature newVertexRoad = epg::utils::createVertex( idVerticesGenerator, ptFranchissement, mVertexAttributes );
				}

				// Découpe de l'arc
				std::vector<ign::feature::Feature> vNewEdges = epg::utils::splitEdge( fEdge, ptFranchissement, idEdgesGenerator );

			}

		}

		delete idVerticesGenerator;
		delete idEdgesGenerator;
		log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() );

	}


}
}
}
