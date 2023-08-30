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
#include <epg/tools/FilterTools.h>
#include <epg/tools/StringTools.h>
#include <epg/utils/CopyTableUtils.h>
#include <epg/utils/replaceTableName.h>
#include <epg/tools/geometry/LineStringSplitter.h>
#include <epg/tools/geometry/interpolate.h>
#include <epg/tools/geometry/project.h>
#include <epg/tools/geometry/getSubLineString.h>
#include <epg/tools/geometry/angle.h>
#include <epg/tools/geometry/SegmentIndexedGeometry.h>

//APP
#include <app/tools/CountryQueryErmTrans.h>

//////
#include <ign/math/Line2T.h>
#include <ign/math/LineT.h>
#include <epg/tools/geometry/LineIntersector.h>


namespace app {
namespace step {

	///
	///
	///
	void TmpCFGeneration::init()
	{
		addWorkingEntity(SOURCE_ROAD_TABLE);
		addWorkingEntity(CP_TABLE);
		addWorkingEntity(CL_TABLE);
	}


	///
	///
	///
	void TmpCFGeneration::onCompute( bool verbose )
	{
		//a mettre en param
		std::string codeCountry1 = "be";
		std::string codeCountry2 = "fr";

		epg::Context* context = epg::ContextS::getInstance();

		epg::log::ShapeLogger* shapeLogger = epg::log::ShapeLoggerS::getInstance();
		shapeLogger->addShape("getCLfromBorder_buffers", epg::log::ShapeLogger::POLYGON);
		shapeLogger->addShape("CLNotMerge", epg::log::ShapeLogger::LINESTRING);
		shapeLogger->addShape("CLno2country", epg::log::ShapeLogger::LINESTRING);

		shapeLogger->addShape("CPUndershoot", epg::log::ShapeLogger::POINT);


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

		std::string listAttr2concatName = _themeParams.getValue(LIST_ATTR_TO_CONCAT).toString();
		//on recup les attribut a concat et on les mets dans un vecteur en splitant
		std::vector<std::string> _vAttrNameToConcat;
		epg::tools::StringTools::Split(listAttr2concatName, "/", _vAttrNameToConcat);
		for (size_t i = 0; i < _vAttrNameToConcat.size(); ++i) {
			_sAttrNameToConcat.insert(_vAttrNameToConcat[i]);
		}

		//mettre dans un set?


		std::string edgeSourceTableName = _themeParams.getValue(SOURCE_ROAD_TABLE).toString();
		std::string edgeTargetTableName = getCurrentWorkingTableName(SOURCE_ROAD_TABLE);
		_epgParams.setParameter(EDGE_TABLE, ign::data::String(edgeTargetTableName));

		epg::utils::CopyTableUtils::copyEdgeTable(edgeSourceTableName, "", false);

		ign::feature::sql::FeatureStorePostgis* fsEdge = _context.getFeatureStore(epg::EDGE);


		// Create tmp_cf table
		std::string tmpCpTableName = getCurrentWorkingTableName(CP_TABLE);

		std::ostringstream ss;
		ss << "DROP TABLE IF EXISTS " << tmpCpTableName << " ;"
			<< "CREATE TABLE " << tmpCpTableName
			<< " AS TABLE " << edgeTargetTableName
			<< " WITH NO DATA;"
			<< "ALTER TABLE " << tmpCpTableName << " ALTER COLUMN " 
			<< geomName << " type geometry(PointZ, 0);"
			<< "ALTER TABLE " << tmpCpTableName << " ALTER COLUMN "
			<< idName << " type varchar(255);"
		    << "ALTER TABLE " << tmpCpTableName << " ALTER COLUMN "
			<< countryCodeName << " type varchar(255);"
			<< "ALTER TABLE " << tmpCpTableName << " ALTER COLUMN "
			<< "w_national_identifier" << " type varchar(25555);";

		_context.getDataBaseManager().getConnection()->update(ss.str());

		// Create tmp_cf table
		std::string tmpClTableName = getCurrentWorkingTableName(CL_TABLE);

		ss.clear();
		ss << "DROP TABLE IF EXISTS " << tmpClTableName << " ;"
			<< "CREATE TABLE " << tmpClTableName
			<< " AS TABLE " << edgeTargetTableName
			<< " WITH NO DATA;"
			<< "ALTER TABLE " << tmpClTableName << " ALTER COLUMN "
			<< geomName << " type geometry(LineString, 0);"
			<< "ALTER TABLE " << tmpClTableName << " ALTER COLUMN "
			<< idName << " type varchar(255);"
			<< "ALTER TABLE " << tmpClTableName << " ALTER COLUMN "
			<< countryCodeName << " type varchar(255);"
			<< "ALTER TABLE " << tmpClTableName << " ALTER COLUMN "
			<< "w_national_identifier" << " type varchar(25555);";
		_context.getDataBaseManager().getConnection()->update(ss.str());

		// Go through objects intersecting the boundary
		_fsBoundary = _context.getDataBaseManager().getFeatureStore(boundaryTableName, idName, geomName);
		
		_fsTmpCP = _context.getDataBaseManager().getFeatureStore(tmpCpTableName, idName, geomName);
		_fsTmpCL = _context.getDataBaseManager().getFeatureStore(tmpClTableName, idName, geomName);

		// id generator
		_idGeneratorCP = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsTmpCP, "CONNECTINGPOINT"));
		_idGeneratorCL = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsTmpCL, "CONNECTINGLINE"));

		//CL
		double distBuffer = 5;
		double thresholdNoCL = 10;
		double ratioInBuff = 0.6;
		double snapOnVertexBorder = 2;
		double angleMaxBorder = 35;
		angleMaxBorder = angleMaxBorder * M_PI / 180;

		ign::feature::FeatureIteratorPtr itBoundary = _fsBoundary->getFeatures(ign::feature::FeatureFilter(countryCodeName + " = '"+ codeCountry1+"#"+ codeCountry2+"'"));
		while (itBoundary->hasNext())
		{
			ign::feature::Feature fBoundary = itBoundary->next();
			ign::geometry::LineString lsBoundary = fBoundary.getGeometry().asLineString();

			ign::geometry::algorithm::BufferOpGeos buffOp;
			ign::geometry::GeometryPtr buffBorder(buffOp.buffer(lsBoundary, distBuffer, 0, ign::geometry::algorithm::BufferOpGeos::CAP_FLAT));

			ign::feature::Feature fBuff;
			fBuff.setGeometry(buffBorder->clone());
			shapeLogger->writeFeature("getCLfromBorder_buffers", fBuff);

			getCLfromBorder(lsBoundary, buffBorder, distBuffer, thresholdNoCL, angleMaxBorder, ratioInBuff, snapOnVertexBorder);


		}

			
		double distMergeCL = 1;
		mergeCL(distMergeCL, snapOnVertexBorder);

		ign::feature::FeatureIteratorPtr itBoundary2 = _fsBoundary->getFeatures(ign::feature::FeatureFilter(countryCodeName + " = '" + codeCountry1 + "#" + codeCountry2 + "'"));
		while (itBoundary2->hasNext()) {
			ign::feature::Feature fBoundary = itBoundary2->next();
			ign::geometry::LineString lsBoundary = fBoundary.getGeometry().asLineString();
			ign::geometry::algorithm::BufferOpGeos buffOp;
			ign::geometry::GeometryPtr buffBorder(buffOp.buffer(lsBoundary, distBuffer, 0, ign::geometry::algorithm::BufferOpGeos::CAP_FLAT));

			getCPfromIntersectBorder(lsBoundary);

			addToUndershootNearBorder(lsBoundary, buffBorder, distBuffer);
		}

		double distMergeCP = 5;
		mergeCPNearBy(distMergeCP, 0);
	
		shapeLogger->closeShape("getCLfromBorder_buffers");
		shapeLogger->closeShape("CLNotMerge"); 
		shapeLogger->closeShape("CLno2country"); 
		shapeLogger->closeShape("CPUndershoot");//
		_epgLogger.log( epg::log::TITLE, "[ END TMP CF GENERATION ] : " + epg::tools::TimeTools::getTime() );

	}

}
}


