
#ifndef _APP_THEMEPARAMETERS_
#define _APP_THEMEPARAMETERS_

#include <string>
#include <epg/params/ParametersT.h>
#include <epg/SingletonT.h>

//SOCLE
#include <ign/Exception.h>



	enum TRANS_PARAMETERS{

		SOURCE_ROAD_TABLE,
		SOURCE_RAILWAY_TABLE,     
		TARGET_ROAD_TABLE,      
		TARGET_RAILWAY_TABLE,

		EDGE_ROAD_CODE,
		EDGE_ROAD_CODE_BDUNI,
		EDGE_RAILWAY_CODE,
		POINT_TO_ROAD_VERTEX_ID,
		POINT_TO_RAILWAY_VERTEX_ID,
		EDGE_POSITION_SOL,

		INIT_ROAD_FILTER,

		CONNECT_POINT_RAILWAY_TABLE,
		CONNECT_LINE_ROAD_TABLE,
		CONNECT_POINT_ROAD_TABLE,

		SOURCE_ROAD_NODE_TABLE,  
		SOURCE_RAILWAY_NODE_TABLE,

		ME_AIRES,
		ME_ECHANGEURS,
		ME_GARES,
		ME_AIRES_POINT,
		ME_ECHANGEURS_POINT,
		ME_GARES_POINT,

		TARGET_INTERCC_TABLE,
		TARGET_LEVELCC_TABLE,
		TARGET_RESTC_TABLE,
		TARGET_RAILRDC_TABLE,

		AIRFLDC_TABLE,
		AIRFLDP_TABLE,
		HARBORC_TABLE,
		HARBORP_TABLE,
		FERRYC_TABLE,
		BUILTUPA_TABLE,
		BUILTUPP_TABLE,

		BDUNI_ROAD_OLD_TABLE,
		ERM_ROAD_OLD_TABLE,
		ERM_RAILWAY_OLD_TABLE,
		ERM_RAILRDC_OLD_TABLE,

		TARGET_COUNTRY_TABLE_FR_MC,
		TARGET_COUNTRY_TABLE_MERGED,	

		TMP_CP_TABLE,
		TMP_CL_TABLE

	};

namespace app{
namespace params{

	class TransParameters : public epg::params::ParametersT< TRANS_PARAMETERS >
	{
		typedef  epg::params::ParametersT< TRANS_PARAMETERS > Base;

		public:

			/// \brief
			TransParameters()
			{
				
				_initParameter( SOURCE_ROAD_TABLE, "SOURCE_ROAD_TABLE" )  ;
				_initParameter( SOURCE_RAILWAY_TABLE, "SOURCE_RAILWAY_TABLE" )  ;
				_initParameter( TARGET_ROAD_TABLE, "TARGET_ROAD_TABLE" )  ;
				_initParameter( TARGET_RAILWAY_TABLE, "TARGET_RAILWAY_TABLE" )  ;

				_initParameter(EDGE_ROAD_CODE, "EDGE_ROAD_CODE");
				_initParameter( EDGE_ROAD_CODE_BDUNI, "EDGE_ROAD_CODE_BDUNI" ) ;
				_initParameter( EDGE_RAILWAY_CODE, "EDGE_RAILWAY_CODE" ) ;
				_initParameter( POINT_TO_ROAD_VERTEX_ID, "POINT_TO_ROAD_VERTEX_ID" ) ;
				_initParameter(POINT_TO_RAILWAY_VERTEX_ID, "POINT_TO_RAILWAY_VERTEX_ID");
				_initParameter(EDGE_POSITION_SOL, "EDGE_POSITION_SOL" ) ;
				
				_initParameter(INIT_ROAD_FILTER, "INIT_ROAD_FILTER");

				_initParameter( CONNECT_POINT_RAILWAY_TABLE, "CONNECT_POINT_RAILWAY_TABLE" ) ;
				_initParameter( CONNECT_LINE_ROAD_TABLE, "CONNECT_LINE_ROAD_TABLE" ) ;
				_initParameter( CONNECT_POINT_ROAD_TABLE, "CONNECT_POINT_ROAD_TABLE" ) ;

				_initParameter(SOURCE_ROAD_NODE_TABLE, "SOURCE_ROAD_NODE_TABLE");
				_initParameter(SOURCE_RAILWAY_NODE_TABLE, "SOURCE_RAILWAY_NODE_TABLE");

				_initParameter(ME_AIRES, "ME_AIRES");
				_initParameter(ME_ECHANGEURS, "ME_ECHANGEURS");
				_initParameter(ME_GARES, "ME_GARES");
				_initParameter(ME_AIRES_POINT, "ME_AIRES_POINT");
				_initParameter(ME_ECHANGEURS_POINT, "ME_ECHANGEURS_POINT");
				_initParameter(ME_GARES_POINT, "ME_GARES_POINT");

				_initParameter( TARGET_INTERCC_TABLE, "TARGET_INTERCC_TABLE" )  ;
				_initParameter( TARGET_LEVELCC_TABLE, "TARGET_LEVELCC_TABLE" )  ;
				_initParameter( TARGET_RESTC_TABLE, "TARGET_RESTC_TABLE" )  ;
				_initParameter( TARGET_RAILRDC_TABLE, "TARGET_RAILRDC_TABLE" )  ;

				_initParameter( AIRFLDC_TABLE, "AIRFLDC_TABLE" )  ;
				_initParameter( AIRFLDP_TABLE, "AIRFLDP_TABLE" )  ;
				_initParameter( HARBORC_TABLE, "HARBORC_TABLE" )  ;
				_initParameter( HARBORP_TABLE, "HARBORP_TABLE" )  ;
				_initParameter( FERRYC_TABLE,  "FERRYC_TABLE"  )  ;
				_initParameter( BUILTUPA_TABLE, "BUILTUPA_TABLE" )  ;
				_initParameter(BUILTUPP_TABLE, "BUILTUPP_TABLE");

				_initParameter( BDUNI_ROAD_OLD_TABLE, "BDUNI_ROAD_OLD_TABLE" )  ;	
				_initParameter( ERM_ROAD_OLD_TABLE, "ERM_ROAD_OLD_TABLE" )  ;
				_initParameter( ERM_RAILWAY_OLD_TABLE, "ERM_RAILWAY_OLD_TABLE" )  ;
				_initParameter( ERM_RAILRDC_OLD_TABLE, "ERM_RAILRDC_OLD_TABLE" )  ;
				
				_initParameter( TARGET_COUNTRY_TABLE_FR_MC, "TARGET_COUNTRY_TABLE_FR_MC" )  ;
				_initParameter( TARGET_COUNTRY_TABLE_MERGED, "TARGET_COUNTRY_TABLE_MERGED" )  ;
				_initParameter(TMP_CP_TABLE, "TMP_CP_TABLE");
				_initParameter(TMP_CL_TABLE, "TMP_CL_TABLE");
			}

			/// \brief
			~TransParameters()
			{
			}

			/// \brief
			virtual std::string getClassName()const
			{
				return "TransParameters";
			}

			/// \brief
			/*virtual void loadParameter( std::string const& parameterName, std::string const& parameterValue )
			{
				right_it it = _mParamName.right.find( parameterName );
				if( it == _mParamName.right.end() )
					IGN_THROW_EXCEPTION( "[epg::"+getClassName()+"] parameter "+parameterName+" does not exist" );

				setParameter( it->second, parameterValue );
			}*/
	};

	typedef epg::Singleton< TransParameters >  TransParametersS;

}
}

#endif