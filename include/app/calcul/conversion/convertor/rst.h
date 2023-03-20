#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RST_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RST_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rst : public epg::calcul::conversion::Convertor
    {

    public:

        rst( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rst( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;
			
			IGN_ASSERT( featureSource.hasAttribute( "nature" ) ) ;

			std::string nature = featureSource.getAttribute("nature").toString() ;

			if ( nature == "Route empierrée" ) // A priori inutile car les routes empierrées ont dû être enlevées à la sélection
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