void app::step::TmpCFGeneration::getCLfromBorder(
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
			std::string idCL = _idGeneratorCL->next();
			featCL.setId(idCL);
			_fsTmpCL->createFeature(featCL, idCL);
		}
	}	
}


void app::step::TmpCFGeneration::addToUndershootNearBorder(
	ign::geometry::LineString & lsBorder,
	ign::geometry::GeometryPtr& buffBorder,
	double distBuffer
)
{
	epg::log::ShapeLogger* shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	std::string const geomName = _epgParams.getValue(GEOM).toString();

	ign::feature::FeatureFilter filterBuffBorder("ST_INTERSECTS(" + geomName + ", '" + buffBorder->toString() + "')");
	ign::feature::FeatureIteratorPtr eit = context->getFeatureStore(epg::EDGE)->getFeatures(filterBuffBorder);
	int numFeatures = context->getDataBaseManager().numFeatures(*context->getFeatureStore(epg::EDGE), filterBuffBorder);
	boost::progress_display display(numFeatures, std::cout, "[ GET CONNECTING POINTS BY UNDERSHOOT LINES ]\n");

	//recuperation des troncons qui intersect le buff de 5m
	while (eit->hasNext())
	{
		++display;
		ign::feature::Feature fEdge = eit->next();
		ign::geometry::LineString lsEdge = fEdge.getGeometry().asLineString();
		//si intersection border -> on fait rien
		if (lsBorder.intersects(lsEdge))
			//if (!lsBorder.Intersection(lsEdge)->isNull())
			//est ce que intersects plus long que intersection?
			continue;

		double distBorder2StartPt = lsBorder.distance(lsEdge.startPoint());
		double distBorder2EndPt = lsBorder.distance(lsEdge.endPoint());
		ign::geometry::Point ptClosestBorder;
		ign::math::Vec2d vecToBorder;
		if (distBorder2StartPt < distBorder2EndPt) {
			ptClosestBorder = lsEdge.startPoint();
		}
		else {
			ptClosestBorder = lsEdge.endPoint();
			lsEdge.reverse();
		}
		vecToBorder.x() = lsEdge.pointN(1).x() - ptClosestBorder.x();
		vecToBorder.y() = lsEdge.pointN(1).y() - ptClosestBorder.y();
		
		//on verifie que le point est un dangle, sinon on fait rien
		ign::feature::FeatureFilter filterArroundPt;
		filterArroundPt.setExtent(ptClosestBorder.getEnvelope().expandBy(1));
		ign::feature::FeatureIteratorPtr eitArroundPt = context->getFeatureStore(epg::EDGE)->getFeatures(filterArroundPt);
		bool isPtADangle = true;
		while (eitArroundPt->hasNext()) {
			ign::feature::Feature featArroundPt = eitArroundPt->next();
			if (featArroundPt.getId() == fEdge.getId())
				continue;
			double dist = featArroundPt.getGeometry().distance(ptClosestBorder);
			if (dist > 0)
				continue;
			isPtADangle = false;
			break;
		}
		if (!isPtADangle)
			continue;

		ign::geometry::Point projPt;
		std::vector< ign::geometry::Point > vPtIntersect;
		epg::tools::geometry::SegmentIndexedGeometry segIndexLsBorder(&lsBorder);
		////////////////////todo recup le bon segment de la frontiere
		epg::tools::geometry::LineIntersector::compute(ptClosestBorder, lsEdge.pointN(1), lsBorder, vPtIntersect);
		double distMin = 100000;
		for (std::vector< ign::geometry::Point >::iterator vit = vPtIntersect.begin(); vit != vPtIntersect.end(); ++vit) {
			double dist = ptClosestBorder.distance(*vit);
			if (dist < distMin) {
				projPt = *vit;
				distMin = dist;
			}
		}

		if (ptClosestBorder.distance(projPt) > 2 * distBuffer)
			continue;
		if (isEdgeIntersectedPtWithCL(fEdge, projPt))
			continue;

		projPt.setZ(0);
		ign::feature::Feature fCF = fEdge;
		fCF.setGeometry(projPt);
		std::string idCP = _idGeneratorCP->next();
		//fCF.setAttribute("w_step", ign::data::String("1"));
		_fsTmpCP->createFeature(fCF, idCP);
		{
			ign::feature::Feature fShaplog = fCF;
			ign::geometry::Point ptProjNoZ = projPt;
			ptProjNoZ.clearZ();
			fShaplog.setGeometry(ptProjNoZ);
			shapeLogger->writeFeature("CPUndershoot", fShaplog);
		}
	}
}


