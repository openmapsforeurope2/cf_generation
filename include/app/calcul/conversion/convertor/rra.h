#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RRA_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RRA_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rra : public epg::calcul::conversion::Convertor
    {

    public:

        rra( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rra( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;

			IGN_ASSERT( featureSource.hasAttribute( "energie" ) ) ;

			std::string energie = featureSource.getAttribute( "energie" ).toString() ;

			if( energie == "Electrifiée" )
				targetValue = ign::data::Integer( 3 );
			else if( energie == "Non électrifiée" )
				targetValue = ign::data::Integer( 4 );
			//"En cours d'électrification"
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
