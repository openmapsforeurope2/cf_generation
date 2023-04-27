#include <app/step/100_TmpCFGeneration.h>

//BOOST
#include <boost/timer.hpp>
#include <boost/progress.hpp>

//SOCLE
#include <ign/geometry/algorithm/BufferOpGeos.h>

//EPG
#include <epg/Context.h>
#include <epg/io/query/CountryQueriesReader.h>
#include <epg/io/query/utils/queryUtils.h>
#include <epg/tools/TimeTools.h>
#include <epg/utils/CopyTableUtils.h>
#include <epg/utils/replaceTableName.h>
#include <epg/tools/geometry/LineStringSplitter.h>
#include <epg/tools/geometry/interpolate.h>
#include <epg/tools/geometry/project.h>
#include <epg/tools/geometry/getSubLineString.h>

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
		std::string const boundaryTableName = epg::utils::replaceTableName(_epgParams.getValue( TARGET_BOUNDARY_TABLE ).toString());
		//copie de boundary dans public
		/*epg::utils::CopyTableUtils::copyTable(
			_epgParams.getValue(TARGET_BOUNDARY_TABLE).toString(),
			idName,
			geomName,
			ign::geometry::Geometry::GeometryTypeLineString,
			boundaryTableName,
			"",
			false,
			true
		);*/

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
			<< geomName << " type geometry(LineString, 0);"
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
			boost::progress_display display(numFeatures, std::cout, "[ CREATE CONNECTING POINTS ]\n");

			while (itFeaturesToMatch->hasNext())
			{

				++display;

				// TO-DO : creer point d'intersection
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
						/*else if (geomCollect.geometryN(i).isLineString())
						{
							fCF.setGeometry(geomCollect.geometryN(i).asLineString());
							fsTmpCL->createFeature(fCF, idGeneratorCL->next());
							continue;
						}*/
						// Autre cas ?? a loguer

					}
				}
			}


			//CL
			double distBuffer = 5;
			double thresholdNoCL = 10;
			double ratioInBuff = 0.6;
			double snapOnVertexBorder = 2;

			ign::geometry::algorithm::BufferOpGeos buffOp;
			ign::geometry::GeometryPtr buffBorder(buffOp.buffer(lsBoundary, distBuffer, 0, ign::geometry::algorithm::BufferOpGeos::CAP_FLAT));
			
			getCLfromBorder(fsTmpCL, idGeneratorCL, lsBoundary, buffBorder, distBuffer, thresholdNoCL, ratioInBuff, snapOnVertexBorder);
		}

		_epgLogger.log( epg::log::TITLE, "[ END TMP CF GENERATION ] : " + epg::tools::TimeTools::getTime() );

	}




}
}



