#include <app/calcul/matching/applyDisplacementOnTransNetwork.h>

// BOOST
#include <boost/progress.hpp>

//EPG
#include <epg/context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/calcul/matching/MatchingWithConnectingFeatures.h>
#include <epg/tools/FilterTools.h>
#include <epg/utils/TableCreator.h>
#include <epg/utils/updateVertexCountryCodeAndNetType.h>
#include <epg/utils/updateVerticesFictitious.h>

//APP
#include <app/params/ThemeParameters.h>


namespace app{
namespace calcul{
namespace matching{


	
	///
	///
	///
	ApplyDisplacementOnTransNetwork::ApplyDisplacementOnTransNetwork( 
		ign::feature::sql::FeatureStorePostgis& fsRoad,
		ign::feature::sql::FeatureStorePostgis& fsRoadNode,
		ign::feature::sql::FeatureStorePostgis& fsRailway,
		ign::feature::sql::FeatureStorePostgis& fsRailwayNode
		):
		_fsRoad(fsRoad),
		_fsRoadNode(fsRoadNode),
		_fsRailway(fsRailway),
		_fsRailwayNode(fsRailwayNode)
	{
		_uidGeneratorRoadNode = new epg::sql::tools::UidGenerator ("ROADMERGED", _fsRoadNode);
		_uidGeneratorRailwayNode = new epg::sql::tools::UidGenerator("RAILWAYMERGED",_fsRailwayNode);
	}

