#include <app/step/100_TmpCFGeneration.h>

//BOOST
#include <boost/timer.hpp>
#include <boost/progress.hpp>

//EPG
#include <epg/Context.h>
#include <epg/io/query/CountryQueriesReader.h>
#include <epg/io/query/utils/queryUtils.h>
#include <epg/tools/TimeTools.h>
#include <epg/utils/CopyTableUtils.h>
#include <epg/utils/replaceTableName.h>
#include <epg/sql/tools/IdGeneratorFactory.h>

//APP
#include <app/tools/CountryQueryErmTrans.h>


namespace app {
namespace step {

	///
	///
	///
	void TmpCFGeneration::init()
	{
		addWorkingEntity(SOURCE_ROAD_TABLE);
		addWorkingEntity(TMP_CP_TABLE);
		addWorkingEntity(TMP_CL_TABLE);
	}


	///
	///
	///
	void TmpCFGeneration::onCompute( bool verbose )
	{

		epg::Context* context = epg::ContextS::getInstance();

		_epgLogger.log( epg::log::TITLE, "[ BEGIN TMP CF GENERATION ] : " + epg::tools::TimeTools::getTime() );

		std::string const idName = _epgParams.getValue( ID ).toString();
		std::string const geomName = _epgParams.getValue( GEOM ).toString();

		std::string const natIdName = _themeParams.getValue(NATIONAL_IDENTIFIER).toString();
		std::string const natRoadCodeName = _themeParams.getValue(EDGE_NATIONAL_ROAD_CODE).toString();
		std::string const euRoadCodeName = _themeParams.getValue(EDGE_EUROPEAN_ROAD_CODE).toString();

		//Lecture du fichier de requetes
		epg::io::query::CountryQueriesReader< app::tools::CountryQueryErmTrans > countryQueriesReader;
		countryQueriesReader.read( _context.getQueryFile() );
		std::vector< app::tools::CountryQueryErmTrans > vQueries = countryQueriesReader.countryQueries();

		std::string sqlProcessedCountries = epg::io::query::utils::sqlWhereProcessed( vQueries );
		std::string sqlNotInQueryFile = epg::io::query::utils::sqlNotInQueryFile( vQueries );
		std::string sqlInQueryFile = epg::io::query::utils::sqlInQueryFile( vQueries );

		// Parametres pour la definition des tables frontieres
		std::string countryCodeName = _epgParams.getValue( COUNTRY_CODE ).toString();
		std::string const boundaryTableName = _epgParams.getValue( TARGET_BOUNDARY_TABLE ).toString();

		std::string edgeSourceTableName = _themeParams.getValue(SOURCE_ROAD_TABLE).toString();
		std::string edgeTargetTableName = getCurrentWorkingTableName(SOURCE_ROAD_TABLE);
		_epgParams.setParameter(EDGE_TABLE, ign::data::String(edgeTargetTableName));

		epg::utils::CopyTableUtils::copyEdgeTable(edgeSourceTableName, "", false);

		ign::feature::sql::FeatureStorePostgis* fsEdge = _context.getFeatureStore(epg::EDGE);

		// Create tmp_cf table
		std::string tmpCpTableName = getCurrentWorkingTableName(TMP_CP_TABLE);

		std::ostringstream ss;
		ss << "DROP TABLE IF EXISTS " << tmpCpTableName << " ;"
			<< "CREATE TABLE " << tmpCpTableName
			<< " AS TABLE " << edgeTargetTableName
			<< " WITH NO DATA;"
			<< "ALTER TABLE " << tmpCpTableName << " ALTER COLUMN " 
			<< geomName << " type geometry(PointZ, 0);"
			<< "ALTER TABLE " << tmpCpTableName << " ALTER COLUMN "
			<< idName << " type varchar(255);";

		_context.getDataBaseManager().getConnection()->update(ss.str());

		// Create tmp_cf table
		std::string tmpClTableName = getCurrentWorkingTableName(TMP_CL_TABLE);

		ss.clear();
		ss << "DROP TABLE IF EXISTS " << tmpClTableName << " ;"
			<< "CREATE TABLE " << tmpClTableName
			<< " AS TABLE " << edgeTargetTableName
			<< " WITH NO DATA;"
			<< "ALTER TABLE " << tmpClTableName << " ALTER COLUMN "
			<< idName << " type varchar(255);";
		_context.getDataBaseManager().getConnection()->update(ss.str());

		// Go through objects intersecting the boundary
		ign::feature::sql::FeatureStorePostgis* fsBoundary = _context.getDataBaseManager().getFeatureStore(boundaryTableName, idName, geomName);
		ign::feature::FeatureIteratorPtr itBoundary = fsBoundary->getFeatures(ign::feature::FeatureFilter("country = 'be#fr'"));

		ign::feature::sql::FeatureStorePostgis* fsTmpCP = _context.getDataBaseManager().getFeatureStore(tmpCpTableName, idName, geomName);
		ign::feature::sql::FeatureStorePostgis* fsTmpCL = _context.getDataBaseManager().getFeatureStore(tmpClTableName, idName, geomName);

		// id generator
		epg::sql::tools::IdGeneratorInterfacePtr idGeneratorCP(epg::sql::tools::IdGeneratorFactory::getNew(*fsTmpCP, "CONNECTINGPOINT"));
		epg::sql::tools::IdGeneratorInterfacePtr idGeneratorCL(epg::sql::tools::IdGeneratorFactory::getNew(*fsTmpCL, "CONNECTINGLINE"));


		while (itBoundary->hasNext())
		{
			ign::feature::Feature fBoundary = itBoundary->next();
			ign::geometry::LineString lsBoundary = fBoundary.getGeometry().asLineString();

			ign::feature::FeatureFilter filterFeaturesToMatch("ST_INTERSECTS(" + geomName + ", '" + lsBoundary.toString() + "')");
			ign::feature::FeatureIteratorPtr itFeaturesToMatch = fsEdge->getFeatures(filterFeaturesToMatch);

			int numFeatures = context->getDataBaseManager().numFeatures(*fsEdge, filterFeaturesToMatch);
			boost::progress_display display(numFeatures, std::cout, "[ CREATE CONNECTING FEATURES ]\n");

			while (itFeaturesToMatch->hasNext())
			{

				++display;

				// TO-DO : créer point d'intersection
				ign::feature::Feature fToMatch = itFeaturesToMatch->next();
				ign::geometry::LineString const& lsFToMatch = fToMatch.getGeometry().asLineString();
				ign::geometry::Geometry* geomPtr = lsFToMatch.Intersection(lsBoundary);

				if (geomPtr->isNull())
					continue;

				ign::feature::Feature fCF = fToMatch;

				if (geomPtr->isPoint())
				{
					fCF.setGeometry(geomPtr->asPoint());
					fsTmpCP->createFeature(fCF, idGeneratorCP->next());
					continue;
				}
				
				if (geomPtr->isGeometryCollection())
				{
					ign::geometry::GeometryCollection geomCollect = geomPtr->asGeometryCollection();
					for (size_t i = 0; i < geomCollect.numGeometries(); ++i)
					{
						if (geomCollect.geometryN(i).isPoint())
						{
							fCF.setGeometry(geomCollect.geometryN(i).asPoint());
							fsTmpCP->createFeature(fCF, idGeneratorCP->next());
							continue;
						}
						else if (geomCollect.geometryN(i).isLineString())
						{
							fCF.setGeometry(geomCollect.geometryN(i).asLineString());
							fsTmpCL->createFeature(fCF, idGeneratorCL->next());
							continue;
						}
						// Autre cas ?? à loguer

					}
				}
			}

		}

		_epgLogger.log( epg::log::TITLE, "[ END TMP CF GENERATION ] : " + epg::tools::TimeTools::getTime() );

	}
}
}
