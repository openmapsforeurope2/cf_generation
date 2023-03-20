#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_TUC_RWC_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_TUC_RWC_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class tuc_rwc : public epg::calcul::conversion::Convertor
    {

    public:

        tuc_rwc( epg::calcul::conversion::ConvertorAttribute const& sourceAttribute_,
             epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( sourceAttribute_, targetAttribute_ )
            {
            }

        tuc_rwc( std::string const& sourceAttributeName,
             epg::sql::SqlType::Type const& sourceType,
             std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( sourceAttributeName, sourceType, targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant const & sourceValue =  featureSource.getAttribute(sourceAttribute.get().getName()) ;

            ign::data::Variant targetValue ;


            if( sourceValue.isNull() )
                targetValue = ign::data::Integer( 0 );
            else
            {
                std::string nature = sourceValue.toString() ;

                if( nature == "Gare de voyageurs" ) targetValue = ign::data::Integer ( 26 ) ;
                else if ( nature == "Gare de fret" ) targetValue = ign::data::Integer ( 25 ) ;
                else if ( nature == "Gare de voyageurs et de fret" ) targetValue = ign::data::Integer ( 45 ) ;
				else targetValue = ign::data::Integer( 0 );

            }

            featureTarget.setAttribute( targetAttribute.getName(), targetValue  );
        }

    };


}
}
}
}


#endif
