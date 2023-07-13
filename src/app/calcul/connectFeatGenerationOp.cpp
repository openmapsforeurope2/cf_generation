#include <app/calcul/connectFeatGenerationOp.h>

//BOOST
#include <boost/timer.hpp>
#include <boost/progress.hpp>

//SOCLE
#include <ign/geometry/algorithm/BufferOpGeos.h>
#include <ign/math/Line2T.h>
#include <ign/math/LineT.h>


//EPG
#include <epg/Context.h>
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
#include <epg/tools/geometry/LineIntersector.h>

//APP
//#include <app/tools/CountryQueryErmTrans.h>
#include <app/params/ThemeParameters.h>



namespace app {
namespace calcul {

	void connectFeatGenerationOp::compute(
		std::string countryCode,
		bool verbose)
	{
		connectFeatGenerationOp connectFeatGenerationOp(countryCode,verbose);
		connectFeatGenerationOp._compute();
	}


	connectFeatGenerationOp::connectFeatGenerationOp(std::string countryCode,
		bool verbose)
	{
		_init(countryCode, verbose);
	}

	connectFeatGenerationOp::~connectFeatGenerationOp()
	{
		_shapeLogger->closeShape("getCLfromBorder_buffers");
		_shapeLogger->closeShape("CLNotMerge");
		_shapeLogger->closeShape("CLno2country");
		_shapeLogger->closeShape("CPUndershoot");
		_shapeLogger->closeShape("Cl2mergeInMls");


		epg::log::ShapeLoggerS::kill();
	}
	