	///
	///
	///
	ApplyDisplacementOnTransNetwork::~ApplyDisplacementOnTransNetwork( )
	{
	}

		
	///
	///
	///
	void ApplyDisplacementOnTransNetwork::compute( 
		std::string countryCode,
		std::map< std::string, ign::math::Vec2d > const& mRefDisplacements,
		bool verbose
		)
	{

		epg::Context* context = epg::ContextS::getInstance();

		epg::log::ScopeLogger log( "app::calcul::matching::applyDisplacementOnTransNetwork" );

		epg::params::EpgParameters& epgParams = context->getEpgParameters();

		std::string const& sourceEdgeName = epgParams.getValue( EDGE_SOURCE ).toString();
		std::string const& targetEdgeName = epgParams.getValue( EDGE_TARGET ).toString();
		std::string const& netTypeName = epgParams.getValue( NET_TYPE ).toString();
		std::string const& countryCodeName = epgParams.getValue( COUNTRY_CODE ).toString();
		std::string const& vertexFictitiousName = epgParams.getValue( VERTEX_FICTITIOUS ).toString();

		std::string const edgeTableNameTemp = "_500_temp_edge_matching_trans";
		std::string const vertexTableNameTemp = "_500_temp_vertex_matching_trans";

		std::string countryCodeQuery = countryCodeName + " LIKE '%" + countryCode + "%'";
		ign::feature::FeatureFilter countryFilter( countryCodeQuery );

		//creation de tables temporaires d'edge et de vertex avec le ferre et le routier proche des frontieres


		epg::utils::TableCreator tableEdge( edgeTableNameTemp );

		epgParams.setParameter( EDGE_TABLE, ign::data::String( edgeTableNameTemp ) );

		int id = tableEdge.addField( new epg::utils::TableCreator::Field( epgParams.getValue( ID ).toString(), epg::sql::SqlType::String ) );
		tableEdge.setKeyField( id );

		id = tableEdge.addField( new epg::utils::TableCreator::Field( epgParams.getValue( GEOM ).toString(), epg::sql::SqlType::Geometry ) );
		tableEdge.setGeomField( id );
		tableEdge.fieldN( id )->setGeometryType( ign::geometry::Geometry::GeometryTypeLineString );

		id = tableEdge.addField( new epg::utils::TableCreator::Field( sourceEdgeName, epg::sql::SqlType::String ) );
		tableEdge.setIndexedField( id );
		id = tableEdge.addField( new epg::utils::TableCreator::Field( targetEdgeName, epg::sql::SqlType::String ) );
		tableEdge.setIndexedField( id );

		tableEdge.addField( new epg::utils::TableCreator::Field( netTypeName, epg::sql::SqlType::Integer, ign::data::Integer( 0 ) ) );

		tableEdge.addField( new epg::utils::TableCreator::Field( countryCodeName, epg::sql::SqlType::String, ign::data::String( countryCode ) ) );

		tableEdge.createTable();

		ign::feature::sql::FeatureStorePostgis* fsEdge = context->getFeatureStore( epg::EDGE );


		epg::utils::TableCreator tableVertex( vertexTableNameTemp );

		epgParams.setParameter( VERTEX_TABLE, ign::data::String( vertexTableNameTemp ) );

		id = tableVertex.addField( new epg::utils::TableCreator::Field( epgParams.getValue( ID ).toString(), epg::sql::SqlType::String ) );
		tableVertex.setKeyField( id );

		id = tableVertex.addField( new epg::utils::TableCreator::Field( epgParams.getValue( GEOM ).toString(), epg::sql::SqlType::Geometry ) );
		tableVertex.setGeomField( id );
		tableVertex.fieldN( id )->setGeometryType( ign::geometry::Geometry::GeometryTypePoint );

		tableVertex.addField( new epg::utils::TableCreator::Field( netTypeName, epg::sql::SqlType::Integer, ign::data::Integer( 0 ) ) );

		tableVertex.addField( new epg::utils::TableCreator::Field( countryCodeName, epg::sql::SqlType::String, ign::data::String( countryCode ) ) );

		tableVertex.addField( new epg::utils::TableCreator::Field( vertexFictitiousName, epg::sql::SqlType::Boolean, ign::data::Boolean( true ) ) );

		tableVertex.createTable();

		ign::feature::sql::FeatureStorePostgis* fsVertex = context->getFeatureStore( epg::VERTEX );


		ign::feature::sql::FeatureStorePostgis* _fsBoundary = context->getFeatureStore( epg::TARGET_BOUNDARY );



		ign::feature::FeatureIteratorPtr itBoundary = _fsBoundary->getFeatures( countryFilter );

		while ( itBoundary->hasNext() ) {

			ign::feature::Feature fBoundary = itBoundary->next();

			ign::feature::FeatureFilter filterBoundary;
			filterBoundary.setExtent( fBoundary.getGeometry().getEnvelope().expandBy( 10000 ) );
			epg::tools::FilterTools::addAndConditions( filterBoundary, countryCodeQuery );

			ign::feature::FeatureIteratorPtr itRoad = _fsRoad.getFeatures( filterBoundary );

			while ( itRoad->hasNext() ) {
				ign::feature::Feature fRoad = itRoad->next();

				if ( _sVisitedEdges.find( fRoad.getId() ) != _sVisitedEdges.end() ) 
					continue;

				_sVisitedEdges.insert( fRoad.getId() );

				ign::feature::Feature fEdge = fsEdge->newFeature();

				fEdge.setId( fRoad.getId() );
				fEdge.setGeometry( fRoad.getGeometry().asLineString() );
				std::string sourceEdge = fRoad.getAttribute( sourceEdgeName ).toString();
				fEdge.setAttribute( sourceEdgeName, ign::data::String( sourceEdge ) );
				std::string targetEdge = fRoad.getAttribute( targetEdgeName ).toString();
				fEdge.setAttribute( targetEdgeName, ign::data::String( targetEdge ) );
				fEdge.setAttribute( netTypeName, fRoad.getAttribute( netTypeName ) );
				fEdge.setAttribute( countryCodeName, fRoad.getAttribute( countryCodeName ) );

				fsEdge->createFeature( fEdge, fEdge.getId() );

				if ( _sVisitedVertices.find( sourceEdge ) == _sVisitedVertices.end() ) {
					ign::feature::Feature fRoadNode;
					_fsRoadNode.getFeatureById( sourceEdge, fRoadNode );
					IGN_ASSERT( !fRoadNode.getId().empty() );
					_sVisitedVertices.insert( sourceEdge );
					ign::feature::Feature fVertex = fsVertex->newFeature();
					fVertex.setId( sourceEdge );
					fVertex.setGeometry( fRoadNode.getGeometry().asPoint() );
					fVertex.setAttribute( countryCodeName, fRoadNode.getAttribute( countryCodeName ) );
					fVertex.setAttribute( vertexFictitiousName, fRoadNode.getAttribute( vertexFictitiousName ) );
					fsVertex->createFeature( fVertex, fVertex.getId() );
				}

				if ( _sVisitedVertices.find( targetEdge ) == _sVisitedVertices.end() ) {
					ign::feature::Feature fRoadNode;
					_fsRoadNode.getFeatureById( targetEdge, fRoadNode );
					IGN_ASSERT( !fRoadNode.getId().empty() );
					_sVisitedVertices.insert( targetEdge );
					ign::feature::Feature fVertex = fsVertex->newFeature();
					fVertex.setId( targetEdge );
					fVertex.setGeometry( fRoadNode.getGeometry().asPoint() );
					fVertex.setAttribute( countryCodeName, fRoadNode.getAttribute( countryCodeName ) );
					fVertex.setAttribute( vertexFictitiousName, fRoadNode.getAttribute( vertexFictitiousName ) );
					fsVertex->createFeature( fVertex, fVertex.getId() );
				}

			}

			ign::feature::FeatureIteratorPtr itRailway = _fsRailway.getFeatures( filterBoundary );

			while ( itRailway->hasNext() ) {
				ign::feature::Feature fRailway = itRailway->next();

				if ( _sVisitedEdges.find( fRailway.getId() ) != _sVisitedEdges.end() ) continue;
				_sVisitedEdges.insert( fRailway.getId() );

				ign::feature::Feature fEdge = fsEdge->newFeature();

				fEdge.setId( fRailway.getId() );
				fEdge.setGeometry( fRailway.getGeometry().asLineString() );
				std::string sourceEdge = fRailway.getAttribute( sourceEdgeName ).toString();
				fEdge.setAttribute( sourceEdgeName, ign::data::String( sourceEdge ) );
				std::string targetEdge = fRailway.getAttribute( targetEdgeName ).toString();
				fEdge.setAttribute( targetEdgeName, ign::data::String( targetEdge ) );
				fEdge.setAttribute( netTypeName, fRailway.getAttribute( netTypeName ) );
				fEdge.setAttribute( countryCodeName, fRailway.getAttribute( countryCodeName ) );

				fsEdge->createFeature( fEdge, fEdge.getId() );

				if ( _sVisitedVertices.find( sourceEdge ) == _sVisitedVertices.end() ) {
					ign::feature::Feature fRailwayNode;
					_fsRailwayNode.getFeatureById( sourceEdge, fRailwayNode );
					IGN_ASSERT( !fRailwayNode.getId().empty() );
					_sVisitedVertices.insert( sourceEdge );
					ign::feature::Feature fVertex = fsVertex->newFeature();
					fVertex.setId( sourceEdge );
					fVertex.setGeometry( fRailwayNode.getGeometry().asPoint() );
					fVertex.setAttribute( countryCodeName, fRailwayNode.getAttribute( countryCodeName ) );
					fVertex.setAttribute( vertexFictitiousName, fRailwayNode.getAttribute( vertexFictitiousName ) );
					fsVertex->createFeature( fVertex, fVertex.getId() );
				}

				if ( _sVisitedVertices.find( targetEdge ) == _sVisitedVertices.end() ) {
					ign::feature::Feature fRailwayNode;
					_fsRailwayNode.getFeatureById( targetEdge, fRailwayNode );
					IGN_ASSERT( !fRailwayNode.getId().empty() );
					_sVisitedVertices.insert( targetEdge );
					ign::feature::Feature fVertex = fsVertex->newFeature();
					fVertex.setId( targetEdge );
					fVertex.setGeometry( fRailwayNode.getGeometry().asPoint() );
					fVertex.setAttribute( countryCodeName, fRailwayNode.getAttribute( countryCodeName ) );
					fVertex.setAttribute( vertexFictitiousName, fRailwayNode.getAttribute( vertexFictitiousName ) );
					fsVertex->createFeature( fVertex, fVertex.getId() );
				}

			}

		}


		//deplacement
		epg::calcul::matching::MatchingWithConnectingFeatures EdgeMatchingStep;

		EdgeMatchingStep.applyDisplacements( countryCode, mRefDisplacements, verbose );


		//impact des deplacements dans les tables du routier et du ferre
		/// \todo impacter pas que la geom mais aussi les autres attributs et supprimer aussi les nds qui existent plus et impacter leurs attributs
		ign::feature::FeatureIteratorPtr itEdgeRoad = fsEdge->getFeatures( countryFilter );
		while ( itEdgeRoad->hasNext() ) {

			ign::feature::Feature fEdge = itEdgeRoad->next();

			//on supprime les edges modifies dans le set des edges visites
			IGN_ASSERT( _sVisitedEdges.find( fEdge.getId() ) != _sVisitedEdges.end() );
			_sVisitedEdges.erase( fEdge.getId() );

			ign::feature::Feature fToUpdate;
			_fsRoad.getFeatureById( fEdge.getId(), fToUpdate );
			if ( !fToUpdate.getId().empty() ) {
				fToUpdate.setGeometry( fEdge.getGeometry() );
				fToUpdate.setAttribute( sourceEdgeName, fEdge.getAttribute( sourceEdgeName ) );
				fToUpdate.setAttribute( targetEdgeName, fEdge.getAttribute( targetEdgeName ) );

				//noeuds
				ign::feature::Feature fSource, fTarget;
				std::string idSourceVertex = fToUpdate.getAttribute( sourceEdgeName ).toString();

				_fsRoadNode.getFeatureById( idSourceVertex, fSource );

				if ( fSource.getId().empty() ) {
					if ( _mModifiedMergedVertex.find( idSourceVertex ) == _mModifiedMergedVertex.end() )
					{
						fSource = _fsRoadNode.newFeature();
						std::string idVertexNew = _uidGeneratorRoadNode->next();
						_mModifiedMergedVertex[idSourceVertex] = idVertexNew;
						fSource.setId( idVertexNew );
						fSource.setGeometry( fToUpdate.getGeometry().asLineString().startPoint() );
						_sVisitedVertices.insert( idVertexNew );
						fToUpdate.setAttribute( sourceEdgeName, ign::data::String( idVertexNew ) );
						_fsRoadNode.createFeature( fSource, idVertexNew );
					}
					else
					{
						_fsRoadNode.getFeatureById( _mModifiedMergedVertex.find( idSourceVertex )->second, fSource );
						IGN_ASSERT( !fSource.getId().empty() );
						fToUpdate.setAttribute( sourceEdgeName, ign::data::String( _mModifiedMergedVertex.find( idSourceVertex )->second ) );
					}
				}

				IGN_ASSERT( !fSource.getId().empty() );


				std::string idTargetVertex = fToUpdate.getAttribute( targetEdgeName ).toString();
				_fsRoadNode.getFeatureById( idTargetVertex, fTarget );

				if ( fTarget.getId().empty() ) {
					if ( _mModifiedMergedVertex.find( idTargetVertex ) == _mModifiedMergedVertex.end() )
					{
						fTarget = _fsRoadNode.newFeature();
						std::string idVertexNew = _uidGeneratorRoadNode->next();
						_mModifiedMergedVertex[idTargetVertex] = idVertexNew;
						fTarget.setId( idVertexNew );
						fTarget.setGeometry( fToUpdate.getGeometry().asLineString().endPoint() );
						_sVisitedVertices.insert( idVertexNew );
						fToUpdate.setAttribute( targetEdgeName, ign::data::String( idVertexNew ) );
						_fsRoadNode.createFeature( fTarget, idVertexNew );
					}
					else
					{
						_fsRoadNode.getFeatureById( _mModifiedMergedVertex.find( idTargetVertex )->second, fTarget );
						IGN_ASSERT( !fTarget.getId().empty() );
						fToUpdate.setAttribute( targetEdgeName, ign::data::String( _mModifiedMergedVertex.find( idTargetVertex )->second ) );
					}
				}

				IGN_ASSERT( !fTarget.getId().empty() );

				//modify feature
				_fsRoad.modifyFeature( fToUpdate );

				epgParams.setParameter( EDGE_TABLE, ign::data::String( _fsRoad.getTableName() ) );
				_updateVertex( fSource );
				_fsRoadNode.modifyFeature( fSource );

				_updateVertex( fTarget );
				_fsRoadNode.modifyFeature( fTarget );
				epgParams.setParameter( EDGE_TABLE, ign::data::String( edgeTableNameTemp ) );
			}

			else 
			{
				_fsRailway.getFeatureById( fEdge.getId(), fToUpdate );
				IGN_ASSERT( !fToUpdate.getId().empty() );
				fToUpdate.setAttribute( sourceEdgeName, fEdge.getAttribute( sourceEdgeName ) );
				fToUpdate.setAttribute( targetEdgeName, fEdge.getAttribute( targetEdgeName ) );
				fToUpdate.setGeometry( fEdge.getGeometry() );


				//noeuds
				ign::feature::Feature fSource, fTarget;
				std::string idSourceVertex = fToUpdate.getAttribute( sourceEdgeName ).toString();

				_fsRailwayNode.getFeatureById( idSourceVertex, fSource );

				if ( fSource.getId().empty() ) {
					if ( _mModifiedMergedVertex.find( idSourceVertex ) == _mModifiedMergedVertex.end() )
					{
						fSource = _fsRailwayNode.newFeature();
						std::string idVertexNew = _uidGeneratorRailwayNode->next();
						_mModifiedMergedVertex[idSourceVertex] = idVertexNew;
						fSource.setId( idVertexNew );
						fSource.setGeometry( fToUpdate.getGeometry().asLineString().startPoint() );
						_sVisitedVertices.insert( idVertexNew );
						fToUpdate.setAttribute( sourceEdgeName, ign::data::String( idVertexNew ) );
						_fsRailwayNode.createFeature( fSource, idVertexNew );
					}
					else
					{
						_fsRailwayNode.getFeatureById( _mModifiedMergedVertex.find( idSourceVertex )->second, fSource );
						IGN_ASSERT( !fSource.getId().empty() );
						fToUpdate.setAttribute( sourceEdgeName, ign::data::String( _mModifiedMergedVertex.find( idSourceVertex )->second ) );
					}
				}

				IGN_ASSERT( !fSource.getId().empty() );


				std::string idTargetVertex = fToUpdate.getAttribute( targetEdgeName ).toString();
				_fsRailwayNode.getFeatureById( idTargetVertex, fTarget );

				if ( fTarget.getId().empty() ) {
					if ( _mModifiedMergedVertex.find( idTargetVertex ) == _mModifiedMergedVertex.end() )
					{
						fTarget = _fsRailwayNode.newFeature();
						std::string idVertexNew = _uidGeneratorRailwayNode->next();
						_mModifiedMergedVertex[idTargetVertex] = idVertexNew;
						fTarget.setId( idVertexNew );
						fTarget.setGeometry( fToUpdate.getGeometry().asLineString().endPoint() );
						_sVisitedVertices.insert( idVertexNew );
						fToUpdate.setAttribute( targetEdgeName, ign::data::String( idVertexNew ) );
						_fsRailwayNode.createFeature( fTarget, idVertexNew );
					}
					else
					{
						_fsRailwayNode.getFeatureById( _mModifiedMergedVertex.find( idTargetVertex )->second, fTarget );
						IGN_ASSERT( !fTarget.getId().empty() );
						fToUpdate.setAttribute( targetEdgeName, ign::data::String( _mModifiedMergedVertex.find( idTargetVertex )->second ) );
					}

				}

				IGN_ASSERT( !fTarget.getId().empty() );

				//modify feature
				_fsRailway.modifyFeature( fToUpdate );

				epgParams.setParameter( EDGE_TABLE, ign::data::String( _fsRailway.getTableName() ) );
				_updateVertex( fSource );
				_fsRailwayNode.modifyFeature( fSource );

				_updateVertex( fTarget );
				_fsRailwayNode.modifyFeature( fTarget );
				epgParams.setParameter( EDGE_TABLE, ign::data::String( edgeTableNameTemp ) );
			}

		}

		//impact des edges supprimes
		for ( std::set<std::string>::iterator sit = _sVisitedEdges.begin(); sit != _sVisitedEdges.end(); ++sit ) 
		{
			ign::feature::Feature fToUpdate;
			_fsRoad.getFeatureById( *sit, fToUpdate );

			if ( !fToUpdate.getId().empty() )
				_fsRoad.deleteFeature( *sit );
			else 
			{
				_fsRailway.getFeatureById( *sit, fToUpdate );
				IGN_ASSERT( !fToUpdate.getId().empty() );
				_fsRailway.deleteFeature( *sit );
			}
		}

		//impact des vertices supprimes
		for ( std::set<std::string>::iterator sit = _sVisitedVertices.begin(); sit != _sVisitedVertices.end(); ++sit ) 
		{
			ign::feature::Feature fToUpdate;
			_fsRoadNode.getFeatureById( *sit, fToUpdate );

			if ( !fToUpdate.getId().empty() )
				_fsRoadNode.deleteFeature( *sit );
			else 
			{
				_fsRailwayNode.getFeatureById( *sit, fToUpdate );
				IGN_ASSERT( !fToUpdate.getId().empty() );
				_fsRailwayNode.deleteFeature( *sit );
			}
		}

		//context->getDataBaseManager().getConnection()->update( "DROP TABLE "+edgeTableNameTemp );

		//context->getDataBaseManager().getConnection()->update( "DROP TABLE "+vertexTableNameTemp );

	}