void app::step::TmpCFGeneration::getCPfromIntersectBorder(
	ign::geometry::LineString & lsBorder
)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const geomName = _epgParams.getValue(GEOM).toString();

	ign::feature::FeatureFilter filterFeaturesToMatch("ST_INTERSECTS(" + geomName + ", '" + lsBorder.toString() + "')");
	ign::feature::FeatureIteratorPtr itFeaturesToMatch = context->getFeatureStore(epg::EDGE)->getFeatures(filterFeaturesToMatch);

	int numFeatures = context->getDataBaseManager().numFeatures(*context->getFeatureStore(epg::EDGE), filterFeaturesToMatch);
	boost::progress_display display(numFeatures, std::cout, "[ CREATE CONNECTING POINTS ]\n");

	while (itFeaturesToMatch->hasNext())
	{
		++display;

		ign::feature::Feature fToMatch = itFeaturesToMatch->next();
		ign::geometry::LineString const& lsFToMatch = fToMatch.getGeometry().asLineString();
		ign::geometry::Geometry* geomPtr = lsFToMatch.Intersection(lsBorder);

		if (geomPtr->isNull())
			continue;

		ign::feature::Feature fCF = fToMatch;

		if (geomPtr->isPoint())
		{
			//si l'edge sert à une CL et, ne pas créer de CP?
			bool isConnectedToCL = isEdgeIntersectedPtWithCL(fToMatch, geomPtr->asPoint());
			if (!isConnectedToCL) {
				fCF.setGeometry(geomPtr->asPoint());
				std::string idCP = _idGeneratorCP->next();
				_fsTmpCP->createFeature(fCF, idCP);
			}

		}

		else if (geomPtr->isGeometryCollection())
		{
			ign::geometry::GeometryCollection geomCollect = geomPtr->asGeometryCollection();
			for (size_t i = 0; i < geomCollect.numGeometries(); ++i)
			{
				if (geomCollect.geometryN(i).isPoint())
				{
					ign::geometry::Point ptIntersect = geomCollect.geometryN(i).asPoint();
					
					bool isConnectedToCL = isEdgeIntersectedPtWithCL(fToMatch, ptIntersect);
					if (!isConnectedToCL) {
						fCF.setGeometry(ptIntersect);
						std::string idCP = _idGeneratorCP->next();
						_fsTmpCP->createFeature(fCF, idCP);
					}
				}
			}
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




bool app::step::TmpCFGeneration::isEdgeIntersectedPtWithCL(
	ign::feature::Feature& fEdge,
	ign::geometry::Point ptIntersectBorder
)
{
	std::string idLinkedEdge = fEdge.getAttribute("w_national_identifier").toString();
	ign::feature::FeatureFilter filterIntersectCL ("w_national_identifier like '%" + idLinkedEdge +"%'");
	filterIntersectCL.setExtent(ptIntersectBorder.getEnvelope().expandBy(1));
	ign::feature::FeatureIteratorPtr itIntersectedCL = _fsTmpCL->getFeatures(filterIntersectCL);

	if (itIntersectedCL->hasNext()) {
		//if(ptIntersectBorder.distance(itIntersectedCL->next().getGeometry()) < 0.1) //==0 ? ou rien, car proche de 1 on suppose que c'est de la CL l'intersection
		return true;
	}

	return false;
}



void app::step::TmpCFGeneration::mergeCPNearBy(
	double distMergeCP,
	double snapOnVertexBorder
)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string countryCodeName = _epgParams.getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCP;
	ign::feature::FeatureIteratorPtr itCP = _fsTmpCP->getFeatures(filterCP);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsTmpCP, filterCP);
	boost::progress_display display(numFeatures, std::cout, "[ FUSION CONNECTING POINTS WITH #]\n");

	std::set<std::string> sCP2Merge;
	std::string separator = "#";

	while (itCP->hasNext())
	{
		++display;

		ign::feature::Feature fCPCurr = itCP->next();

		std::string idCP = fCPCurr.getId();

		if (sCP2Merge.find(idCP) != sCP2Merge.end())
			continue;

		std::map<std::string, ign::feature::Feature> mCPNear;
		bool hasNearestCP = getNearestCP(fCPCurr, distMergeCP, mCPNear);
		if (hasNearestCP) {
			ign::feature::Feature fCPNew = _fsTmpCP->newFeature();
			std::string idCPNew = _idGeneratorCP->next();
			
			ign::geometry::MultiPoint multiPtCP;

			for (std::map<std::string, ign::feature::Feature>::iterator mit = mCPNear.begin(); mit != mCPNear.end(); ++mit) {
				sCP2Merge.insert(mit->first);
				multiPtCP.addGeometry(mit->second.getGeometry());
				
				if (mit == mCPNear.begin())
					fCPNew = mit->second;
				else
					addFeatAttributeMergingOnBorder(fCPNew, mit->second, separator);
			}
			fCPNew.setId(idCPNew);
			//geom
			ign::geometry::Point ptCentroidCP = multiPtCP.asMultiPoint().getCentroid();
			ign::feature::FeatureFilter filterBorderNearCP;// (countryCodeName + " = 'be#fr'");
			filterBorderNearCP.setExtent(ptCentroidCP.getEnvelope().expandBy(distMergeCP));
			ign::geometry::LineString lsBorderClosest;
			double distMinBorder = 2*distMergeCP;
			ign::feature::FeatureIteratorPtr fitBorder = _fsBoundary->getFeatures(filterBorderNearCP);
			while (fitBorder->hasNext()) {
				ign::geometry::LineString lsBorder = fitBorder->next().getGeometry().asLineString();
				double dist = lsBorder.distance(ptCentroidCP);
				if (dist < distMinBorder) {
					distMinBorder = dist;
					lsBorderClosest = lsBorder;
				}
			}
			ign::geometry::Point ptCentroidOnBorderCP = epg::tools::geometry::project(lsBorderClosest, ptCentroidCP, snapOnVertexBorder);
			ptCentroidOnBorderCP.setZ(0);
			fCPNew.setGeometry(ptCentroidOnBorderCP);

			_fsTmpCP->createFeature(fCPNew, idCPNew);
		}			
	}

	for (std::set<std::string>::iterator sit = sCP2Merge.begin(); sit != sCP2Merge.end(); ++sit) {
		_fsTmpCP->deleteFeature(*sit);
	}
}

