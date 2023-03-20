#include <app/calcul/conversion/DataModelConversion.h>


// IGN
#include <ign/feature/FeatureFilter.h>

// EPG
//#include <epg/log/ScopeLogger.h>
#include <epg/Context.h>
#include <epg/log/EpgLogger.h>
//#include <epg/Parameters.h>
#include <epg/tools/FilterTools.h>

//
#include <epg/calcul/conversion/DataModelConvertor.h>
//#include <epg/calcul/conversion/convertor/concate.h>
#include <epg/calcul/conversion/convertor/formatNamn1.h>
#include <epg/calcul/conversion/convertor/namn1toAscii.h>
#include <epg/calcul/conversion/convertor/single.h>
#include <epg/calcul/conversion/convertor/nln1.h>


//// APP
#include <app/params/ThemeParameters.h>
#include <app/calcul/conversion/convertor/afa.h>
#include <app/calcul/conversion/convertor/cor.h>
#include <app/calcul/conversion/convertor/exs_rw.h>
#include <app/calcul/conversion/convertor/exs_rd.h>
#include <app/calcul/conversion/convertor/fco.h>
#include <app/calcul/conversion/convertor/gaw.h>
#include <app/calcul/conversion/convertor/lle.h>
#include <app/calcul/conversion/convertor/ltn.h>
#include <app/calcul/conversion/convertor/med.h>
#include <app/calcul/conversion/convertor/rgc.h>
#include <app/calcul/conversion/convertor/rjc.h>
#include <app/calcul/conversion/convertor/rra.h>
#include <app/calcul/conversion/convertor/rrc.h>
#include <app/calcul/conversion/convertor/rsd.h>
#include <app/calcul/conversion/convertor/rst.h>
#include <app/calcul/conversion/convertor/rstationid.h>
#include <app/calcul/conversion/convertor/rsu_rd.h>
#include <app/calcul/conversion/convertor/rte.h>
#include <app/calcul/conversion/convertor/rtn.h>
#include <app/calcul/conversion/convertor/rtt.h>
#include <app/calcul/conversion/convertor/tol.h>
#include <app/calcul/conversion/convertor/tuc_rwc.h>


//using namespace ign::data;


namespace app{
namespace calcul{
namespace conversion{

//
//
	void railrdCModelConversion(
		std::string const& sourceDataStoreName,
		std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter,
		int countRStationID
	)
	{

		epg::Context* context = epg::ContextS::getInstance();

		epg::log::EpgLogger* logger = epg::log::EpgLoggerS::getInstance();

		epg::params::EpgParameters & params = context->getEpgParameters();

		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		//epg::ScopeLogger log("[ app::calcul::conversion::dataModelConversion ]") ;
		boost::timer timer;

		epg::calcul::conversion::DataModelConvertor convertor( targetDataStoreName );

		std::string const idName = params.getValue( ID ).toString();

		std::string const geomName = params.getValue( GEOM ).toString();

		int id;


		//GEOM
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( geomName, epg::sql::SqlType::Geometry, geomName, epg::sql::SqlType::Geometry ) );
		convertor.convertorN( id )->getTargetAttribute().setGeometryType( ign::geometry::Geometry::GeometryTypePoint );
		convertor.setGeomConvertor( id );

		//ID
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( idName, epg::sql::SqlType::String, idName, epg::sql::SqlType::String ) );
		convertor.setKeyConvertor( id );


		//-- FCSUBTYPE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "fcsubtype", epg::sql::SqlType::Integer, ign::data::Integer( 1 ) ) );

		//-- F_CODE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "f_code", epg::sql::SqlType::String10, ign::data::String( "AQ125" ) ) );


		//-- ICC
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( "icc", epg::sql::SqlType::String, "icc", epg::sql::SqlType::String ) );


		//-- NAMN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::formatNamn1( "namn1", epg::sql::SqlType::String100 ) );

		//-- NAMN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "namn2", epg::sql::SqlType::String100, ign::data::String( "N_A" ) ) );

		//-- NAMA1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::namn1toAscii( "nama1", epg::sql::SqlType::String100 ) );

		//-- NAMA2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nama2", epg::sql::SqlType::String100, ign::data::String( "N_A" ) ) );

		//-- NLN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::nln1( "nln1", epg::sql::SqlType::String10 ) );

		//-- NLN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nln2", epg::sql::SqlType::String10, ign::data::String( "N_A" ) ) );


