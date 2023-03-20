#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RSU_RD_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RSU_RD_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rsu_rd : public epg::calcul::conversion::Convertor
    {

    public:

        rsu_rd( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rsu_rd( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue;

			IGN_ASSERT( featureSource.hasAttribute( "periode_de_fermeture" ) ) ;

			std::string fermeture = featureSource.getAttribute("periode_de_fermeture").toString() ;

			if ( fermeture == "" )
				targetValue = ign::data::Integer( 2 );
			else
				targetValue = ign::data::Integer( 1 );

            featureTarget.setAttribute( targetAttribute.getName(), targetValue  );
        }

    };


}
}
}
}


#endif