void app::step::TmpCFGeneration::getCLfromBorder(
	ign::feature::sql::FeatureStorePostgis* fsTmpCL,
	epg::sql::tools::IdGeneratorInterfacePtr idGeneratorCL,
	ign::geometry::LineString & lsBorder,
	ign::geometry::GeometryPtr& buffBorder,
	double distBuffer,
	double thresholdNoCL,
	double ratioInBuff,
	double snapOnVertexBorder
)
{

	epg::Context* context = epg::ContextS::getInstance();

	ign::feature::FeatureFilter filter;
	filter.setExtent(lsBorder.getEnvelope().expandBy(distBuffer * 2));

	ign::feature::FeatureIteratorPtr eit = context->getFeatureStore(epg::EDGE)->getFeatures(filter);
	int numFeatures = context->getDataBaseManager().numFeatures(*context->getFeatureStore(epg::EDGE), filter);
	boost::progress_display display(numFeatures, std::cout, "[ CREATE CONNECTING LINES ]\n");

	while (eit->hasNext())
	{
		++display;
		//
		ign::feature::Feature fEdge = eit->next();
		ign::geometry::LineString const & lsEdge = fEdge.getGeometry().asLineString();

		//ign::geometry::Geometry* geomIntersect = lsEdge.Intersection(*buff);
		std::vector<ign::geometry::LineString> vLsToProjectOnBorder;

		epg::tools::geometry::LineStringSplitter lsSplitter(lsEdge);
		lsSplitter.addCuttingGeometry(*buffBorder);
		std::vector<ign::geometry::LineString> subEdgesBorder = lsSplitter.getSubLineStrings();

		/*if (subEdgesBorder.size() == 0) { //ne devrait pas arriver, verifier si pas d'intersection que le sub contient juste l'edge
			bool test = true;
		}//*/


		//pas d'intersection par le buffer
		if (subEdgesBorder.size() == 1) {
			//si l'edge est "proche" on consid�re qu'il est enti�rement dans le buffer et longe la fronti�re
			if (lsEdge.distance(lsBorder) < distBuffer) {
				//TODO//verifier que l'edge ne soit pas un edge court, inclus entierement dans le buffer avec un angle perpendiculaire qui arrive et non long le frontiere ->ne pas prendre en compte				
				vLsToProjectOnBorder.push_back(lsEdge);	
			}
			continue;
		}

		int numfirstSubInBuff = -1;
		int numlastSubInBuff = -1;
		int lengthInBuff = 0;
		int lengthNearByBuff = 0;

		for (size_t i = 0; i < subEdgesBorder.size(); ++i) {
			ign::geometry::LineString lsSubEdgeCurr = subEdgesBorder[i];

			int numSeg = static_cast<int>(std::floor(lsSubEdgeCurr.numSegments() / 2.));
			ign::geometry::Point interiorPointSEC = epg::tools::geometry::interpolate(lsSubEdgeCurr, numSeg, 0.5);
			bool isSubSegInBuff = false;
			if (buffBorder->contains(interiorPointSEC)) {
				isSubSegInBuff = true;
				numlastSubInBuff = i;

				lengthInBuff += lsSubEdgeCurr.length();
				if (numfirstSubInBuff < 0)
					numfirstSubInBuff = i;
			}

			if (isSubSegInBuff || lsSubEdgeCurr.length() <= thresholdNoCL)
				lengthNearByBuff += lsSubEdgeCurr.length();

			if ((lsSubEdgeCurr.length() > thresholdNoCL && !isSubSegInBuff) || i == subEdgesBorder.size() - 1) {
				if (lengthInBuff > ratioInBuff * lengthNearByBuff) {
					//recup ptStart, ptFin et proj des pt sur la border
					std::pair< int, double > pairPtCurvStartCL = epg::tools::geometry::projectAlong(lsBorder, subEdgesBorder[numfirstSubInBuff].startPoint(), snapOnVertexBorder);
					std::pair< int, double > pairPtCurvEndCL = epg::tools::geometry::projectAlong(lsBorder, subEdgesBorder[numlastSubInBuff].endPoint(), snapOnVertexBorder);
					//recup de la border entre ces points pour recup de la geom CL
					ign::geometry::LineString lsCL = epg::tools::geometry::getSubLineString(pairPtCurvStartCL, pairPtCurvEndCL, lsBorder, snapOnVertexBorder);
					vLsToProjectOnBorder.push_back(lsCL);
				}

				//reset
				numfirstSubInBuff = -1;
				numlastSubInBuff = -1;
				lengthInBuff = 0;
				lengthNearByBuff = 0;

			}

		}

		for (size_t i = 0; i < vLsToProjectOnBorder.size(); ++i) {
			//create CL
			//generation de l'id CL
			//creation du feat en copie du featEdge puis modif de la geom et de l'id
			ign::feature::Feature featCL = fEdge;
			featCL.setGeometry(vLsToProjectOnBorder[i]);
			std::string idCL = idGeneratorCL->next();
			featCL.setId(idCL);
			if (idCL == "CONNECTINGLINE4") return;
			fsTmpCL->createFeature(featCL, idCL);

		}
	}
					

}


