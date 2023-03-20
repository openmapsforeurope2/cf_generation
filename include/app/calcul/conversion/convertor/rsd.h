#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RSD_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RSD_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rsd : public epg::calcul::conversion::Convertor
    {

    public:

        rsd( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rsd( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;

			IGN_ASSERT( featureSource.hasAttribute( "nature" ) ) ;

			std::string nature = featureSource.getAttribute( "nature" ).toString() ;

			if ( nature == "Voie normale" )
				targetValue = ign::data::Integer( 1 );
			else if (  nature == "TGV" )
				targetValue = ign::data::Integer( 2 );
			else 
				targetValue = ign::data::Integer( 0 );

            featureTarget.setAttribute( targetAttribute.getName(), targetValue  );

        }

    };


}
}
}
}


#endif