	///
	///
	///
	void connectFeatGenerationOp::_init(std::string countryCode, bool verbose)
	{
		_logger = epg::log::EpgLoggerS::getInstance();
		_logger->log(epg::log::TITLE, "[ BEGIN INITIALIZATION ] : " + epg::tools::TimeTools::getTime());


		epg::Context* context = epg::ContextS::getInstance();
		params::TransParameters* themeParameters = params::TransParametersS::getInstance();
		std::string const idName = context->getEpgParameters().getValue(ID).toString();
		std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
		std::string const natIdName = themeParameters->getValue(NATIONAL_IDENTIFIER).toString();
		std::string const natRoadCodeName = themeParameters->getValue(EDGE_NATIONAL_ROAD_CODE).toString();
		std::string const euRoadCodeName = themeParameters->getValue(EDGE_EUROPEAN_ROAD_CODE).toString();
		std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();


		_shapeLogger = epg::log::ShapeLoggerS::getInstance();
		_shapeLogger->setDataDirectory(context->getLogDirectory()+"/shapeLog");
		_shapeLogger->addShape("getCLfromBorder_buffers", epg::log::ShapeLogger::POLYGON);
		_shapeLogger->addShape("CLNotMerge", epg::log::ShapeLogger::LINESTRING);
		_shapeLogger->addShape("CLno2country", epg::log::ShapeLogger::LINESTRING);
		_shapeLogger->addShape("CPUndershoot", epg::log::ShapeLogger::POINT);
		_shapeLogger->addShape("Cl2mergeInMls", epg::log::ShapeLogger::LINESTRING);



		_countryCode = countryCode;
		_verbose = verbose;


		///recuperation de la liste des attributs à concatener dans la fusion des attributs
		std::string listAttr2concatName = themeParameters->getValue(LIST_ATTR_TO_CONCAT).toString();
		//on recup les attribut a concat et on les mets dans un vecteur en splitant
		std::vector<std::string> _vAttrNameToConcat;
		epg::tools::StringTools::Split(listAttr2concatName, "/", _vAttrNameToConcat);
		for (size_t i = 0; i < _vAttrNameToConcat.size(); ++i) {
			_sAttrNameToConcat.insert(_vAttrNameToConcat[i]);
		}


		///recuperation des features
		std::string const boundaryTableName = epg::utils::replaceTableName(context->getEpgParameters().getValue(TARGET_BOUNDARY_TABLE).toString());
		_fsBoundary = context->getDataBaseManager().getFeatureStore(boundaryTableName, idName, geomName);


		std::string edgeSourceTableName = themeParameters->getValue(SOURCE_ROAD_TABLE).toString();
		std::string edgeTargetTableName = "_110_" + epg::utils::replaceTableName(themeParameters->getValue(SOURCE_ROAD_TABLE).toString());
		context->getEpgParameters().setParameter(EDGE_TABLE, ign::data::String(edgeTargetTableName));

		epg::utils::CopyTableUtils::copyEdgeTable(edgeSourceTableName, "", false);

		_fsEdge = context->getFeatureStore(epg::EDGE);

		_reqFilterEdges2generateCF = themeParameters->getValue(SQL_FILTER_EDGES_2_GENERATE_CF).toString();
		//_filterEdges2generateCF.setPropertyConditions(reqFilterEdges2generateCF);

		///Create tmp_cp table
		std::string cpTableName = epg::utils::replaceTableName(themeParameters->getValue(TMP_CP_TABLE).toString());

		if (!context->getDataBaseManager().tableExists(cpTableName)) {
			std::ostringstream ss;
			//ss << "DROP TABLE IF EXISTS " << cpTableName << " ;"
			ss << "CREATE TABLE " << cpTableName
				<< " AS TABLE " << edgeTargetTableName
				<< " WITH NO DATA;"
				<< "ALTER TABLE " << cpTableName << " ALTER COLUMN "
				<< geomName << " type geometry(PointZ, 0);"
				<< "ALTER TABLE " << cpTableName << " ALTER COLUMN "
				<< idName << " type varchar(255);"
				<< "ALTER TABLE " << cpTableName << " ALTER COLUMN "
				<< countryCodeName << " type varchar(255);"
				<< "ALTER TABLE " << cpTableName << " ALTER COLUMN "
				<< "w_national_identifier" << " type varchar(25555);"
				<< "ALTER TABLE " << cpTableName << " ALTER COLUMN "
				<< "national_road_code" << " type varchar(255);"
				<< "ALTER TABLE " << cpTableName << " ALTER COLUMN "
				<< "european_route_number" << " type varchar(255);"
				<< "ALTER TABLE " << cpTableName << " ADD COLUMN "
				<< themeParameters->getValue(CF_STATUS).toString() << " cf_status_value default 'not_edge_matched';";

			context->getDataBaseManager().getConnection()->update(ss.str());
		}
		// Create tmp_cl table
		std::string clTableName = epg::utils::replaceTableName(themeParameters->getValue(TMP_CL_TABLE).toString());
		if (!context->getDataBaseManager().tableExists(clTableName)) {
			std::ostringstream ss;
			//ss << "DROP TABLE IF EXISTS " << clTableName << " ;"
			ss << "CREATE TABLE " << clTableName
				<< " AS TABLE " << edgeTargetTableName
				<< " WITH NO DATA;"
				<< "ALTER TABLE " << clTableName << " ALTER COLUMN "
				<< geomName << " type geometry(LineString, 0);"
				<< "ALTER TABLE " << clTableName << " ALTER COLUMN "
				<< idName << " type varchar(255);"
				<< "ALTER TABLE " << clTableName << " ALTER COLUMN "
				<< countryCodeName << " type varchar(255);"
				<< "ALTER TABLE " << clTableName << " ALTER COLUMN "
				<< "w_national_identifier" << " type varchar(25555);"
				<< "ALTER TABLE " << clTableName << " ALTER COLUMN "
				<< "national_road_code" << " type varchar(255);"
				<< "ALTER TABLE " << clTableName << " ALTER COLUMN "
				<< "european_route_number" << " type varchar(255);"
				<< "ALTER TABLE " << clTableName << " ADD COLUMN "
				<< themeParameters->getValue(CF_STATUS).toString() << " cf_status_value default 'not_edge_matched';";
			context->getDataBaseManager().getConnection()->update(ss.str());
		}

		_fsTmpCP = context->getDataBaseManager().getFeatureStore(cpTableName, idName, geomName);
		_fsTmpCL = context->getDataBaseManager().getFeatureStore(clTableName, idName, geomName);

		// id generator
		_idGeneratorCP = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsTmpCP, "CONNECTINGPOINT"));
		_idGeneratorCL = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsTmpCL, "CONNECTINGLINE"));

		_logger->log(epg::log::TITLE, "[ END INITIALIZATION ] : " + epg::tools::TimeTools::getTime());
	}



	///
	///
	///
	void connectFeatGenerationOp::getCountryCodeDoubleFromCC(std::string countryCC, std::set<std::string>& sCountryCodeDouble)
	{
		//recup des icc pour un pays


	}

	///
	///
	///
	void connectFeatGenerationOp::_compute()
	{
		_logger->log(epg::log::TITLE, "[ BEGIN CF GENERATION ] : " + epg::tools::TimeTools::getTime());
		
		std::set<std::string> sCountryCodeDouble;

		if (_countryCode.find("#") != std::string::npos)
			sCountryCodeDouble.insert(_countryCode);
		else
			getCountryCodeDoubleFromCC(_countryCode, sCountryCodeDouble);

		for(std::set<std::string>::iterator sit = sCountryCodeDouble.begin(); sit != sCountryCodeDouble.end(); ++sit)
			_computeOnDoubleCC(*sit);

	}

	///
	///
	///
	void connectFeatGenerationOp::_computeOnDoubleCC(std::string countryCodeDouble)
	{
		_logger->log( epg::log::TITLE, "[ BEGIN CF GENERATION FOR "+ countryCodeDouble +" ] : " + epg::tools::TimeTools::getTime() );

		epg::Context* context = epg::ContextS::getInstance();
		std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();


		//CL
		double distBuffer = 5;
		double thresholdNoCL = 10;
		double ratioInBuff = 0.6;
		double snapOnVertexBorder = 2;
		double angleMaxBorder = 25;
		angleMaxBorder = angleMaxBorder * M_PI / 180;

		// Go through objects intersecting the boundary
		{
			ign::feature::FeatureIteratorPtr itBoundary = _fsBoundary->getFeatures(ign::feature::FeatureFilter(countryCodeName + " = '" + countryCodeDouble + "'"));
			while (itBoundary->hasNext())
			{
				ign::feature::Feature fBoundary = itBoundary->next();

				std::string boundaryType = fBoundary.getAttribute("boundary_type").toString();

				// On ne traite que les frontières de type international_boundary ou coastline_sea_limit
				// On aurait pu filtrer en entrée mais le filtre semble très long, peut-être à cause de l'enum qui oblige à utiliser boundary_type::text like '%coastline_sea_limit%'
				if (boundaryType != "international_boundary" && boundaryType.find("coastline_sea_limit") == -1)
					continue;

				ign::geometry::LineString lsBoundary = fBoundary.getGeometry().asLineString();

				ign::geometry::algorithm::BufferOpGeos buffOp;
				ign::geometry::GeometryPtr buffBorder(buffOp.buffer(lsBoundary, distBuffer, 0, ign::geometry::algorithm::BufferOpGeos::CAP_FLAT));

				ign::feature::Feature fBuff;
				fBuff.setGeometry(buffBorder->clone());
				_shapeLogger->writeFeature("getCLfromBorder_buffers", fBuff);

				getCLfromBorder(lsBoundary, buffBorder, distBuffer, thresholdNoCL, angleMaxBorder, ratioInBuff, snapOnVertexBorder);


			}
		}

		double distMergeCL = 1;
		//mergeCL(distMergeCL, snapOnVertexBorder); 
		mergeIntersectingCL(countryCodeDouble, distMergeCL, snapOnVertexBorder);


		{
			ign::feature::FeatureIteratorPtr itBoundary = _fsBoundary->getFeatures(ign::feature::FeatureFilter(countryCodeName + " = '" + countryCodeDouble + "'"));
			while (itBoundary->hasNext()) {
				ign::feature::Feature fBoundary = itBoundary->next();
				std::string boundaryType = fBoundary.getAttribute("boundary_type").toString();
				if (boundaryType != "international_boundary" && boundaryType.find("coastline_sea_limit") == -1)
					continue;
				ign::geometry::LineString lsBoundary = fBoundary.getGeometry().asLineString();
				ign::geometry::algorithm::BufferOpGeos buffOp;
				ign::geometry::GeometryPtr buffBorder(buffOp.buffer(lsBoundary, distBuffer, 0, ign::geometry::algorithm::BufferOpGeos::CAP_FLAT));

				getCPfromIntersectBorder(lsBoundary);

				addToUndershootNearBorder(lsBoundary, buffBorder, distBuffer);
			}
		}

		double distMergeCP = 5;
		//mergeCPNearBy(distMergeCP, 0);
		snapCPNearBy(countryCodeDouble,distMergeCP, 0);
	

		_logger->log( epg::log::TITLE, "[ END CF GENERATION FOR " + countryCodeDouble + " ] : " + epg::tools::TimeTools::getTime() );

	}

}
}


