#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RTT_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RTT_H_

//SOCLE
#include <ign/feature/Feature.h>

//EPG
#include <epg/calcul/conversion/Convertor.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rtt : public epg::calcul::conversion::Convertor
    {

    public:

        rtt( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rtt( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;
			
			IGN_ASSERT( featureSource.hasAttribute("nature")) ;
			IGN_ASSERT(featureSource.hasAttribute("importance"));

			std::string nature = featureSource.getAttribute("nature").toString() ;
			std::string importance = featureSource.getAttribute("importance").toString();

			if (nature == "Type autoroutier")
				targetValue = ign::data::Integer(16);
			else if ( importance == "1" || importance == "2" )
				targetValue = ign::data::Integer( 14 );
			else if (importance == "3")
				targetValue = ign::data::Integer( 15 );
			else 
				targetValue = ign::data::Integer( 984 );

            featureTarget.setAttribute( targetAttribute.getName(), targetValue  );
        }

    };

}
}
}
}


#endif
