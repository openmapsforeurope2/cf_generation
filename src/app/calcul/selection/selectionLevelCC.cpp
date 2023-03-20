#include <app/calcul/selection/selectionLevelCC.h>


//BOOST
#include <boost/progress.hpp>


//EPG
#include <epg/Context.h>
#include <epg/log/EpgLogger.h>
#include <epg/tools/FilterTools.h>
#include <epg/sql/tools/UidGenerator.h>
#include <app/params/ThemeParameters.h>





namespace app{
namespace calcul{
namespace selection{

	///
	///
	///
	void selectionLevelCC(std::string levelCCTableName, std::string edgeRailwayTableName, std::string edgeRoadTableName, ign::feature::FeatureFilter filterProcessed )
	{

		epg::Context* context=epg::ContextS::getInstance();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();
		epg::params::EpgParameters & params=context->getEpgParameters();
		epg::log::EpgLogger* logger= epg::log::EpgLoggerS::getInstance();

		std::string const idName = params.getValue(ID).toString();
		std::string const geomName = params.getValue(GEOM).toString();
		std::string const netTypeName = params.getValue(NET_TYPE).toString();
		std::string const countryCodeName = params.getValue(COUNTRY_CODE).toString();
		std::string const posSolName = transParameter->getValue(EDGE_POSITION_SOL).toString();

	
		//Creation de la table LevelCC
		std::ostringstream ss;
		ss << "DROP TABLE IF EXISTS " << levelCCTableName << ";"
			<< "CREATE TABLE " << levelCCTableName << " ( " << idName << " character varying NOT NULL, " << geomName << " geometry, " << countryCodeName << " character varying ) WITH (OIDS=TRUE) ;"
			<< "ALTER TABLE " << levelCCTableName << " ADD CONSTRAINT " << levelCCTableName << "_pkey PRIMARY KEY (" << idName << ");"
			<< "ALTER TABLE " << levelCCTableName << " ADD CONSTRAINT enforce_dims_geom CHECK (st_ndims(" << geomName << ") = 2);"
			<< "ALTER TABLE " << levelCCTableName << " ADD CONSTRAINT enforce_geotype_geom CHECK (geometrytype(" << geomName << ") = 'POINT'::text OR " << geomName << " IS NULL);"
			<< "ALTER TABLE " << levelCCTableName << " ADD CONSTRAINT enforce_srid_geom CHECK (st_srid(" << geomName << ") = (0)); "
			<< "CREATE INDEX " << levelCCTableName + "_index_geom ON " << levelCCTableName << " USING GIST (" << geomName << ");"
			<< "CREATE UNIQUE INDEX " << levelCCTableName << "_" << idName << "_idx ON " << levelCCTableName << " (" << idName << ");";

		context->getDataBaseManager().getConnection()->update( ss.str() );

////////////////////////////
		// Patch 2022 car réutilisation de RailrdL issu d'ERM 2021 qui n'est donc pas dans les specs BDUni
		std::ostringstream ssRailrdl;
		ssRailrdl << "ALTER TABLE " << edgeRailwayTableName << " ADD COLUMN " << posSolName
			<< " character varying(10) default '0'";
		context->getDataBaseManager().getConnection()->update(ssRailrdl.str());

		ss << "UPDATE " << edgeRailwayTableName << " SET " << posSolName << " = '-3' "
			<< "WHERE lle = -9;";
		context->getDataBaseManager().getConnection()->update(ss.str());

		ss << "UPDATE " << edgeRailwayTableName << " SET " << posSolName << " = '-2' "
			<< "WHERE lle = -2;";
		context->getDataBaseManager().getConnection()->update(ss.str());

		ss << "UPDATE " << edgeRailwayTableName << " SET " << posSolName << " = '-1' "
			<< "WHERE lle = -1;";
		context->getDataBaseManager().getConnection()->update(ss.str());

		ss << "UPDATE " << edgeRailwayTableName << " SET " << posSolName << " = '1' "
			<< "WHERE lle = 2;";
		context->getDataBaseManager().getConnection()->update(ss.str());

		ss << "UPDATE " << edgeRailwayTableName << " SET " << posSolName << " = '2' "
			<< "WHERE lle = -9;";
		context->getDataBaseManager().getConnection()->update(ss.str());

		ss << "UPDATE " << edgeRailwayTableName << " SET " << posSolName << " = '3' "
			<< "WHERE lle = 9;";
		context->getDataBaseManager().getConnection()->update(ss.str());

//////////////////////////

		ign::feature::sql::FeatureStorePostgis* fsLevelCC = context->getDataBaseManager().getFeatureStore( levelCCTableName, idName, geomName ) ;
		ign::feature::sql::FeatureStorePostgis* fsRoadL = context->getDataBaseManager().getFeatureStore( edgeRoadTableName, idName, geomName ) ;
		ign::feature::sql::FeatureStorePostgis* fsRailwayL = context->getDataBaseManager().getFeatureStore( edgeRailwayTableName, idName, geomName ) ;

		epg::sql::tools::UidGenerator* idLevelCCGenerator = new epg::sql::tools::UidGenerator(context->getPrefix() + "LVLCC", *fsLevelCC);

		std::string condSelected = netTypeName + " > 9 or " + netTypeName + " = 5";
		epg::tools::FilterTools::addAndConditions(filterProcessed, condSelected);

		ign::feature::FeatureIteratorPtr itRailwayL = fsRailwayL->getFeatures(filterProcessed);

		while (itRailwayL->hasNext()) {

			ign::feature::Feature fRailwayL = itRailwayL->next();

			ign::geometry::LineString lsRailwayL = fRailwayL.getGeometry().asLineString();

			std::string posSolRailway = fRailwayL.getAttribute(posSolName).toString();
			std::string condPosAuSol = posSolName + " = '" + posSolRailway+"'";

			ign::feature::FeatureFilter filterRailwayL;
			epg::tools::FilterTools::addAndConditions(filterRailwayL, condSelected);
			epg::tools::FilterTools::addAndConditions(filterRailwayL, condPosAuSol);
			filterRailwayL.setExtent(lsRailwayL.getEnvelope());

			ign::feature::FeatureIteratorPtr itRoadL = fsRoadL->getFeatures(filterRailwayL);

			while (itRoadL->hasNext()) {

				ign::feature::Feature fRoadL = itRoadL->next();
				ign::geometry::LineString lsRoadL = fRoadL.getGeometry().asLineString();

				if (!lsRoadL.intersects(lsRailwayL))
					continue;

				ign::geometry::Geometry* geomIntersect = lsRoadL.Intersection(lsRailwayL);

				ign::geometry::Point ptLevelcc;
				std::string warnMsg = "LevelcC: Intersection non ponctuelle pour le passage a niveau entre l'objet " + fRailwayL .getId() + " et " + fRoadL.getId();

				if (geomIntersect->isPoint())
					ptLevelcc = geomIntersect->asPoint();

				else if (geomIntersect->isLineString())
				{
					ptLevelcc = geomIntersect->asLineString().startPoint();
					logger->log(epg::log::WARN, warnMsg);
				}

				else if (geomIntersect->isMultiPoint()) 
				{
					ptLevelcc = geomIntersect->asMultiPoint().pointN(0);
					logger->log(epg::log::WARN, warnMsg);
				}

				else if (geomIntersect->isMultiLineString()) 
				{
					ptLevelcc = geomIntersect->asMultiLineString().lineStringN(0).startPoint();
					logger->log(epg::log::WARN, warnMsg);
				}

				ign::feature::Feature fLevelcc = fsLevelCC->newFeature();

				std::string idLevelCC = idLevelCCGenerator->next();
				fLevelcc.setId(idLevelCC);
				fLevelcc.setGeometry(ptLevelcc);
				fLevelcc.setAttribute(countryCodeName, fRailwayL.getAttribute(countryCodeName));
				fsLevelCC->createFeature(fLevelcc, idLevelCC);

			}					
		}
		delete idLevelCCGenerator;
	}
	
}
}
}