void app::calcul::connectFeatGenerationOp::getCLfromBorder(
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
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();

	ign::feature::FeatureFilter filter("ST_INTERSECTS(" + geomName + ", '" + buffBorder->toString() + "')");
	epg::tools::FilterTools::addAndConditions(filter, _reqFilterEdges2generateCF);
	//filter.setExtent(lsBorder.expandBy(distBuffer));

	ign::feature::FeatureIteratorPtr eit = _fsEdge->getFeatures(filter);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsEdge, filter);
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
				if (lsCL.numPoints() >= 2) {
					vLsProjectedOnBorder.push_back(lsCL);
				}
			}
		}
		else 
		{
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


void app::calcul::connectFeatGenerationOp::addToUndershootNearBorder(
	ign::geometry::LineString & lsBorder,
	ign::geometry::GeometryPtr& buffBorder,
	double distBuffer
)
{
	epg::log::ShapeLogger* _shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();

	ign::feature::FeatureFilter filterBuffBorder("ST_INTERSECTS(" + geomName + ", '" + buffBorder->toString() + "')");
	epg::tools::FilterTools::addAndConditions(filterBuffBorder, _reqFilterEdges2generateCF);

	ign::feature::FeatureIteratorPtr eit = _fsEdge->getFeatures(filterBuffBorder);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsEdge, filterBuffBorder);
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
		filterArroundPt.setPropertyConditions(_reqFilterEdges2generateCF);
		filterArroundPt.setExtent(ptClosestBorder.getEnvelope().expandBy(1));
		ign::feature::FeatureIteratorPtr eitArroundPt = _fsEdge->getFeatures(filterArroundPt);
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
		//fCF.setAttribute("w_calcul", ign::data::String("1"));
		_fsTmpCP->createFeature(fCF, idCP);
		{
			ign::feature::Feature fShaplog = fCF;
			ign::geometry::Point ptProjNoZ = projPt;
			ptProjNoZ.clearZ();
			fShaplog.setGeometry(ptProjNoZ);
			_shapeLogger->writeFeature("CPUndershoot", fShaplog);
		}
	}
}