	void ApplyDisplacementOnTransNetwork::_updateVertex( ign::feature::Feature& vertexToUpdate )
	{
		if( _sVisitedVertices.find( vertexToUpdate.getId() ) == _sVisitedVertices.end() ) return;

		_sVisitedVertices.erase( vertexToUpdate.getId() );

		epg::Context* context = epg::ContextS::getInstance();

		epg::params::EpgParameters& epgParams = context->getEpgParameters();
		
		std::string const& netTypeName = epgParams.getValue( NET_TYPE ).toString();
		std::string const& countryCodeName = epgParams.getValue( COUNTRY_CODE ).toString();
		std::string const& vertexFictitiousName = epgParams.getValue( VERTEX_FICTITIOUS ).toString();

		
		ign::feature::sql::FeatureStorePostgis* fsVertex = context->getFeatureStore( epg::VERTEX ) ;

		ign::feature::Feature vertexTemp;

		fsVertex->getFeatureById( vertexToUpdate.getId(), vertexTemp ) ;

		if ( ! vertexTemp.getId().empty() )
		{		
			vertexToUpdate.setAttribute( netTypeName, vertexTemp.getAttribute(netTypeName) ) ;
			vertexToUpdate.setAttribute( countryCodeName, vertexTemp.getAttribute(countryCodeName) ) ;
			vertexToUpdate.setAttribute(vertexFictitiousName, vertexTemp.getAttribute(vertexFictitiousName));
			vertexToUpdate.setGeometry(vertexTemp.getGeometry());
		}
		else {
			epg::utils::updateVertexCountryCodeAndNetType(vertexToUpdate);
			vertexToUpdate.setAttribute(vertexFictitiousName,ign::data::Boolean(false));
		}


		
	}


}
}
}