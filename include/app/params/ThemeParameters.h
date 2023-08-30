
#ifndef _APP_THEMEPARAMETERS_
#define _APP_THEMEPARAMETERS_

#include <string>
#include <epg/params/ParametersT.h>
#include <epg/SingletonT.h>

//SOCLE
#include <ign/Exception.h>



	enum TRANS_PARAMETERS{

		SOURCE_ROAD_TABLE,

		EDGE_NATIONAL_ROAD_CODE,
		EDGE_EUROPEAN_ROAD_CODE,
		NATIONAL_IDENTIFIER,

		CP_TABLE,
		CL_TABLE,
		CF_STATUS,

		LIST_ATTR_TO_CONCAT,
		LIST_ATTR_W,

		SQL_FILTER_EDGES_2_GENERATE_CF,

		LAND_COVER_TYPE,
		TYPE_LAND_AREA

	};

namespace app{
namespace params{

	class ThemeParameters : public epg::params::ParametersT< TRANS_PARAMETERS >
	{
		typedef  epg::params::ParametersT< TRANS_PARAMETERS > Base;

		public:

			/// \brief
			ThemeParameters()
			{				
				_initParameter( SOURCE_ROAD_TABLE, "SOURCE_ROAD_TABLE" )  ;

				_initParameter(NATIONAL_IDENTIFIER, "NATIONAL_IDENTIFIER");
				_initParameter(EDGE_NATIONAL_ROAD_CODE, "EDGE_NATIONAL_ROAD_CODE");
				_initParameter(EDGE_EUROPEAN_ROAD_CODE, "EDGE_EUROPEAN_ROAD_CODE" ) ;

				_initParameter(CP_TABLE, "CP_TABLE");
				_initParameter(CL_TABLE, "CL_TABLE");
				_initParameter(CF_STATUS, "CF_STATUS");

				_initParameter(LIST_ATTR_TO_CONCAT, "LIST_ATTR_TO_CONCAT");
				_initParameter(LIST_ATTR_W, "LIST_ATTR_W");

				_initParameter(SQL_FILTER_EDGES_2_GENERATE_CF, "SQL_FILTER_EDGES_2_GENERATE_CF");

				_initParameter(LAND_COVER_TYPE, "LAND_COVER_TYPE");
				_initParameter(TYPE_LAND_AREA, "TYPE_LAND_AREA");

			}

			/// \brief
			~ThemeParameters()
			{
			}

			/// \brief
			virtual std::string getClassName()const
			{
				return "ThemeParameters";
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

	typedef epg::Singleton< ThemeParameters >  ThemeParametersS;

}
}

#endif