#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RGC_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RGC_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rgc : public epg::calcul::conversion::Convertor
    {

    public:

        rgc( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rgc( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;

			IGN_ASSERT( featureSource.hasAttribute( "largeur" ) ) ;

			std::string largeur = featureSource.getAttribute( "largeur" ).toString() ;

			if ( largeur == "Normale" )
				targetValue = ign::data::Integer( 3 );
			else if( largeur == "Large" )
				targetValue = ign::data::Integer( 1 );
			else if( largeur == "Etroite" )
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