		//-- TFC
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "tfc", epg::sql::SqlType::Integer, ign::data::Integer( 15 ) ) );

		//-- TUC
		id = convertor.addConvertor( new convertor::tuc_rwc( "nature", epg::sql::SqlType::String, "tuc", epg::sql::SqlType::Integer ) );

		//-- RSTATIONID
		id = convertor.addConvertor( new convertor::rstationid( "rstationid", epg::sql::SqlType::String, countRStationID ) );


		//ATTRIBUTS DE TRAVAIL A CONSERVER
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( params.getValue( NET_TYPE ).toString(), epg::sql::SqlType::Integer, params.getValue( NET_TYPE ).toString(), epg::sql::SqlType::Integer ) );

		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( params.getValue( LINKED_FEATURE_ID ).toString(), epg::sql::SqlType::String, params.getValue( LINKED_FEATURE_ID ).toString(), epg::sql::SqlType::String ) );


		//chargement du fs source
		ign::feature::sql::FeatureStorePostgis fsSource( *epg::ContextS::getInstance()->getDataBaseManager().getConnection() );
		fsSource.load( sourceDataStoreName );


		//std::string conditionGare = "(nature='Gare de voyageurs' or nature='Gare de fret' or nature='Gare de voyageurs et de fret') and toponyme_specification != '' and "+params.getValue(NET_TYPE).toString()+" > 9 ";
		ign::feature::FeatureFilter filterGare = filter;
		
		std::string conditionGare = params.getValue( NET_TYPE ).toString() + " > 9 ";
		epg::tools::FilterTools::addAndConditions( filterGare, conditionGare );

		convertor.convert( fsSource, filterGare );

	}


	void railrdLModelConversion(
		std::string const& sourceDataStoreName,
		std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
	)
	{

		epg::Context* context = epg::ContextS::getInstance();

		epg::log::EpgLogger* logger = epg::log::EpgLoggerS::getInstance();

		epg::params::EpgParameters & params = context->getEpgParameters();

		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		boost::timer timer;

		epg::calcul::conversion::DataModelConvertor convertor( targetDataStoreName );

		std::string const idName = params.getValue( ID ).toString();

		std::string const geomName = params.getValue( GEOM ).toString();

		int id;

		//GEOM
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( geomName, epg::sql::SqlType::Geometry, geomName, epg::sql::SqlType::Geometry ) );
		convertor.convertorN( id )->getTargetAttribute().setGeometryType( ign::geometry::Geometry::GeometryTypeLineString );
		convertor.setGeomConvertor( id );

		//ID
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( idName, epg::sql::SqlType::String, idName, epg::sql::SqlType::String ) );
		convertor.setKeyConvertor( id );


		//-- FCSUBTYPE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "fcsubtype", epg::sql::SqlType::Integer, ign::data::Integer( 1 ) ) );

		//-- F_CODE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "f_code", epg::sql::SqlType::String10, ign::data::String( "AN010" ) ) );

		//-- ICC
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( "icc", epg::sql::SqlType::String, "icc", epg::sql::SqlType::String ) );


		//-- NAMN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::formatNamn1( "namn1", epg::sql::SqlType::String100 ) );

		//-- NAMN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "namn2", epg::sql::SqlType::String100, ign::data::String( "N_A" ) ) );

		//-- NAMA1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::namn1toAscii( "nama1", epg::sql::SqlType::String100 ) );

		//-- NAMA2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nama2", epg::sql::SqlType::String100, ign::data::String( "N_A" ) ) );

		//-- NLN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::nln1( "nln1", epg::sql::SqlType::String10 ) );

		//-- NLN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nln2", epg::sql::SqlType::String10, ign::data::String( "N_A" ) ) );


		//-- EXS
		id = convertor.addConvertor( new convertor::exs_rw( "exs", epg::sql::SqlType::Integer ) );

		//-- FCO
		id = convertor.addConvertor( new convertor::fco( "fco", epg::sql::SqlType::Integer ) );


		//-- GAW
		id = convertor.addConvertor( new convertor::gaw( "gaw", epg::sql::SqlType::Integer ) );

		//-- RCO
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "rco", epg::sql::SqlType::String10, ign::data::String( "UNK" ) ) );


		//-- RGC
		id = convertor.addConvertor( new convertor::rgc( "rgc", epg::sql::SqlType::Integer ) );

		//-- RRA
		id = convertor.addConvertor( new convertor::rra( "rra", epg::sql::SqlType::Integer ) );

		//-- RRC
		id = convertor.addConvertor( new convertor::rrc( "rrc", epg::sql::SqlType::Integer ) );

		//-- RSD
		id = convertor.addConvertor( new convertor::rsd( "rsd", epg::sql::SqlType::Integer ) );

		//-- RSU
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "rsu", epg::sql::SqlType::Integer, ign::data::Integer( 1 ) ) );

		//-- TEN
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "ten", epg::sql::SqlType::Integer, ign::data::Integer( 2 ) ) );

		//-- TUC
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "tuc", epg::sql::SqlType::Integer, ign::data::Integer( 45 ) ) );

		//-- LLE
		id = convertor.addConvertor( new convertor::lle( "lle", epg::sql::SqlType::Integer ) );



		//ATTRIBUT DE TRAVAIL A CONSERVER
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( params.getValue( NET_TYPE ).toString(), epg::sql::SqlType::Integer, params.getValue( NET_TYPE ).toString(), epg::sql::SqlType::Integer ) );

		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( params.getValue( EDGE_SOURCE ).toString(), epg::sql::SqlType::String, params.getValue( EDGE_SOURCE ).toString(), epg::sql::SqlType::String ) );

		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( params.getValue( EDGE_TARGET ).toString(), epg::sql::SqlType::String, params.getValue( EDGE_TARGET ).toString(), epg::sql::SqlType::String ) );


		//chargement du fs source
		ign::feature::sql::FeatureStorePostgis fsSource( *epg::ContextS::getInstance()->getDataBaseManager().getConnection() );
		fsSource.load( sourceDataStoreName );

		ign::feature::FeatureFilter filterSelection = filter;
		epg::tools::FilterTools::addAndConditions( filterSelection,	params.getValue( NET_TYPE ).toString() + " > 9 or " + params.getValue( NET_TYPE ).toString() + "=5" );

		convertor.convert( fsSource, filterSelection );

	}


	void restCModelConversion(
		std::string const& sourceDataStoreName,
		std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
	)
	{
		epg::Context* context = epg::ContextS::getInstance();
		epg::log::EpgLogger* logger = epg::log::EpgLoggerS::getInstance();

		epg::params::EpgParameters & params = context->getEpgParameters();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		epg::calcul::conversion::DataModelConvertor convertor( targetDataStoreName );

		std::string const idName = params.getValue( ID ).toString();
		std::string const geomName = params.getValue( GEOM ).toString();
		int id;

		//GEOM
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( geomName, epg::sql::SqlType::Geometry, geomName, epg::sql::SqlType::Geometry ) );
		convertor.convertorN( id )->getTargetAttribute().setGeometryType( ign::geometry::Geometry::GeometryTypePoint );
		convertor.setGeomConvertor( id );

		//ID
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( idName, epg::sql::SqlType::String, idName, epg::sql::SqlType::String ) );
		convertor.setKeyConvertor( id );

		//-- FCSUBTYPE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "fcsubtype", epg::sql::SqlType::Integer, ign::data::Integer( 1 ) ) );

		//-- F_CODE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "f_code", epg::sql::SqlType::String10, ign::data::String( "AQ135" ) ) );

		//-- ICC
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( "icc", epg::sql::SqlType::String, "icc", epg::sql::SqlType::String ) );

		//-- NAMN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::formatNamn1( "namn1", epg::sql::SqlType::String255 ) );

		//-- NAMN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "namn2", epg::sql::SqlType::String255, ign::data::String( "UNK" ) ) );

		//-- NAMA1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::namn1toAscii( "nama1", epg::sql::SqlType::String255 ) );

		//-- NAMA2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nama2", epg::sql::SqlType::String255, ign::data::String( "UNK" ) ) );

		//-- NLN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::nln1( "nln1", epg::sql::SqlType::String10 ) );

		//-- NLN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nln2", epg::sql::SqlType::String10, ign::data::String( "UNK" ) ) );

		//-- AFA
		id = convertor.addConvertor( new convertor::afa( "afa", epg::sql::SqlType::Integer ) );

		//chargement du fs source
		ign::feature::sql::FeatureStorePostgis fsSource( *epg::ContextS::getInstance()->getDataBaseManager().getConnection() );

		fsSource.load( sourceDataStoreName );

		ign::feature::FeatureFilter filterSelection = filter;
		epg::tools::FilterTools::addAndConditions( filterSelection, params.getValue( NET_TYPE ).toString() + " > 9 " );

		convertor.convert( fsSource, filterSelection );
	}


	void interCCModelConversion(
		std::string const& sourceDataStoreName,
		std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
	)
	{

		epg::Context* context = epg::ContextS::getInstance();
		epg::log::EpgLogger* logger = epg::log::EpgLoggerS::getInstance();

		epg::params::EpgParameters & params = context->getEpgParameters();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		epg::calcul::conversion::DataModelConvertor convertor( targetDataStoreName );

		std::string const idName = params.getValue( ID ).toString();
		std::string const geomName = params.getValue( GEOM ).toString();
		int id;

		//GEOM
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( geomName, epg::sql::SqlType::Geometry, geomName, epg::sql::SqlType::Geometry ) );
		convertor.convertorN( id )->getTargetAttribute().setGeometryType( ign::geometry::Geometry::GeometryTypePoint );
		convertor.setGeomConvertor( id );

		//ID
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( idName, epg::sql::SqlType::String, idName, epg::sql::SqlType::String ) );
		convertor.setKeyConvertor( id );

		//-- FCSUBTYPE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "fcsubtype", epg::sql::SqlType::Integer, ign::data::Integer( 1 ) ) );

		//-- F_CODE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "f_code", epg::sql::SqlType::String10, ign::data::String( "AP020" ) ) );

		//-- ICC
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( "icc", epg::sql::SqlType::String, "icc", epg::sql::SqlType::String ) );

		//-- NAMN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::formatNamn1( "namn1", epg::sql::SqlType::String100 ) );

		//-- NAMN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "namn2", epg::sql::SqlType::String100, ign::data::String( "UNK" ) ) );

		//-- NAMA1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::namn1toAscii( "nama1", epg::sql::SqlType::String100 ) );

		//-- NAMA2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nama2", epg::sql::SqlType::String100, ign::data::String( "UNK" ) ) );

		//-- NLN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::nln1( "nln1", epg::sql::SqlType::String10 ) );

		//-- NLN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nln2", epg::sql::SqlType::String10, ign::data::String( "UNK" ) ) );

		//-- RJC
		id = convertor.addConvertor( new convertor::rjc( "rjc", epg::sql::SqlType::Integer ) );

		//chargement du fs source
		ign::feature::sql::FeatureStorePostgis fsSource( *epg::ContextS::getInstance()->getDataBaseManager().getConnection() );

		fsSource.load( sourceDataStoreName );

		ign::feature::FeatureFilter filterSelection = filter;
		epg::tools::FilterTools::addAndConditions( filterSelection, params.getValue( NET_TYPE ).toString() + " > 9 " );

		convertor.convert( fsSource, filterSelection );

	}


	void levelCCModelConversion(
		std::string const& sourceDataStoreName,
		std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
	)
	{

		epg::Context* context = epg::ContextS::getInstance();
		epg::log::EpgLogger* logger = epg::log::EpgLoggerS::getInstance();

		epg::params::EpgParameters & params = context->getEpgParameters();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		epg::calcul::conversion::DataModelConvertor convertor( targetDataStoreName );

		std::string const idName = params.getValue( ID ).toString();
		std::string const geomName = params.getValue( GEOM ).toString();
		int id;

		//GEOM
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( geomName, epg::sql::SqlType::Geometry, geomName, epg::sql::SqlType::Geometry ) );
		convertor.convertorN( id )->getTargetAttribute().setGeometryType( ign::geometry::Geometry::GeometryTypePoint );
		convertor.setGeomConvertor( id );

		//ID
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( idName, epg::sql::SqlType::String, idName, epg::sql::SqlType::String ) );
		convertor.setKeyConvertor( id );

		//-- FCSUBTYPE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "fcsubtype", epg::sql::SqlType::Integer, ign::data::Integer( 1 ) ) );

		//-- F_CODE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "f_code", epg::sql::SqlType::String10, ign::data::String( "AQ062" ) ) );

		//-- ICC
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( "icc", epg::sql::SqlType::String, "icc", epg::sql::SqlType::String ) );

		//chargement du fs source
		ign::feature::sql::FeatureStorePostgis fsSource( *epg::ContextS::getInstance()->getDataBaseManager().getConnection() );

		fsSource.load( sourceDataStoreName );

		convertor.convert( fsSource, filter );

	}



	void roadLModelConversion(
		std::string const& sourceDataStoreName,
		std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
	)
	{

		epg::Context* context = epg::ContextS::getInstance();
		epg::log::EpgLogger* logger = epg::log::EpgLoggerS::getInstance();

		epg::params::EpgParameters & params = context->getEpgParameters();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		epg::calcul::conversion::DataModelConvertor convertor( targetDataStoreName );

		std::string const idName = params.getValue( ID ).toString();
		std::string const geomName = params.getValue( GEOM ).toString();

		int id;


		//GEOM
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( geomName, epg::sql::SqlType::Geometry, geomName, epg::sql::SqlType::Geometry ) );
		convertor.convertorN( id )->getTargetAttribute().setGeometryType( ign::geometry::Geometry::GeometryTypeLineString );
		convertor.setGeomConvertor( id );

		//ID
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( idName, epg::sql::SqlType::String, idName, epg::sql::SqlType::String ) );
		convertor.setKeyConvertor( id );


		//-- FCSUBTYPE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "fcsubtype", epg::sql::SqlType::Integer, ign::data::Integer( 1 ) ) );

		//-- F_CODE
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "f_code", epg::sql::SqlType::String10, ign::data::String( "AP030" ) ) );

		//-- ICC
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( "icc", epg::sql::SqlType::String, "icc", epg::sql::SqlType::String ) );

		//-- NAMN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::formatNamn1( "namn1", epg::sql::SqlType::String100 ) );

		//-- NAMN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "namn2", epg::sql::SqlType::String100, ign::data::String( "UNK" ) ) );

		//-- NAMA1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::namn1toAscii( "nama1", epg::sql::SqlType::String100 ) );

		//-- NAMA2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nama2", epg::sql::SqlType::String100, ign::data::String( "UNK" ) ) );

		//-- NLN1
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::nln1( "nln1", epg::sql::SqlType::String10 ) );

		//-- NLN2
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "nln2", epg::sql::SqlType::String10, ign::data::String( "UNK" ) ) );

		//-- COR
		id = convertor.addConvertor( new convertor::cor( "cor", epg::sql::SqlType::Integer ) );
		//ATTENTION la valeur COR=2  n'est pas remplie, cela est fait par la suite

		//-- EXS
		id = convertor.addConvertor( new convertor::exs_rd( "exs", epg::sql::SqlType::Integer ) );

		//-- LTN
		id = convertor.addConvertor( new convertor::ltn( "ltn", epg::sql::SqlType::Integer ) );

		//-- MED
		id = convertor.addConvertor( new convertor::med( "med", epg::sql::SqlType::Integer ) );

		//-- RST
		id = convertor.addConvertor( new convertor::rst( "rst", epg::sql::SqlType::Integer ) );

		//-- RSU
		id = convertor.addConvertor( new convertor::rsu_rd( "rsu", epg::sql::SqlType::Integer ) );

		//-- RTE
		id = convertor.addConvertor( new convertor::rte( "rte", epg::sql::SqlType::String ) );

		//-- RTN
		id = convertor.addConvertor( new convertor::rtn( "rtn", epg::sql::SqlType::String ) );

		//-- RTT
		id = convertor.addConvertor( new convertor::rtt( "rtt", epg::sql::SqlType::Integer ) );

		//-- TEN
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "ten", epg::sql::SqlType::Integer, ign::data::Integer( 2 ) ) );

		//-- TOC
		id = convertor.addConvertor( new convertor::tol( "tol", epg::sql::SqlType::Integer ) );

		//-- TUC
		id = convertor.addConvertor( new epg::calcul::conversion::convertor::single( "tuc", epg::sql::SqlType::Integer, ign::data::Integer( 7 ) ) );

		//-- LLE
		id = convertor.addConvertor( new convertor::lle( "lle", epg::sql::SqlType::Integer ) );


		//ATTRIBUT DE TRAVAIL A CONSERVER
		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( params.getValue( NET_TYPE ).toString(), epg::sql::SqlType::Integer, params.getValue( NET_TYPE ).toString(), epg::sql::SqlType::Integer ) );

		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( params.getValue( EDGE_SOURCE ).toString(), epg::sql::SqlType::String, params.getValue( EDGE_SOURCE ).toString(), epg::sql::SqlType::String ) );

		id = convertor.addConvertor( new epg::calcul::conversion::Convertor( params.getValue( EDGE_TARGET ).toString(), epg::sql::SqlType::String, params.getValue( EDGE_TARGET ).toString(), epg::sql::SqlType::String ) );


		//chargement du fs source
		ign::feature::sql::FeatureStorePostgis fsSource( *epg::ContextS::getInstance()->getDataBaseManager().getConnection() );
		fsSource.load( sourceDataStoreName );

		ign::feature::FeatureFilter filterSelection = filter;
		epg::tools::FilterTools::addAndConditions( filterSelection, params.getValue( NET_TYPE ).toString() + " > 9 or " + params.getValue( NET_TYPE ).toString() + "=5" );

		convertor.convert( fsSource, filterSelection );

	}


}
}
}