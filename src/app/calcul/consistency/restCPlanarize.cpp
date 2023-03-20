#include <app/calcul/consistency/restCPlanarize.h>

// BOOST
#include <boost/progress.hpp>

//EPG
#include <epg/context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/utils/splitEdge.h>
#include <epg/utils/createVertex.h>
#include <epg/sql/tools/UidGenerator.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/tools/FilterTools.h>
#include <epg/tools/geometry/project.h>

//APP
#include <app/params/ThemeParameters.h>


namespace app{
namespace calcul{
namespace consistency{

	void restCMerge(
		ign::feature::sql::FeatureStorePostgis* fsRestC,
		ign::feature::FeatureFilter filterProcessed
		)
	{

		std::map < std::string, ign::feature::Feature > mRestCToCreate;
		std::set<std::string> sRestCToDelete;

		double distMax = 1000;
		double threshold = 1e-5;

		epg::Context* context = epg::ContextS::getInstance();

		epg::params::EpgParameters& params = context->getEpgParameters();
		std::string geomName = params.getValue(GEOM).toString();
		std::string idName = params.getValue( ID ).toString();

		ign::feature::sql::FeatureStorePostgis* fsRoad = context->getFeatureStore( epg::EDGE );

		epg::sql::tools::UidGenerator* idRestCGenerator = new epg::sql::tools::UidGenerator( context->getPrefix() + "MERGERESTC", *fsRestC );

		ign::feature::FeatureIteratorPtr itRestC = fsRestC->getFeatures( filterProcessed );


		while (itRestC->hasNext()) {

			ign::feature::Feature fRestC = itRestC->next();

			//deja traite
			if (sRestCToDelete.find(fRestC.getId()) != sRestCToDelete.end())
				continue;

			ign::feature::FeatureFilter filterRestCNear(idName + "!='" + fRestC.getId()+"'");
			filterRestCNear.setExtent(fRestC.getExtent().expandBy(distMax));

			ign::feature::FeatureIteratorPtr itRestCNear = fsRestC->getFeatures(filterRestCNear);

			while (itRestCNear->hasNext()) {

				ign::feature::Feature fRestCNear = itRestCNear->next();
				if (fRestC.getId() == fRestCNear.getId())
					continue;

				if (fRestCNear.getGeometry().distance(fRestC.getGeometry()) > distMax)
					continue;

				sRestCToDelete.insert(fRestCNear.getId());
				sRestCToDelete.insert(fRestC.getId());

				ign::geometry::Point ptRestCNew;
				ign::geometry::Point ptRestC = fRestC.getGeometry().asPoint();
				ign::geometry::Point ptRestCNear = fRestCNear.getGeometry().asPoint();
				ptRestCNew.setX(ptRestC.x() + 0.5*(ptRestCNear.x() - ptRestC.x()));
				ptRestCNew.setY(ptRestC.y() + 0.5*(ptRestCNear.y()- ptRestC.y()));

				ign::feature::Feature featRestCNew = fRestC;

				std::string idRestCNew = idRestCGenerator->next();
				featRestCNew.setId(idRestCNew);

				featRestCNew.setGeometry(ptRestCNew);
				
				//recuperation afa
				std::string valueAfaRestNew;
				if (fRestC.getAttribute("afa").toString() == "9" || fRestCNear.getAttribute("afa").toString() == "9")
					valueAfaRestNew = "9";
				else
					valueAfaRestNew = "999";
				featRestCNew.setAttribute("afa", ign::data::String(valueAfaRestNew));

				//set namn1 et nama1
				std::set< std::string> sNamn;
				sNamn.insert(fRestC.getAttribute("namn1").toString());
				sNamn.insert(fRestCNear.getAttribute("namn1").toString());
				std::set< std::string>::iterator sitRestNamn = sNamn.begin();

				
				std::string namnRestNew = *sitRestNamn;
				if (sNamn.size() > 1) {
					++sitRestNamn;
					std::string namnRestc2 = *sitRestNamn;
					if (namnRestc2 != "UNK" && namnRestc2 != "")
						namnRestNew += "/" + namnRestc2;
				}				
				featRestCNew.setAttribute("namn1", ign::data::String(namnRestNew));

				std::set< std::string> sNama;
				sNama.insert(fRestC.getAttribute("nama1").toString());
				sNama.insert(fRestCNear.getAttribute("nama1").toString());
				std::set< std::string>::iterator sitRestNama = sNama.begin();

				std::string namaRestNew = *sitRestNama;
				if (sNamn.size() > 1) {
					++sitRestNama;
					std::string namaRestc2 = *sitRestNamn;
					if (namaRestc2 != "UNK" && namaRestc2 != "")
						namaRestNew += "/" + namaRestc2;
				}
				featRestCNew.setAttribute("nama1", ign::data::String(namaRestNew));

				mRestCToCreate.insert(std::make_pair(idRestCNew, featRestCNew));				
			}
		}

		for (std::set<std::string>::iterator sit = sRestCToDelete.begin(); sit != sRestCToDelete.end(); ++sit)
			fsRestC->deleteFeature(*sit);

		for (std::map < std::string, ign::feature::Feature >::iterator mit = mRestCToCreate.begin(); mit != mRestCToCreate.end(); ++mit)
			fsRestC->createFeature(mit->second, mit->first);
		
	}


