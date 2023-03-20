#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RTN_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RTN_H_


//SOCLE
#include <ign/feature/Feature.h>

//EPG
#include <epg/calcul/conversion/Convertor.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/replaceTableName.h>

//APP
#include <app/params/ThemeParameters.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rtn : public epg::calcul::conversion::Convertor
    {

    public:

        rtn( epg::calcul::conversion::ConvertorAttribute const& targetAttribute_
            ):
            epg::calcul::conversion::Convertor( targetAttribute_ )
            {
            }

        rtn( std::string const& targetAttributeName,
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

			epg::log::ScopeLogger log;


			//std::string  const routeNumTableName = epg::utils::replaceTableName(transParameter->getValue(BDUNI_ROUTE_NUMEROTEE).toString());
			std::string  const routeNumTableName = transParameter->getValue( BDUNI_ROUTE_NUMEROTEE ).toString();

			std::string const idName=params.getValue(ID).toString();

			std::string const geomName=params.getValue(GEOM).toString();


			ign::feature::sql::FeatureStorePostgis * fsRouteNum=context->getDataBaseManager().getFeatureStore( routeNumTableName , idName, geomName );


            ign::data::Variant targetValue ;

			
			IGN_ASSERT( featureSource.hasAttribute( "liens_vers_route_numerotee" ) ) ;

			std::string idRouteNum = featureSource.getAttribute("liens_vers_route_numerotee").toString() ;


			if( idRouteNum=="")
				targetValue = ign::data::String( "UNK" );

			else
			{
				ign::feature::Feature fRouteNum;
				fsRouteNum->getFeatureById(idRouteNum,fRouteNum);

//				IGN_ASSERT(!fRouteNum.getId().empty());
				if ( fRouteNum.getId().empty() )
				{
					log.log( epg::log::ERROR, idRouteNum + " n'existe pas dans la table des routes numerotees - rtn non recupere pour le troncon " + featureTarget.getId() + "." );
					targetValue = ign::data::String( "UNK" );
				}
				
				else
				{
					IGN_ASSERT( fRouteNum.hasAttribute( "numero_de_route" ) );

					if ( fRouteNum.getAttribute( "numero_de_route" ).toString() != "" )
						targetValue = ign::data::String( fRouteNum.getAttribute( "numero_de_route" ).toString() );

					else
						targetValue = ign::data::String( "UNK" );
				}
				

			}


            featureTarget.setAttribute( targetAttribute.getName(), targetValue  );
*/
			app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();
			std::string sourceAttributeName = transParameter->getValue(EDGE_ROAD_CODE).toString();
			IGN_ASSERT(featureSource.hasAttribute(sourceAttributeName));

			std::string numRoute = featureSource.getAttribute(sourceAttributeName).toString();

			if (numRoute == "")
				featureTarget.setAttribute(targetAttribute.getName(), ign::data::String("UNK"));
			else
				featureTarget.setAttribute(targetAttribute.getName(), ign::data::String(numRoute));


        }

    };


}
}
}
}


#endif
