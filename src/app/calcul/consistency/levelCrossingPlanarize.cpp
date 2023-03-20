#include <app/calcul/consistency/levelCrossingPlanarize.h>

// BOOST
#include <boost/progress.hpp>

//EPG
#include <epg/context.h>
#include <epg/utils/splitEdge.h>
#include <epg/utils/createVertex.h>
#include <epg/sql/tools/UidGenerator.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/tools/FilterTools.h>

//APP
#include <app/params/ThemeParameters.h>


namespace app{
namespace calcul{
namespace consistency{

	void levelCrossingPlanarize(
		ign::feature::sql::FeatureStorePostgis* fsLevelCC,
		ign::feature::sql::FeatureStorePostgis* fsRailwayL,
		ign::feature::sql::FeatureStorePostgis* fsRailwayNode,
		ign::feature::FeatureFilter filterProcessed
		)
	{

		double threshold = 1e-5;

		epg::Context* context = epg::ContextS::getInstance();

		epg::params::EpgParameters& params = context->getEpgParameters();
		std::string edgeSourceName = params.getValue( EDGE_SOURCE ).toString();
		std::string edgeTargetName = params.getValue( EDGE_TARGET ).toString();
		std::string netTypeName = params.getValue( NET_TYPE ).toString();
		std::string countryCodeName = params.getValue( COUNTRY_CODE ).toString();
		std::string ficitiousName = params.getValue( VERTEX_FICTITIOUS ).toString();
		std::string geomName = params.getValue( GEOM ).toString();


		ign::feature::sql::FeatureStorePostgis* fsRoadNode = context->getFeatureStore( epg::VERTEX );

		ign::feature::sql::FeatureStorePostgis* fsRoad = context->getFeatureStore( epg::EDGE );


		epg::sql::tools::UidGenerator* idVerticesRoadGenerator = new epg::sql::tools::UidGenerator( context->getPrefix() + "LVLCCROADNODE", *fsRoadNode );

		epg::sql::tools::UidGenerator* idEdgesRoadGenerator = new epg::sql::tools::UidGenerator( context->getPrefix() + "LVLCCROAD", *fsRoad );

		epg::sql::tools::UidGenerator* idVerticesRailwayGenerator = new epg::sql::tools::UidGenerator( context->getPrefix() + "LVLCCRAILNODE", *fsRailwayNode );

		epg::sql::tools::UidGenerator* idEdgesRailwayGenerator = new epg::sql::tools::UidGenerator( context->getPrefix() + "LVLCCRAIL", *fsRailwayL );


		std::map<std::string, std::string> mVertexAttributes;

		mVertexAttributes.insert( std::make_pair( ficitiousName, "false" ) );


		ign::feature::FeatureFilter filter = filterProcessed;
		epg::tools::FilterTools::addAndConditions( filter, "f_code = 'AQ062'" );

		ign::feature::FeatureIteratorPtr itLevelCC = fsLevelCC->getFeatures( filter );

		int nb_iter = epg::sql::tools::numFeatures( *fsLevelCC, filter );

		boost::progress_display display( nb_iter, std::cout, "levelCrossingPlanarize % complete ]\n" );


		while ( itLevelCC->hasNext() ) {

			++display;

			ign::feature::Feature fLevelCC = itLevelCC->next();

			//std::string condLevelCCIntersect="st_distance("+geomName + ",'"+fLevelCC.getGeometry().asText()+"')<0.00001";
			//ign::feature::FeatureFilter filterLevelCC(condLevelCCIntersect);
			ign::feature::FeatureFilter filterLevelCC;
			filterLevelCC.setExtent( fLevelCC.getGeometry().asPoint().getEnvelope().expandBy( threshold ) );


			params.setParameter( EDGE_TABLE, ign::data::String( fsRoad->getTableName() ) );

			params.setParameter( VERTEX_TABLE, ign::data::String( fsRoadNode->getTableName() ) );


			ign::feature::FeatureIteratorPtr itVertexRoad = fsRoadNode->getFeatures( filterLevelCC );

			if ( !itVertexRoad->hasNext() ) {

				ign::feature::FeatureIteratorPtr itEdgeRoad = fsRoad->getFeatures( filterLevelCC );

				IGN_ASSERT( itEdgeRoad->hasNext() );

				while ( itEdgeRoad->hasNext() ) {

					ign::feature::Feature edge = itEdgeRoad->next();

					if ( fLevelCC.getGeometry().asPoint().distance( edge.getGeometry() ) > threshold ) continue;

					int netType = edge.getAttribute( netTypeName ).toInteger();

					std::string countryCode = edge.getAttribute( countryCodeName ).toString();

					mVertexAttributes.insert( std::make_pair( netTypeName, ign::data::Integer( netType ).toString() ) );

					mVertexAttributes.insert( std::make_pair( countryCodeName, countryCode ) );

					ign::feature::Feature newVertexRoad = epg::utils::createVertex( idVerticesRoadGenerator, fLevelCC.getGeometry().asPoint(), mVertexAttributes );

					std::vector<ign::feature::Feature> vNewEdges = epg::utils::splitEdge( edge, newVertexRoad.getGeometry().asPoint(), idEdgesRoadGenerator );

				}

			}


			params.setParameter( EDGE_TABLE, ign::data::String( fsRailwayL->getTableName() ) );

			params.setParameter( VERTEX_TABLE, ign::data::String( fsRailwayNode->getTableName() ) );


			ign::feature::FeatureIteratorPtr itVertexRailway = fsRailwayNode->getFeatures( filterLevelCC );

			if ( !itVertexRailway->hasNext() ) {

				ign::feature::FeatureIteratorPtr itEdgeRailway = fsRailwayL->getFeatures( filterLevelCC );

				IGN_ASSERT( itEdgeRailway->hasNext() );

				while ( itEdgeRailway->hasNext() ) {

					ign::feature::Feature edge = itEdgeRailway->next();

					if ( fLevelCC.getGeometry().asPoint().distance( edge.getGeometry() ) > threshold ) continue;

					int netType = edge.getAttribute( netTypeName ).toInteger();

					std::string countryCode = edge.getAttribute( countryCodeName ).toString();

					mVertexAttributes.insert( std::make_pair( netTypeName, ign::data::Integer( netType ).toString() ) );

					mVertexAttributes.insert( std::make_pair( countryCodeName, countryCode ) );

					ign::feature::Feature newVertexRailway = epg::utils::createVertex( idVerticesRailwayGenerator, fLevelCC.getGeometry().asPoint(), mVertexAttributes );

					std::vector<ign::feature::Feature> vNewEdges = epg::utils::splitEdge( edge, newVertexRailway.getGeometry().asPoint(), idEdgesRailwayGenerator );

				}

			}


		}


		delete idVerticesRoadGenerator;
		delete idVerticesRailwayGenerator;
		delete idEdgesRoadGenerator;
		delete idEdgesRailwayGenerator;
		
	}


}
}
}
