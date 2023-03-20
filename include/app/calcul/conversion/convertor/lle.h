#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_LLE_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_LLE_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class lle : public epg::calcul::conversion::Convertor
    {

    public:

        lle( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        lle( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
            ign::data::Variant targetValue ;
			
			IGN_ASSERT( featureSource.hasAttribute( "position_par_rapport_au_sol" ) ) ;

			std::string niveauAuSol = featureSource.getAttribute("position_par_rapport_au_sol").toString() ;

			if ( niveauAuSol == "-3" || niveauAuSol == "-4")
				targetValue = ign::data::Integer( -9 );

			else if (niveauAuSol == "-2")
				targetValue = ign::data::Integer(-2);

			else if (niveauAuSol == "-1")
				targetValue = ign::data::Integer(-1);

			else if (niveauAuSol == "0" || niveauAuSol == "Gué ou radier" )
				targetValue = ign::data::Integer( 1 );

			else if ( niveauAuSol == "1")
				targetValue = ign::data::Integer(2);

			else if (niveauAuSol == "2")
				targetValue = ign::data::Integer(3);

			else if (niveauAuSol == "3" || niveauAuSol == "4")
				targetValue = ign::data::Integer(9);

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
