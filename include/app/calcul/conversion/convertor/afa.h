#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_AFA_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_AFA_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class afa : public epg::calcul::conversion::Convertor
    {

    public:

        afa( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        afa( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;
			
			IGN_ASSERT( featureSource.hasAttribute( "nature_detaillee" ) ) ;

			std::string nature = featureSource.getAttribute("nature_detaillee").toString() ;

			if ( nature == "Aire de service" )
				targetValue = ign::data::Integer( 9 );
			else if ( nature == "Aire de repos" )
				targetValue = ign::data::Integer( 999 );
			else 
				targetValue = ign::data::Integer( -32768 );

            featureTarget.setAttribute( targetAttribute.getName(), targetValue  );
        }

    };


}
}
}
}


#endif