bool app::step::TmpCFGeneration::getNearestCP(
	ign::feature::Feature fCP,
	double distMergeCP,
	std::map<std::string,ign::feature::Feature>& mCPNear
)
{
	mCPNear[fCP.getId()] = fCP;
	
	std::string const idName = _epgParams.getValue(ID).toString();
	ign::feature::FeatureFilter filterArroundCP;
	for (std::map<std::string, ign::feature::Feature>::iterator mit = mCPNear.begin(); mit != mCPNear.end(); ++mit) {
		epg::tools::FilterTools::addAndConditions(filterArroundCP, idName + " <> '" + mit->first + "'");	//(idName + " <> '" + fCP.getId() + "'");
	}
	filterArroundCP.setExtent(fCP.getGeometry().getEnvelope().expandBy(distMergeCP));
	ign::feature::FeatureIteratorPtr itArroundCP = _fsTmpCP->getFeatures(filterArroundCP);
	if (!itArroundCP->hasNext())
		return false;
	while (itArroundCP->hasNext())
	{
		ign::feature::Feature fCPArround = itArroundCP->next();
		getNearestCP(fCPArround, distMergeCP, mCPNear);
		mCPNear[fCPArround.getId()] = fCPArround;
	}
	return true;
}

void app::step::TmpCFGeneration::addFeatAttributeMergingOnBorder(
	ign::feature::Feature& featMerged,
	ign::feature::Feature& featAttrToAdd,
	std::string separator
)
{
	ign::feature::FeatureType featTypMerged = featMerged.getFeatureType();
	//ign::feature::FeatureType featTyp2 = feat2.getFeatureType();
	//test si featTyp1 == featTyp2 sinon msg d'erreur
	std::vector<std::string> vAttrNames;
	featTypMerged.getAttributeNames(vAttrNames);
	std::string attrValueMerged;

	for (size_t i = 0; i < vAttrNames.size(); ++i) {
		std::string attrName = vAttrNames[i];
		//std::string typeName = featMerged.getAttribute(attrName).getTypeName();
		//faire une liste des attr de travail?
		if (attrName == featTypMerged.getIdName() || attrName == featTypMerged.getDefaultGeometryName()
			 || attrName == "begin_lifespan_version" || attrName == "end_lifespan_version"
			|| attrName == "valid_from" || attrName == "valid_to"
			|| attrName == "w_step" || attrName == "net_type"
			)//attribut en timestamp qui sont pour l'historisation
			continue;

		std::string attrValueToMerge = featMerged.getAttribute(attrName).toString();
		std::string attrValueToAdd = featAttrToAdd.getAttribute(attrName).toString();
		if (_sAttrNameToConcat.find(attrName) != _sAttrNameToConcat.end()) {
			//fichier param qui précise le type où on concat avec un  # lister les attributs (country, et num route eur et national)
			//si même valeur on garde une fois sinon # dans l'ordre des countrycode

			//TODO: organiser par ordre de country
		if (attrValueToMerge == "null")
			attrValueMerged = attrValueToAdd;
		else if (attrValueToAdd == "null")
			attrValueMerged = attrValueToMerge;
		else	
			attrValueMerged = attrValueToMerge + separator + attrValueToAdd;
		}
		else {//si même valeur on garde, sinon on laisse vide "" dans le cas des enums
			if (attrValueToMerge == attrValueToAdd)
				attrValueMerged = attrValueToMerge;
			else if (attrValueToMerge == "null")
				attrValueMerged = attrValueToAdd;
			else if (attrValueToAdd == "null")
				attrValueMerged = attrValueToMerge;
			else
				continue;// attrValueMerged = "";
		}
		
		featMerged.setAttribute(attrName,ign::data::String(attrValueMerged));

	}
}


