#ifndef _APP_CONVERSION_DATAMODELCONVERSION_H_
#define _APP_CONVERSION_DATAMODELCONVERSION_H_

#include < string >

//EPG
#include <epg/graph/EpgGraph.h>

namespace app{
namespace calcul{
namespace conversion{

	/// \brief
    void railrdLModelConversion(
        std::string const& sourceDataStoreName,
        std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
        ) ;

	/// \brief
	void railrdCModelConversion(
		std::string const& sourceDataStoreName,
		std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter,
		int countRStationID
        ) ;

	/// \brief
    void restCModelConversion(
        std::string const& sourceDataStoreName,
        std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
        ) ;

	/// \brief
    void interCCModelConversion(
        std::string const& sourceDataStoreName,
        std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
        ) ;

	/// \brief
    void levelCCModelConversion(
        std::string const& sourceDataStoreName,
        std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
        ) ;


	/// \brief
	void roadLModelConversion(
        std::string const& sourceDataStoreName,
        std::string const& targetDataStoreName,
		ign::feature::FeatureFilter filter
        ) ;

   
}
}
}


#endif