	void projOnRoadRestC(
		ign::feature::sql::FeatureStorePostgis* fsRestC,
		ign::feature::FeatureFilter filterProcessed
	)
	{
		double distMax = 500;
		epg::log::ScopeLogger log("app::calcul::consistency::projOnRoadRestC");

		epg::Context* context = epg::ContextS::getInstance();
		epg::params::EpgParameters& params = context->getEpgParameters();

		ign::feature::FeatureIteratorPtr itRestC = fsRestC->getFeatures(filterProcessed);

		ign::feature::sql::FeatureStorePostgis* fsRoad = context->getFeatureStore(epg::EDGE);

		std::set<std::string> sRestNoProj;

		while (itRestC->hasNext()) {
			double distMin = distMax;
			ign::feature::Feature fRestC = itRestC->next();

			ign::feature::FeatureFilter filterAutorout("rtt = '16'");
			filterAutorout.setExtent(fRestC.getExtent().expandBy(distMin));
			ign::geometry::Point ptRestC = fRestC.getGeometry().asPoint();

			ign::feature::FeatureIteratorPtr itAutorout = fsRoad->getFeatures(filterAutorout);

			ign::geometry::LineString lsClosestAutorout;

			while (itAutorout->hasNext()) {
				ign::feature::Feature featAutorout = itAutorout->next();
				double dist = featAutorout.getGeometry().distance(ptRestC);
				if (dist < distMin) {
					distMin = dist;
					lsClosestAutorout = featAutorout.getGeometry().asLineString();
				}
			}
			
			//aucune selection d'autoroute, ne devrait pas arriver
			if (distMin == distMax) {
				sRestNoProj.insert(fRestC.getId());
				continue;
			}

			ign::geometry::Point ptProj = epg::tools::geometry::project(lsClosestAutorout, ptRestC);
			if (ptProj.distance(lsClosestAutorout.startPoint()) < 50)
				ptProj = lsClosestAutorout.startPoint();
			else if (ptProj.distance(lsClosestAutorout.endPoint()) < 50)
				ptProj = lsClosestAutorout.endPoint();

			fRestC.setGeometry(ptProj);

			fsRestC->modifyFeature(fRestC);

		}

		for (std::set<std::string>::iterator sit = sRestNoProj.begin(); sit != sRestNoProj.end(); ++sit) {
			fsRestC->deleteFeature(*sit);
			log.log(epg::log::INFO, "Suppression du RestC "+ ign::data:: String(*sit).toString ()+", pas de reseau autoroutier à moins de 500 m" );
		}

	}


}
}
}
