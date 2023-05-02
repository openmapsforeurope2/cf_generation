#include <app/step/100_TmpCFGeneration.h>

//BOOST
#include <boost/timer.hpp>
#include <boost/progress.hpp>

//SOCLE
#include <ign/geometry/algorithm/BufferOpGeos.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ShapeLogger.h>
#include <epg/io/query/CountryQueriesReader.h>
#include <epg/io/query/utils/queryUtils.h>
#include <epg/tools/TimeTools.h>
#include <epg/utils/CopyTableUtils.h>
#include <epg/utils/replaceTableName.h>
#include <epg/tools/geometry/LineStringSplitter.h>
#include <epg/tools/geometry/interpolate.h>
#include <epg/tools/geometry/project.h>
#include <epg/tools/geometry/getSubLineString.h>
#include <epg/tools/geometry/angle.h>

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

		epg::log::ShapeLogger* shapeLogger = epg::log::ShapeLoggerS::getInstance();
		shapeLogger->addShape("getCLfromBorder_buffers", epg::log::ShapeLogger::POLYGON);


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
			double angleMaxBorder = 35;
			angleMaxBorder = angleMaxBorder * M_PI / 180;

			ign::geometry::algorithm::BufferOpGeos buffOp;
			ign::geometry::GeometryPtr buffBorder(buffOp.buffer(lsBoundary, distBuffer, 0, ign::geometry::algorithm::BufferOpGeos::CAP_FLAT));
			
			ign::feature::Feature fBuff;
			fBuff.setGeometry(buffBorder->clone());
			shapeLogger->writeFeature("getCLfromBorder_buffers", fBuff);
			getCLfromBorder(fsTmpCL, idGeneratorCL, lsBoundary, buffBorder, distBuffer, thresholdNoCL, angleMaxBorder, ratioInBuff, snapOnVertexBorder);
		}

		shapeLogger->closeShape("getCLfromBorder_buffers");
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
	double angleMax,
	double ratioInBuff,
	double snapOnVertexBorder
)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const geomName = _epgParams.getValue(GEOM).toString();

	ign::feature::FeatureFilter filter("ST_INTERSECTS(" + geomName + ", '" + buffBorder->toString() + "')");

	//filter.setExtent(lsBorder.expandBy(distBuffer));

	ign::feature::FeatureIteratorPtr eit = context->getFeatureStore(epg::EDGE)->getFeatures(filter);
	int numFeatures = context->getDataBaseManager().numFeatures(*context->getFeatureStore(epg::EDGE), filter);
	boost::progress_display display(numFeatures, std::cout, "[ CREATE CONNECTING LINES ]\n");

	ign::math::Vec2d vecBorder(lsBorder.endPoint().x() - lsBorder.startPoint().x(), lsBorder.endPoint().y() - lsBorder.startPoint().y());


	while (eit->hasNext())
	{
		++display;
		//
		ign::feature::Feature fEdge = eit->next();
		ign::geometry::LineString lsEdge = fEdge.getGeometry().asLineString();

		//debug
		if (fEdge.getId() == "599f9252-a266-4484-a130-3f2e2d388620" ||
			fEdge.getId() == "69a04ff8-b7a6-4740-a2da-567e00694827" ||
			fEdge.getId() == "ad4cb433-0869-48b0-b9d9-66b30c85d528" ||
			fEdge.getId() == "52b6e92e-ab6f-44f3-925e-bce9436c7b63" ||
			fEdge.getId() == "75d794fe-dbc6-4f82-a9d5-d4f0a17d341c" ||
			fEdge.getId() == "5e8441d8-2922-4aec-b719-e41c1f6ec36b" ||
			fEdge.getId() == "9171ec83-18a1-430a-85b5-016db06e6de9"){
				std::string idStop = fEdge.getId();
		}

		//pkoi pas pris côté fr
		if (fEdge.getId() == "8c10dcc7-39d0-47ca-a116-bebac4a2cb38")
			bool bStop = true;
		else if (fEdge.getId() == "c361908f-c8c9-4d4b-9bc9-826756ef20f5")
			bool bStop = true;

		//pkoi pas de CL ici:
		if (fEdge.getId() == "dbb6d870-4781-43e8-860e-bbbbba68bb6a")
			bool bStop = true;
		else if (fEdge.getId() == "20bb78a5-0c41-4486-883a-20b6d6cd8142")
			bool bStop = true;
		
		//ign::geometry::Geometry* geomIntersect = lsEdge.Intersection(*buff);
		std::vector<ign::geometry::LineString> vLsProjectedOnBorder;

		epg::tools::geometry::LineStringSplitter lsSplitter(lsEdge);
		lsSplitter.addCuttingGeometry(*buffBorder);
		std::vector<ign::geometry::LineString> subEdgesBorder = lsSplitter.getSubLineStrings();

		//pas d'intersection par le buffer
		if (subEdgesBorder.size() == 1) {			
			double angleEdgBorder = getAngleEdgeWithBorder(lsEdge,lsBorder);		
			//si l'edge est "proche" on considere qu'il est entierement dans le buffer et longe la frontiere
			if (lsEdge.distance(lsBorder) < distBuffer && (angleEdgBorder < angleMax || angleEdgBorder > (M_PI - angleMax) ) ) {
				ign::geometry::LineString lsCL;
				getGeomCL(lsCL, lsBorder, lsEdge.startPoint(), lsEdge.endPoint(), snapOnVertexBorder);
				vLsProjectedOnBorder.push_back(lsCL);	
			}
		}
		else {

			int numfirstSubInBuff = -1;
			int numlastSubInBuff = -1;
			int lengthInBuff = 0;
			int lengthNearByBuff = 0;

			for (size_t i = 0; i < subEdgesBorder.size(); ++i) {
				ign::geometry::LineString lsSubEdgeCurr = subEdgesBorder[i];

				double angleSubEdgBorder = getAngleEdgeWithBorder(lsSubEdgeCurr, lsBorder);

				int numSeg = static_cast<int>(std::floor(lsSubEdgeCurr.numSegments() / 2.));
				ign::geometry::Point interiorPointSEC = epg::tools::geometry::interpolate(lsSubEdgeCurr, numSeg, 0.5);
				bool isSubSegInBuff = false;

				if (buffBorder->contains(interiorPointSEC) && (angleSubEdgBorder < angleMax || angleSubEdgBorder > (M_PI - angleMax) ) ) {
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
						//recup de la border entre ces points pour recup de la geom CL
						ign::geometry::LineString lsCL;
						getGeomCL(lsCL, lsBorder, subEdgesBorder[numfirstSubInBuff].startPoint(), subEdgesBorder[numlastSubInBuff].endPoint(), snapOnVertexBorder);
						if (lsCL.numPoints() >= 2 ) {
							vLsProjectedOnBorder.push_back(lsCL);
						}
					
					}

					//reset
					numfirstSubInBuff = -1;
					numlastSubInBuff = -1;
					lengthInBuff = 0;
					lengthNearByBuff = 0;
				}
			}

		}

		for (size_t i = 0; i < vLsProjectedOnBorder.size(); ++i) {
			//create CL
			//generation de l'id CL
			//creation du feat en copie du featEdge puis modif de la geom et de l'id
			ign::feature::Feature featCL = fEdge;
			featCL.setGeometry(vLsProjectedOnBorder[i]);
			std::string idCL = idGeneratorCL->next();
			featCL.setId(idCL);
			fsTmpCL->createFeature(featCL, idCL);
		}
	}	
}


