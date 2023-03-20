#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_FCO_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_FCO_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class fco : public epg::calcul::conversion::Convertor
    {

    public:

        fco( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        fco( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;
			
			IGN_ASSERT( featureSource.hasAttribute( "nombre_de_voies" ) ) ;

			std::string nbVoies = featureSource.getAttribute( "nombre_de_voies" ).toString() ;

			if ( nbVoies == "1 voie" )
				targetValue = ign::data::Integer( 3 );
			else if ( nbVoies == "2 voies ou plus" )
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
