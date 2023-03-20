#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RRC_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RRC_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rrc : public epg::calcul::conversion::Convertor
    {

    public:

        rrc( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rrc( std::string const& targetAttributeName,
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

			if ( nature == "Voie normale" || nature == "TGV" )
				targetValue = ign::data::Integer( 16 );
			else if( nature == "Embranchement particulier" || nature == "Voie de triage" )
				targetValue = ign::data::Integer( 17 );
			else if( nature == "Voie à crémaillère" || nature == "Funiculaire" )
				targetValue = ign::data::Integer( 999 );
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
