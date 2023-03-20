#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_EXS_RD_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_EXS_RD_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class exs_rd : public epg::calcul::conversion::Convertor
    {

    public:

        exs_rd( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        exs_rd(std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {

			ign::data::Variant targetValue ;

			IGN_ASSERT( featureSource.hasAttribute( "etat_de_l_objet" ) ) ;

			std::string etatObjet = featureSource.getAttribute( "etat_de_l_objet" ).toString() ;

			if ( etatObjet == "En construction" || etatObjet == "En projet" )
				targetValue = ign::data::Integer( 5 );
			else if ( etatObjet == "En service" )
				targetValue = ign::data::Integer( 28 );
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