void app::calcul::connectFeatGenerationOp::getCPfromIntersectBorder(
	ign::geometry::LineString & lsBorder
)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();

	ign::feature::FeatureFilter filterFeaturesToMatch("ST_INTERSECTS(" + geomName + ", '" + lsBorder.toString() + "')");
	epg::tools::FilterTools::addAndConditions(filterFeaturesToMatch, _reqFilterEdges2generateCF);
	ign::feature::FeatureIteratorPtr itFeaturesToMatch = _fsEdge->getFeatures(filterFeaturesToMatch);

	int numFeatures = context->getDataBaseManager().numFeatures(*_fsEdge, filterFeaturesToMatch);
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

double app::calcul::connectFeatGenerationOp::getAngleEdgeWithBorder(
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


void app::calcul::connectFeatGenerationOp::getGeomCL(
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




bool app::calcul::connectFeatGenerationOp::isEdgeIntersectedPtWithCL(
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



/*void app::calcul::connectFeatGenerationOp::mergeCPNearBy(
	double distMergeCP,
	double snapOnVertexBorder
)
{
	epg::Context* context = epg::ContextS::getInstance();
	params::TransParameters* themeParameters = params::TransParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

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
			fCPNew.setAttribute(themeParameters->getValue(CF_STATUS).toString(), ign::data::String("edge_matched"));

			// Patch a supprimer quand la table aura été revue
			fCPNew.setAttribute("vertical_level", ign::data::Integer(0));
			fCPNew.setAttribute("maximum_height", ign::data::Integer(-999));
			fCPNew.setAttribute("maximum_length", ign::data::Integer(-999));
			fCPNew.setAttribute("maximum_width", ign::data::Integer(-999));
			fCPNew.setAttribute("maximum_total_weight", ign::data::Integer(-999));
			fCPNew.setAttribute("maximum_single_axle_weight", ign::data::Integer(-999));
			fCPNew.setAttribute("maximum_double_axle_weight", ign::data::Integer(-999));
			fCPNew.setAttribute("maximum_triple_axle_weight", ign::data::Integer(-999));


			_fsTmpCP->createFeature(fCPNew, idCPNew);
		}			
	}

	for (std::set<std::string>::iterator sit = sCP2Merge.begin(); sit != sCP2Merge.end(); ++sit) {
		_fsTmpCP->deleteFeature(*sit);
	}
}*/



void app::calcul::connectFeatGenerationOp::snapCPNearBy(
	std::string countryCodeDouble,
	double distMergeCP,
	double snapOnVertexBorder
)
{
	epg::Context* context = epg::ContextS::getInstance();
	params::TransParameters* themeParameters = params::TransParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCP;
	std::vector<std::string> vCountriesCodeName;
	epg::tools::StringTools::Split(countryCodeDouble, "#", vCountriesCodeName);
	for (size_t i = 0; i < vCountriesCodeName.size(); ++i) {
		epg::tools::FilterTools::addOrConditions(filterCP, countryCodeName + " = '" + vCountriesCodeName[i] + "'");
	}
	ign::feature::FeatureIteratorPtr itCP = _fsTmpCP->getFeatures(filterCP);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsTmpCP, filterCP);
	boost::progress_display display(numFeatures, std::cout, "[ FUSION CONNECTING POINTS WITH #]\n");

	std::set<std::string> sCP2Snap;
	std::string separator = "#";

	while (itCP->hasNext())
	{
		++display;

		ign::feature::Feature fCPCurr = itCP->next();
		std::string idCP = fCPCurr.getId();

		if (sCP2Snap.find(idCP) != sCP2Snap.end())
			continue;

		std::map<std::string, ign::feature::Feature> mCPNear;
		bool hasNearestCP = getNearestCP(fCPCurr, distMergeCP, mCPNear);
		if (hasNearestCP) {
			ign::geometry::MultiPoint multiPtCP;
			for (std::map<std::string, ign::feature::Feature>::iterator mit = mCPNear.begin(); mit != mCPNear.end(); ++mit) {
				sCP2Snap.insert(mit->first);
				multiPtCP.addGeometry(mit->second.getGeometry());
			}
			
			//geom
			ign::geometry::Point ptCentroidCP = multiPtCP.asMultiPoint().getCentroid();
			ign::feature::FeatureFilter filterBorderNearCP;// (countryCodeName + " = 'be#fr'");
			filterBorderNearCP.setExtent(ptCentroidCP.getEnvelope().expandBy(distMergeCP));
			ign::geometry::LineString lsBorderClosest;
			double distMinBorder = 2 * distMergeCP;
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

			//boucle sur mCPNear pour modif
			for (std::map<std::string, ign::feature::Feature>::iterator mit = mCPNear.begin(); mit != mCPNear.end(); ++mit) {
				ign::feature::Feature fCP2snap;
				_fsTmpCP->getFeatureById(mit->first, fCP2snap);
				fCP2snap.setGeometry(ptCentroidOnBorderCP);
				fCP2snap.setAttribute(themeParameters->getValue(CF_STATUS).toString(), ign::data::String("edge_matched"));
				_fsTmpCP->modifyFeature(fCP2snap);
			}

		}
	}

}

bool app::calcul::connectFeatGenerationOp::getNearestCP(
	ign::feature::Feature fCP,
	double distMergeCP,
	std::map<std::string,ign::feature::Feature>& mCPNear
)
{
	epg::Context* context = epg::ContextS::getInstance();
	mCPNear[fCP.getId()] = fCP;
	
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
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

void app::calcul::connectFeatGenerationOp::addFeatAttributeMergingOnBorder(
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
			|| attrName == "w_calcul" || attrName == "net_type"
			|| attrName == "w_step" 
			//|| attrName.find("w_") != std::string::npos 
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


void app::calcul::connectFeatGenerationOp::mergeIntersectingCL(
	std::string countryCodeDouble,
	double distMergeCL,
	double snapOnVertexBorder
)
{
	epg::log::ShapeLogger* _shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	params::TransParameters* themeParameters = params::TransParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCL;
	std::vector<std::string> vCountriesCodeName;
	epg::tools::StringTools::Split(countryCodeDouble, "#", vCountriesCodeName);
	for (size_t i = 0; i < vCountriesCodeName.size(); ++i) {
		epg::tools::FilterTools::addOrConditions(filterCL, countryCodeName + " = '" + vCountriesCodeName [i]+ "'");
	}
	ign::feature::FeatureIteratorPtr itCL = _fsTmpCL->getFeatures(filterCL);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsTmpCL, filterCL);
	boost::progress_display display(numFeatures, std::cout, "[ FUSION CONNECTING LINES WITH #]\n");

	std::set<std::string> sCL2Merged;
	std::string separator = "#";

	while (itCL->hasNext())
	{
		++display;
		ign::feature::Feature fCLCurr = itCL->next();

		std::string idCLCurr = fCLCurr.getId();
		ign::geometry::LineString lsCurr = fCLCurr.getGeometry().asLineString();
		std::string countryCodeCLCurr = fCLCurr.getAttribute(countryCodeName).toString();

		if (countryCodeCLCurr.find("#") != std::string::npos)
			continue;

		if (sCL2Merged.find(idCLCurr) != sCL2Merged.end())
			continue;

		sCL2Merged.insert(idCLCurr);
		//std::map<std::string, ign::feature::Feature> mCL2merge;
		//std::set<std::string> sCountryCode;
		ign::feature::FeatureFilter filterArroundCL;
		filterArroundCL.setPropertyConditions(countryCodeName + " != '" + countryCodeCLCurr + "'");
		filterArroundCL.setExtent(lsCurr.getEnvelope());
		ign::feature::FeatureIteratorPtr itArroundCL = _fsTmpCL->getFeatures(filterArroundCL);
		while (itArroundCL->hasNext())
		{
			ign::feature::Feature fCLArround = itArroundCL->next();
			std::string idClArround = fCLArround.getId();
			ign::geometry::LineString lsClArround = fCLArround.getGeometry().asLineString();

			//si CL deja traite on ne fait rien
			if (sCL2Merged.find(idClArround) != sCL2Merged.end())
				continue;
			std::string countryCodeCLArround = fCLArround.getAttribute(countryCodeName).toString();
			if (countryCodeCLArround.find("#") != std::string::npos)
				continue;
			//si pas d'intersection avec une CL d'un autre pays on ne crée pas de CL merged
			if (!lsCurr.intersects(lsClArround))
				continue;

			ign::geometry::Geometry* geomIntersect = lsCurr.Intersection(lsClArround);

			ign::geometry::LineString lsIntersectedCL;

			if (geomIntersect->isLineString()) {
				lsIntersectedCL = geomIntersect->asLineString();
			}
			else if (geomIntersect->isMultiLineString()) {
				ign::geometry::MultiLineString mlsCLTmp= geomIntersect->asMultiLineString();
				lsIntersectedCL = mlsCLTmp.geometryN(0).asLineString();
				for (size_t i = 1; i < mlsCLTmp.numGeometries(); ++i) {
					ign::geometry::LineString lsCLTmp = mlsCLTmp.geometryN(i).asLineString();
					//on ne garde pas le 1er pt qui est le même que le dernier de la ls précédente
					for (size_t j = 1; j < lsCLTmp.numPoints(); ++j) {
						lsIntersectedCL.addPoint(lsCLTmp.pointN(j));
					}
				}
			}
			else {
				std::string geomIntersectTypeName = geomIntersect->getGeometryTypeName();
				_logger->log(epg::log::WARN, "Intersection between CL " + idClArround + " and " + idCLCurr + " is a " + geomIntersectTypeName);
				continue;
			}

			ign::feature::Feature fCLNew = _fsTmpCL->newFeature();

			if(countryCodeCLArround < countryCodeCLCurr) {
				fCLNew = fCLArround;
				addFeatAttributeMergingOnBorder(fCLNew, fCLCurr, separator);
			}
			else {
				fCLNew = fCLCurr;
				addFeatAttributeMergingOnBorder(fCLNew, fCLArround, separator);
			}

			std::string idCLNew = _idGeneratorCL->next();
			fCLNew.setId(idCLNew);
			fCLNew.setGeometry(lsIntersectedCL);

			fCLNew.setAttribute(themeParameters->getValue(CF_STATUS).toString(), ign::data::String("edge_matched"));

			// Patch a supprimer quand la table aura été revue
			fCLNew.setAttribute("maximum_height", ign::data::Integer(-999));
			fCLNew.setAttribute("maximum_length", ign::data::Integer(-999));
			fCLNew.setAttribute("maximum_width", ign::data::Integer(-999));
			fCLNew.setAttribute("maximum_total_weight", ign::data::Integer(-999));
			fCLNew.setAttribute("maximum_single_axle_weight", ign::data::Integer(-999));
			fCLNew.setAttribute("maximum_double_axle_weight", ign::data::Integer(-999));
			fCLNew.setAttribute("maximum_triple_axle_weight", ign::data::Integer(-999));

			_fsTmpCL->createFeature(fCLNew, idCLNew);

		}
	}

	_logger->log(epg::log::INFO, "Nb de CL suppr apres merging " + ign::data::Integer(sCL2Merged.size()).toString());
	for (std::set<std::string>::iterator sit = sCL2Merged.begin(); sit != sCL2Merged.end(); ++sit) {
		_fsTmpCL->deleteFeature(*sit);
	}

}
/*void app::calcul::connectFeatGenerationOp::mergeCL(
	double distMergeCL,
	double snapOnVertexBorder
)
{
	epg::log::ShapeLogger* _shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	params::TransParameters* themeParameters = params::TransParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCL;
	ign::feature::FeatureIteratorPtr itCL = _fsTmpCL->getFeatures(filterCL);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsTmpCL, filterCL);
	boost::progress_display display(numFeatures, std::cout, "[ FUSION CONNECTING LINES WITH #]\n");

	std::set<std::string> sCL2Merged;
	std::string separator = "#";

	while (itCL->hasNext())
	{
		++display;
		ign::feature::Feature fCLCurr = itCL->next();

		std::string idCL = fCLCurr.getId();
		ign::geometry::LineString lsCurr = fCLCurr.getGeometry().asLineString();

		if (sCL2Merged.find(idCL) != sCL2Merged.end())
			continue;

		std::map<std::string, ign::feature::Feature> mCL2merge;
		std::set<std::string> sCountryCode;
		bool hasNearestCL = getCLToMerge(fCLCurr, distMergeCL, mCL2merge, sCountryCode);

		if (!hasNearestCL) {
			_shapeLogger->writeFeature("CLNotMerge", fCLCurr);
			continue;
		}

		ign::feature::Feature fCLNew = _fsTmpCL->newFeature();
		std::string idCLNew = _idGeneratorCL->next();

		ign::geometry::LineString lsSE,lsSS,lsES,lsEE,lsTempCL;

		ign::geometry::LineString lsBorder;
		getBorderFromEdge(lsCurr, lsBorder);
		
		for (std::map<std::string, ign::feature::Feature>::iterator mit = mCL2merge.begin(); mit != mCL2merge.end(); ++mit) {
			sCL2Merged.insert(mit->first);
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
		if (sCountryCode.size() >= 2) {		
			fCLNew.setAttribute(themeParameters->getValue(CF_STATUS).toString(),ign::data::String("edge_matched"));
		}
		else {
			for (std::map<std::string, ign::feature::Feature>::iterator mit = mCL2merge.begin(); mit != mCL2merge.end(); ++mit) {
				_shapeLogger->writeFeature("CLno2country", mit->second);
			}
		}
		fCLNew.setGeometry(lsTempCL);

		// Patch a supprimer quand la table aura été revue
		fCLNew.setAttribute("maximum_height", ign::data::Integer(-999));
		fCLNew.setAttribute("maximum_length", ign::data::Integer(-999));
		fCLNew.setAttribute("maximum_width", ign::data::Integer(-999));
		fCLNew.setAttribute("maximum_total_weight", ign::data::Integer(-999));
		fCLNew.setAttribute("maximum_single_axle_weight", ign::data::Integer(-999));
		fCLNew.setAttribute("maximum_double_axle_weight", ign::data::Integer(-999));
		fCLNew.setAttribute("maximum_triple_axle_weight", ign::data::Integer(-999));


		_fsTmpCL->createFeature(fCLNew, idCLNew);
	}

	for (std::set<std::string>::iterator sit = sCL2Merged.begin(); sit != sCL2Merged.end(); ++sit) {
		_fsTmpCL->deleteFeature(*sit);
	}
}*/



bool app::calcul::connectFeatGenerationOp::getCLToMerge(
	ign::feature::Feature fCL,
	double distMergeCL,
	std::map < std::string, ign::feature::Feature>& mCL2merge,
	std::set<std::string>& sCountryCode
)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

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


void app::calcul::connectFeatGenerationOp::getBorderFromEdge(
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
		if (distBorder < 0.1) {
			lsBorder = lsBorderTemp;
			return;
		}
			
	}

}