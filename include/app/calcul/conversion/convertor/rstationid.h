#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RSTATIONID_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RSTATIONID_H_

//SOCLE
#include <ign/feature/Feature.h>

//EPG
#include <epg/calcul/conversion/Convertor.h>
#include <epg/utils/replaceTableName.h>
#include <epg/tools/StringTools.h>

//APP
#include <app/params/ThemeParameters.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rstationid : public epg::calcul::conversion::Convertor
    {

    public:

        rstationid( 
             std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType,
			 int countRStationID
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )
            {
				_countRStationID=countRStationID;
            }

        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
			
			epg::Context* context=epg::ContextS::getInstance();

			app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();
			
			epg::params::EpgParameters & params = context->getEpgParameters();


			std::string const idName=params.getValue(ID).toString();

			std::string const geomName=params.getValue(GEOM).toString();


			ign::data::Variant targetValue ;  


			IGN_ASSERT( featureTarget.hasAttribute("namn1"));

			std::string namn1Attribute;


			std::vector<std::string> vNamn1AttributeParts;

			epg::tools::StringTools::Split( featureTarget.getAttribute("namn1").toString(), "'", vNamn1AttributeParts);

			for(size_t i=0;i<vNamn1AttributeParts.size();++i){

				IGN_ASSERT(!vNamn1AttributeParts[i].empty());

				if( vNamn1AttributeParts[i] == "'" ) 

					namn1Attribute+="'";

				namn1Attribute+=vNamn1AttributeParts[i];

			}


			ign::feature::FeatureFilter filterRailRdC( "LOWER(namn1) = LOWER('"+namn1Attribute+"')" );

			std::string  const railRdCOldTableName=epg::utils::replaceTableName(transParameter->getValue(ERM_RAILRDC_OLD_TABLE).toString());

			ign::feature::sql::FeatureStorePostgis* fsRailRdCOld=context->getDataBaseManager().getFeatureStore(railRdCOldTableName,idName,geomName);

			IGN_ASSERT (fsRailRdCOld->getFeatureType ().hasAttribute("namn1"));


			ign::feature::FeatureIteratorPtr it=fsRailRdCOld->getFeatures(filterRailRdC);


			double distMaxAppariement = 10000;
			double distMin=distMaxAppariement;


			if(!it->hasNext()){

				std::string countRStationIDNew=ign::data::Integer(_countRStationID++).toString();

				//IGN_ASSERT(idRSttationIDNew.size()<7);

				while(countRStationIDNew.size()<6)

					countRStationIDNew="0"+countRStationIDNew;

				targetValue = ign::data::String( "N.FR.RAILRDC"+countRStationIDNew );

			}


			while (it->hasNext()){

				ign::feature::Feature fRailRdCOld=it->next();

				IGN_ASSERT( fRailRdCOld.hasAttribute(targetAttribute.getName()) );

				double dist=featureSource.getGeometry().asPoint().distance(fRailRdCOld.getGeometry().asPoint());

				if(dist<distMin) {

					distMin=dist;

					targetValue = ign::data::String( fRailRdCOld.getAttribute( targetAttribute.getName() ).toString() );
				}

			}

			if( distMin == distMaxAppariement) {
				std::string countRStationIDNew=ign::data::Integer(_countRStationID++).toString();

				while(countRStationIDNew.size()<6)

					countRStationIDNew="0"+countRStationIDNew;

				targetValue = ign::data::String( "N.FR.RAILRDC"+countRStationIDNew );

			}

            featureTarget.setAttribute( targetAttribute.getName(), targetValue  );
        }

	private:
		mutable int  _countRStationID;


    };


}
}
}
}


#endif