void app::step::TmpCFGeneration::mergeCL(
	double distMergeCL,
	double snapOnVertexBorder
)
{
	epg::log::ShapeLogger* shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	std::string countryCodeName = _epgParams.getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCL;
	ign::feature::FeatureIteratorPtr itCL = _fsTmpCL->getFeatures(filterCL);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsTmpCL, filterCL);
	boost::progress_display display(numFeatures, std::cout, "[ FUSION CONNECTING LINES WITH #]\n");

	std::set<std::string> sCL2delete;
	std::string separator = "#";

	while (itCL->hasNext())
	{
		++display;
		ign::feature::Feature fCLCurr = itCL->next();

		std::string idCL = fCLCurr.getId();
		ign::geometry::LineString lsCurr = fCLCurr.getGeometry().asLineString();

		if (sCL2delete.find(idCL) != sCL2delete.end())
			continue;

		std::map<std::string, ign::feature::Feature> mCL2merge;
		std::set<std::string> sCountryCode;
		bool hasNearestCL = getCLToMerge(fCLCurr, distMergeCL, mCL2merge, sCountryCode);

		if (!hasNearestCL) {
			sCL2delete.insert(idCL);
			shapeLogger->writeFeature("CLNotMerge", fCLCurr);
			continue;
		}

		if (sCountryCode.size() < 2) {
			for (std::map<std::string, ign::feature::Feature>::iterator mit = mCL2merge.begin(); mit != mCL2merge.end(); ++mit) {
				sCL2delete.insert(mit->first);
				shapeLogger->writeFeature("CLno2country", mit->second);
			}
			continue;
		}

		ign::feature::Feature fCLNew = _fsTmpCL->newFeature();
		std::string idCLNew = _idGeneratorCL->next();

		ign::geometry::LineString lsSE,lsSS,lsES,lsEE,lsTempCL;

		ign::geometry::LineString lsBorder;
		getBorderFromEdge(lsCurr, lsBorder);
		
		for (std::map<std::string, ign::feature::Feature>::iterator mit = mCL2merge.begin(); mit != mCL2merge.end(); ++mit) {
			sCL2delete.insert(mit->first);
			if (mit == mCL2merge.begin()) {
				lsTempCL = mit->second.getGeometry().asLineString();
				fCLNew = mit->second;
			}
			else {
				addFeatAttributeMergingOnBorder(fCLNew, mit->second, separator);
				ign::geometry::LineString lsMit = mit->second.getGeometry().asLineString();
				getGeomCL(lsSE, lsBorder, lsTempCL.startPoint(), lsMit.endPoint(), snapOnVertexBorder);
				getGeomCL(lsSS, lsBorder, lsTempCL.startPoint(), lsMit.startPoint(), snapOnVertexBorder);
				getGeomCL(lsES, lsBorder, lsTempCL.endPoint(), lsMit.startPoint(), snapOnVertexBorder);
				getGeomCL(lsEE, lsBorder, lsTempCL.endPoint(), lsMit.endPoint(), snapOnVertexBorder);
				double lengthMax= lsTempCL.length();
				if (lsSE.length() > lengthMax) {
					lsTempCL = lsSE;
					lengthMax = lsTempCL.length();
				}
				if (lsSS.length() > lengthMax) {
					lsTempCL = lsSS;
					lengthMax = lsTempCL.length();
				}
				if (lsES.length() > lengthMax) {
					lsTempCL = lsES;
					lengthMax = lsTempCL.length();
				}
				if (lsEE.length() > lengthMax) {
					lsTempCL = lsEE;
					lengthMax = lsTempCL.length();
				}
				if (lsMit.length() > lengthMax) {
					lsTempCL = lsMit;
					lengthMax = lsTempCL.length();
				}
			}
		}
		fCLNew.setGeometry(lsTempCL);
		_fsTmpCL->createFeature(fCLNew, idCLNew);
	}

	for (std::set<std::string>::iterator sit = sCL2delete.begin(); sit != sCL2delete.end(); ++sit) {
		_fsTmpCL->deleteFeature(*sit);
	}
}



