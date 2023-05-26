
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

		TMP_CP_TABLE,
		TMP_CL_TABLE,

		LIST_ATTR_TO_CONCAT

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

				_initParameter(NATIONAL_IDENTIFIER, "NATIONAL_IDENTIFIER");
				_initParameter(EDGE_NATIONAL_ROAD_CODE, "EDGE_NATIONAL_ROAD_CODE");
				_initParameter(EDGE_EUROPEAN_ROAD_CODE, "EDGE_EUROPEAN_ROAD_CODE" ) ;

				_initParameter(TMP_CP_TABLE, "TMP_CP_TABLE");
				_initParameter(TMP_CL_TABLE, "TMP_CL_TABLE");

				_initParameter(LIST_ATTR_TO_CONCAT, "LIST_ATTR_TO_CONCAT");
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