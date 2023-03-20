#include <app/calcul/selection/selectionRestC.h>

// BOOST
#include <boost/progress.hpp>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/sql/tools/numFeatures.h>

////TRANS_ERM
#include <app/params/ThemeParameters.h>


namespace app{
namespace calcul{
namespace selection{

	void selectionRestC( std::string  const & restCTableName, ign::feature::FeatureFilter filterProcessed, double linkThreshold )
	{

		epg::log::ScopeLogger log( "app::calcul::selection::selectionRestC" );
		epg::Context* context = epg::ContextS::getInstance();
		epg::params::EpgParameters & params = context->getEpgParameters();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		std::string const netTypeName = params.getValue( NET_TYPE ).toString();
		std::string const idName = params.getValue( ID ).toString();
		std::string const geomName = params.getValue( GEOM ).toString();
		std::string const& linkedFeatidName = params.getValue(LINKED_FEATURE_ID).toString();

		if ( !context->getDataBaseManager().columnExists( restCTableName, netTypeName ) )
			context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + restCTableName + " ADD COLUMN " + netTypeName + " integer default 0" );

		if (!context->getDataBaseManager().columnExists(restCTableName, linkedFeatidName))
			context->getDataBaseManager().getConnection()->update("ALTER TABLE " + restCTableName + " ADD COLUMN " + linkedFeatidName + " character varying");

		ign::feature::sql::FeatureStorePostgis* fsEdge = context->getFeatureStore(epg::EDGE);
		ign::feature::sql::FeatureStorePostgis* fsRestC = context->getDataBaseManager().getFeatureStore( restCTableName, idName, geomName );
		ign::feature::FeatureIteratorPtr itRestC = fsRestC->getFeatures( filterProcessed );

		int nb_iter = epg::sql::tools::numFeatures(*fsRestC, filterProcessed);
		log.log(epg::log::INFO, "Nombre d'objets à lier : " + ign::data::Integer(nb_iter).toString());

		boost::progress_display display(nb_iter, std::cout, "[link table " + restCTableName + " with a threshold of " + ign::data::Double(linkThreshold).toString() + " running % complete ]\n");

		while (itRestC->hasNext() ) 
		{			
			++display;

			ign::feature::Feature fRestC = itRestC->next();

			std::string idLinkedEdge = "";
			double distMin = linkThreshold + 1;

			//Recuperation de la zone de recherche des edges en fonction du seuil
			ign::geometry::Envelope envPoi;
			fRestC.getGeometry().envelope(envPoi);
			envPoi.expandBy(linkThreshold);
		
			ign::feature::FeatureFilter filterMotorway("nature = 'Type autoroutier' AND " + netTypeName + " >= 10");

			filterMotorway.setExtent(envPoi);
			ign::feature::FeatureIteratorPtr eit = fsEdge->getFeatures(filterMotorway);

			// Recherche des edges selectionnes a une distance inferieur au seuil
			if (eit->hasNext())
			{
				fRestC.setAttribute( netTypeName, ign::data::Integer(10));
				fsRestC->modifyFeature(fRestC);
			}
		}
	}

}
}
}