double app::step::TmpCFGeneration::getAngleEdgeWithBorder(
	ign::geometry::LineString& lsEdge,
	ign::geometry::LineString& lsBorder)
{
	double angleEdgWBorder;
	ign::math::Vec2d vecEdge(lsEdge.endPoint().x() - lsEdge.startPoint().x(), lsEdge.endPoint().y() - lsEdge.startPoint().y());
	
	ign::geometry::Point ptStartProjOnBorder= epg::tools::geometry::project(lsBorder, lsEdge.startPoint(), 0);
	ign::geometry::Point ptEndProjOnBorder = epg::tools::geometry::project(lsBorder, lsEdge.endPoint(), 0);
	
	ign::math::Vec2d vecBorder(ptEndProjOnBorder.x() - ptStartProjOnBorder.x(), ptEndProjOnBorder.y() - ptStartProjOnBorder.y());

	angleEdgWBorder = epg::tools::geometry::angle(vecBorder, vecEdge);

	return angleEdgWBorder;
	//	//double angleSubEdgBorder = epg::tools::geometry::angle(vecBorder, vecSubEdge);
}


void app::step::TmpCFGeneration::getGeomCL(
	ign::geometry::LineString& lsCL,
	ign::geometry::LineString& lsBorder,
	ign::geometry::Point ptStartToProject,
	ign::geometry::Point ptEndToProject,
	double snapOnVertexBorder
)
{
	std::pair< int, double > pairPtCurvStartCL = epg::tools::geometry::projectAlong(lsBorder, ptStartToProject, snapOnVertexBorder);
	std::pair< int, double > pairPtCurvEndCL = epg::tools::geometry::projectAlong(lsBorder, ptEndToProject, snapOnVertexBorder);
	//recup de la border entre ces points pour recup de la geom CL
	lsCL = epg::tools::geometry::getSubLineString(pairPtCurvStartCL, pairPtCurvEndCL, lsBorder, snapOnVertexBorder);
}
