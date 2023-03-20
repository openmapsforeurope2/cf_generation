#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_TOL_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_TOL_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class tol : public epg::calcul::conversion::Convertor
    {

    public:

        tol( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        tol( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;
			
			IGN_ASSERT( featureSource.hasAttribute( "acces_vehicule_leger" ) ) ;

			std::string acces = featureSource.getAttribute("acces_vehicule_leger").toString() ;

			if ( acces == "A péage" )
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
