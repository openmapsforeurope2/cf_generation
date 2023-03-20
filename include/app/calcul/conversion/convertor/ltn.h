#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_LTN_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_LTN_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class ltn : public epg::calcul::conversion::Convertor
    {

    public:

        ltn( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        ltn( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
			
			IGN_ASSERT( featureSource.hasAttribute( "nombre_de_voies" ) ) ;

			std::string nbVoies = featureSource.getAttribute("nombre_de_voies").toString();
			ign::data::Integer targetValue;

			if (nbVoies != "Sans objet")
			{

				targetValue = featureSource.getAttribute("nombre_de_voies").toInteger();

				if (targetValue.toInteger() == 0)
					targetValue = ign::data::Integer(-32768);
			}
			else
				targetValue = ign::data::Integer(-32768);
			
            featureTarget.setAttribute( targetAttribute.getName(), targetValue );
        }

    };


}
}
}
}


#endif