bool app::step::TmpCFGeneration::getCLToMerge(
	ign::feature::Feature fCL,
	double distMergeCL,
	std::map < std::string, ign::feature::Feature>& mCL2merge,
	std::set<std::string>& sCountryCode
)
{
	std::string const idName = _epgParams.getValue(ID).toString();
	std::string const countryCodeName = _epgParams.getValue(COUNTRY_CODE).toString();

	mCL2merge[fCL.getId()] = fCL;
	sCountryCode.insert(fCL.getAttribute(countryCodeName).toString());

	ign::feature::FeatureFilter filterArroundCL;
	for (std::map<std::string, ign::feature::Feature>::iterator mit = mCL2merge.begin(); mit != mCL2merge.end(); ++mit) {
		epg::tools::FilterTools::addAndConditions(filterArroundCL, idName + " <> '" + mit->first + "'");
	}
	filterArroundCL.setExtent(fCL.getGeometry().getEnvelope());
	ign::feature::FeatureIteratorPtr itArroundCL = _fsTmpCL->getFeatures(filterArroundCL);
	if (!itArroundCL->hasNext())
		return false;
	while (itArroundCL->hasNext())
	{
		ign::feature::Feature fCLArround = itArroundCL->next();
		if (fCL.getGeometry().distance(fCLArround.getGeometry()) > distMergeCL)
			continue;
		std::string countryCodeCL = fCLArround.getAttribute(countryCodeName).toString();
		sCountryCode.insert(countryCodeCL);
		getCLToMerge(fCLArround, distMergeCL, mCL2merge, sCountryCode);
		mCL2merge[fCLArround.getId()] = fCLArround;
	}
	return true;
}


void app::step::TmpCFGeneration::getBorderFromEdge(
	ign::geometry::LineString& lsEdgeOnBorder,
	ign::geometry::LineString& lsBorder
)
{
	ign::feature::FeatureFilter filter;//(countryCodeName + " = 'be#fr'")
	filter.setExtent(lsEdgeOnBorder.getEnvelope());
	ign::feature::FeatureIteratorPtr itBoundary = _fsBoundary->getFeatures(filter);
	while (itBoundary->hasNext()) {
		ign::feature::Feature fBorder = itBoundary->next();
		ign::geometry::LineString lsBorderTemp = fBorder.getGeometry().asLineString();
		double distBorder = lsEdgeOnBorder.distance(lsBorderTemp);
		if (distBorder == 0) {
			lsBorder = lsBorderTemp;
			return;
		}
			
	}

}