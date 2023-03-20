#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RTE_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RTE_H_

//SOCLE
#include <ign/feature/Feature.h>
#include <ign/feature/sql/FeatureStorePostgis.h>

//EPG
#include <epg/calcul/conversion/Convertor.h>
#include <epg/tools/StringTools.h>
#include <epg/utils/replaceTableName.h>

//APP
#include <app/params/ThemeParameters.h>



namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rte : public epg::calcul::conversion::Convertor
    {

    public:

        rte( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rte( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
/*
			epg::Context* context=epg::ContextS::getInstance();

			app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

			epg::params::EpgParameters & params = context->getEpgParameters();


			//std::string  const itineraireTableName=epg::utils::replaceTableName(transParameter->getValue(BDUNI_ITINERAIRE).toString());
			std::string  const itineraireTableName = transParameter->getValue( BDUNI_ITINERAIRE ).toString();

			std::string const idName=params.getValue(ID).toString();

			std::string const geomName=params.getValue(GEOM).toString();


			ign::feature::sql::FeatureStorePostgis * fsItineraire=context->getDataBaseManager().getFeatureStore( itineraireTableName , idName, geomName );


            ign::data::Variant targetValue ;

			
			IGN_ASSERT( featureSource.hasAttribute( "liens_vers_itineraire" ) ) ;

			std::string idItineraire = featureSource.getAttribute("liens_vers_itineraire").toString() ;


			if( idItineraire=="")
				targetValue = ign::data::String( "N_A" );

			else
			{
				std::vector<std::string> vIdItineraireParts;

				epg::tools::StringTools::Split( idItineraire, "/", vIdItineraireParts);

				bool isEuropeanRoad = false ;


				for(size_t i=0;i<vIdItineraireParts.size();++i){

					IGN_ASSERT(!vIdItineraireParts[i].empty());

					std::string idItiParts=vIdItineraireParts[i];

					if( idItiParts == "/" ) continue;

					ign::feature::Feature fItinieraire;
					
					fsItineraire->getFeatureById(idItiParts,fItinieraire);

					IGN_ASSERT( !fItinieraire.getId().empty() );

					IGN_ASSERT( fItinieraire.hasAttribute( "nature" ) ) ;

					IGN_ASSERT( fItinieraire.hasAttribute( "numero" ) ) ;

					if( fItinieraire.getAttribute("nature").toString()=="Route européenne" ) {

						if (isEuropeanRoad)
							targetValue = ign::data::String(targetValue.toString()+"#"+fItinieraire.getAttribute("numero").toString()) ;

						else
							targetValue = ign::data::String( fItinieraire.getAttribute("numero").toString() ) ;

						isEuropeanRoad=true;
					}

				}

				if ( ! isEuropeanRoad ) 
					targetValue = ign::data::String( "N_A" );

			}
*/


			IGN_ASSERT(featureSource.hasAttribute("cpx_numero_route_europeenne"));

			std::string numEuro = featureSource.getAttribute("cpx_numero_route_europeenne").toString();

			if (numEuro == "")
				featureTarget.setAttribute(targetAttribute.getName(), ign::data::String("UNK"));
			else
				featureTarget.setAttribute( targetAttribute.getName(), ign::data::String(numEuro));
        }

    };


}
}
}
}


#endif
