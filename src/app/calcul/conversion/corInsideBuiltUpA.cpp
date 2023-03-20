#include <app/calcul/conversion/corInsideBuiltUpA.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/replaceTableName.h>
#include <epg/utils/CopyTableUtils.h>
#include <epg/calcul/consistency/SplittingEdgesAndAreas.h>


//APP
#include <app/params/ThemeParameters.h>


namespace app{
namespace calcul{
namespace conversion{

	void corInsideBuiltUpA(){

		epg::log::ScopeLogger log("[ app::calcul::conversion::corInsideBuiltpA ]") ;

		epg::Context* context=epg::ContextS::getInstance();

		epg::params::EpgParameters & params=context->getEpgParameters();

		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();


		std::string const idName=params.getValue(ID).toString();

		std::string const geomName=params.getValue(GEOM).toString();


		std::string  const builtUpATableName=epg::utils::replaceTableName(transParameter->getValue(BUILTUPA_TABLE).toString());

		epg::utils::CopyTableUtils::copyTable(transParameter->getValue(BUILTUPA_TABLE).toString() , idName, geomName, ign::geometry::Geometry::GeometryTypePolygon,builtUpATableName,"",false);


		ign::feature::sql::FeatureStorePostgis* fsBuiltUpA= context->getDataBaseManager().getFeatureStore( builtUpATableName, idName ,geomName );

		ign::feature::sql::FeatureStorePostgis* fsEdge= context->getFeatureStore( epg::EDGE ) ;

		ign::feature::FeatureFilter filterEdge( "cor != 1" ) ;

		ign::feature::FeatureIteratorPtr itBuiltupa = fsBuiltUpA->getFeatures( ign::feature::FeatureFilter() ) ;

		while( itBuiltupa->hasNext() ) {
			ign::feature::Feature fBuiltupa = itBuiltupa->next() ;

			ign::geometry::Polygon poly = fBuiltupa.getGeometry().asPolygon();

			ign::feature::FeatureFilter filterBuiltupa = filterEdge;
			filterBuiltupa.setExtent( poly.getEnvelope() );

			ign::feature::FeatureIteratorPtr itEdge = fsEdge->getFeatures(filterBuiltupa);

			while( itEdge->hasNext() ) {
				ign::feature::Feature fEdge = itEdge->next() ;

				if( fEdge.getAttribute("cor").toInteger() == 2 ) continue;

				if( fEdge.getGeometry().distance( poly ) > 0 ) continue;

				if( ! poly.contains( fEdge.getGeometry() ) ) continue;

				fEdge.setAttribute( "cor", ign::data::Integer(2) );

				fsEdge->modifyFeature( fEdge ) ;

			}
		}


		/*
		std::string const locTempValue = "-1" ;
		epg::calcul::consistency::SplittingEdgesAndAreas splittingRoadLAndBuitUpA(locTempValue);

		std::set< std::string > sCuttingPts;

		splittingRoadLAndBuitUpA.onCompute(*fsBuiltUpA,sCuttingPts,ign::feature::FeatureFilter(),false);
		*/



	}

}
}
}