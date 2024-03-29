#include <app/calcul/connectFeatGenerationOp.h>

//BOOST
#include <boost/timer.hpp>
#include <boost/progress.hpp>

//SOCLE
#include <ign/geometry/algorithm/BufferOpGeos.h>
#include <ign/math/Line2T.h>
#include <ign/math/LineT.h>
#include <ign/geometry/tools/LengthIndexedLineString.h>

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





void app::calcul::connectFeatGenerationOp::compute(
	std::string countryCode,
	bool verbose)
{
	connectFeatGenerationOp connectFeatGenerationOp(countryCode,verbose);
	connectFeatGenerationOp._compute();
}


app::calcul::connectFeatGenerationOp::connectFeatGenerationOp(std::string countryCode,
	bool verbose)
{
	//debug//test
	_init(countryCode, verbose);
}

app::calcul::connectFeatGenerationOp::~connectFeatGenerationOp()
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
void app::calcul::connectFeatGenerationOp::_init(std::string countryCode, bool verbose)
{
	_logger = epg::log::EpgLoggerS::getInstance();
	_logger->log(epg::log::TITLE, "[ BEGIN INITIALIZATION ] : " + epg::tools::TimeTools::getTime());


	epg::Context* context = epg::ContextS::getInstance();
	params::ThemeParameters* themeParameters = params::ThemeParametersS::getInstance();
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

	///recuperation de la liste des attributs à concatener dans la fusion des attributs
	std::string listAttrWName = themeParameters->getValue(LIST_ATTR_W).toString();
	//on recup les attribut a concat et on les mets dans un vecteur en splitant
	std::vector<std::string> _vAttrNameW;
	epg::tools::StringTools::Split(listAttrWName, "/", _vAttrNameW);
	for (size_t i = 0; i < _vAttrNameW.size(); ++i) {
		_sAttrNameW.insert(_vAttrNameW[i]);
	}


	///recuperation des features
	std::string const boundaryTableName = epg::utils::replaceTableName(context->getEpgParameters().getValue(TARGET_BOUNDARY_TABLE).toString());
	_fsBoundary = context->getDataBaseManager().getFeatureStore(boundaryTableName, idName, geomName);

	std::string const landmaskableName = epg::utils::replaceTableName(context->getEpgParameters().getValue(TARGET_COUNTRY_TABLE).toString());
	_fsLandmask= context->getDataBaseManager().getFeatureStore(landmaskableName, idName, geomName);


	std::string edgeSourceTableName = themeParameters->getValue(SOURCE_ROAD_TABLE).toString();
	std::string edgeTargetTableName = "_110_" + epg::utils::replaceTableName(themeParameters->getValue(SOURCE_ROAD_TABLE).toString());
	context->getEpgParameters().setParameter(EDGE_TABLE, ign::data::String(edgeTargetTableName));

	epg::utils::CopyTableUtils::copyEdgeTable(edgeSourceTableName, "", false);

	_fsEdge = context->getFeatureStore(epg::EDGE);

	//context->getDataBaseManager().addColumn(edgeTargetTableName, "is_linked_to_cl", "boolean", "false");
	//context->getDataBaseManager().refreshFeatureStore(_fsEdge->getTableName(),idName,geomName);

	_reqFilterEdges2generateCF = themeParameters->getValue(SQL_FILTER_EDGES_2_GENERATE_CF).toString();
	//_filterEdges2generateCF.setPropertyConditions(reqFilterEdges2generateCF);

	///Create tmp_cp table
	std::string cpTableName = epg::utils::replaceTableName(themeParameters->getValue(CP_TABLE).toString());

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
			<< "w_national_identifier" << " type varchar(255);"
			<< "ALTER TABLE " << cpTableName << " ALTER COLUMN "
			<< "national_road_code" << " type varchar(255);"
			<< "ALTER TABLE " << cpTableName << " ALTER COLUMN "
			<< "european_route_number" << " type varchar(255);"
			<< "ALTER TABLE " << cpTableName << " ADD COLUMN "
			<< themeParameters->getValue(CF_STATUS).toString() << " cf_status_value default 'not_edge_matched';"
			<< "ALTER TABLE " << cpTableName << " ADD COLUMN " << context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString() << " character varying(255);";

		context->getDataBaseManager().getConnection()->update(ss.str());
	}
	// Create tmp_cl table
	std::string clTableName = epg::utils::replaceTableName(themeParameters->getValue(CL_TABLE).toString());
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
			<< "w_national_identifier" << " type varchar(255);"
			<< "ALTER TABLE " << clTableName << " ALTER COLUMN "
			<< "national_road_code" << " type varchar(255);"
			<< "ALTER TABLE " << clTableName << " ALTER COLUMN "
			<< "european_route_number" << " type varchar(255);"
			<< "ALTER TABLE " << clTableName << " ADD COLUMN "
			<< themeParameters->getValue(CF_STATUS).toString() << " cf_status_value default 'not_edge_matched';"
			<< "ALTER TABLE " << clTableName << " ADD COLUMN " << context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString() << " character varying(255);";
		//patch pour ne pas avoir des enums et eviter les soucis lors de la fusion des attributs	
		for (std::set<std::string>::iterator sit = _sAttrNameToConcat.begin(); sit != _sAttrNameToConcat.end(); ++sit)
			ss << "ALTER TABLE " << clTableName << " ALTER COLUMN " << *sit << " TYPE character varying(255);";
		//ss << "CREATE TRIGGER ome2_reduce_precision_2d_trigger BEFORE INSERT OR UPDATE ON " << clTableName << " FOR EACH ROW EXECUTE FUNCTION public.ome2_reduce_precision_2d_trigger_function();";
			
				
		context->getDataBaseManager().getConnection()->update(ss.str());
	}

	_fsCP = context->getDataBaseManager().getFeatureStore(cpTableName, idName, geomName);
	_fsCL = context->getDataBaseManager().getFeatureStore(clTableName, idName, geomName);

	// id generator
	_idGeneratorCP = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsCP, "CONNECTINGPOINT"));
	_idGeneratorCL = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsCL, "CONNECTINGLINE"));

	_logger->log(epg::log::TITLE, "[ END INITIALIZATION ] : " + epg::tools::TimeTools::getTime());
}



	///
	///
///
void app::calcul::connectFeatGenerationOp::getCountryCodeDoubleFromCC(std::string countryCC, std::set<std::string>& sCountryCodeDouble)
{
	//recup des icc pour un pays
	epg::Context* context = epg::ContextS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
	ign::feature::FeatureIteratorPtr itBoundary = _fsBoundary->getFeatures(ign::feature::FeatureFilter(countryCodeName + " like '%" + countryCC + "%' and "+ countryCodeName + " like '%#%'"));
	while (itBoundary->hasNext())
	{
		ign::feature::Feature fBoundary = itBoundary->next();
		std::string boundaryType = fBoundary.getAttribute("boundary_type").toString();
		if (boundaryType != "international_boundary" && boundaryType.find("coastline_sea_limit") == -1)
			continue;
		std::string  countryCodeBoundary=fBoundary.getAttribute(countryCodeName).toString();
		sCountryCodeDouble.insert(countryCodeBoundary);
	}

}

///
///
///
void app::calcul::connectFeatGenerationOp::_compute()
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
void app::calcul::connectFeatGenerationOp::_computeOnDoubleCC(std::string countryCodeDouble)
{
	_logger->log( epg::log::TITLE, "[ BEGIN CF GENERATION FOR "+ countryCodeDouble +" ] : " + epg::tools::TimeTools::getTime() );

	epg::Context* context = epg::ContextS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	//CL
	double distBuffer = 5;
	double thresholdNoCL = 0.1;
	double ratioInBuff = 0.6;
	//double snapOnVertexBorder = 1;
	double snapOnVertexBorder = 1;
	double angleMaxBorder = 25;
	angleMaxBorder = angleMaxBorder * M_PI / 180;
	double distCLIntersected = 10;
	double distUnderShoot = 10;
	double distMergeCL = 1;
	double distMergeCP = 2;

	std::vector<std::string> vCountriesCodeName;
	epg::tools::StringTools::Split(countryCodeDouble, "#", vCountriesCodeName);
	/*for (size_t i = 0; i < vCountriesCodeName.size(); ++i) {
		cleanAntennasOutOfCountry(vCountriesCodeName[i]);
	}*/
//debug//test
/*
	for (size_t i = 0; i < vCountriesCodeName.size(); ++i) {
		cleanEdgesOutOfCountry(vCountriesCodeName[i]);
	}
	
	// Go through objects intersecting the boundary and create the CL
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

		//suppression de la colonne de lien des cl sur les edges
		//context->getDataBaseManager().getConnection()->update("ALTER TABLE " + _fsEdge->getTableName() + " DROP COLUMN IF EXISTS is_linked_to_cl");
		//context->getDataBaseManager().refreshFeatureStore(_fsEdge->getTableName(), idName, geomName);
		
	}

	// Go through objects intersecting the boundary and create the CP
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

			getCPfromIntersectBorder(lsBoundary, distCLIntersected);

			addToUndershootNearBorder(lsBoundary, buffBorder, distUnderShoot);
		}
	}


	mergeIntersectingCL(countryCodeDouble, distMergeCL, snapOnVertexBorder);

	snapCPNearBy(countryCodeDouble,distMergeCP, 0);

	deleteCLUnderThreshold(countryCodeDouble);
//debug//test*/
	updateGeomCL(countryCodeDouble, snapOnVertexBorder);

	_logger->log( epg::log::TITLE, "[ END CF GENERATION FOR " + countryCodeDouble + " ] : " + epg::tools::TimeTools::getTime() );
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
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	std::vector<ign::feature::FeatureAttributeType> listAttrEdge = _fsEdge->getFeatureType().attributes();

	ign::feature::FeatureFilter filter("ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + buffBorder->toString() + "'),3035))");
	//ign::feature::FeatureFilter filter("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + buffBorder->toString() + "'))");
	epg::tools::FilterTools::addAndConditions(filter, _reqFilterEdges2generateCF);

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
			ign::feature::Feature featCL = _fsCL->newFeature();
			featCL.setGeometry(vLsProjectedOnBorder[i]);
			std::string idCL = _idGeneratorCL->next();
			featCL.setId(idCL);
			featCL.setAttribute(linkedFeatIdName, ign::data::String(fEdge.getId()));
			for (std::vector<ign::feature::FeatureAttributeType>::iterator vit = listAttrEdge.begin();
				vit != listAttrEdge.end(); ++vit) {
				std::string attrName = vit->getName();
				if (attrName == geomName || attrName == idName || !_fsCL->getFeatureType().hasAttribute(attrName) )
					continue;
				featCL.setAttribute(attrName,fEdge.getAttribute(attrName));
			}
			_fsCL->createFeature(featCL, idCL);
		}
		/*if (vLsProjectedOnBorder.size() > 0) { //edge lié à au moins une CL
			fEdge.setAttribute("is_linked_to_cl", ign::data::Boolean(true));
			_fsEdge->modifyFeature(fEdge);
		}*/
	}	
}


void app::calcul::connectFeatGenerationOp::addToUndershootNearBorder(
	ign::geometry::LineString & lsBorder,
	ign::geometry::GeometryPtr& buffBorder,
	double distUnderShoot
)
{
	epg::log::ShapeLogger* _shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	std::vector<ign::feature::FeatureAttributeType> listAttrEdge = _fsEdge->getFeatureType().attributes();

	//ign::feature::FeatureFilter filterBuffBorder("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + buffBorder->toString() + "'))");
	ign::feature::FeatureFilter filterBuffBorder("ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + buffBorder->toString() + "'),3035))");
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

		if (ptClosestBorder.distance(projPt) > distUnderShoot )
			continue;
		double distCLIntersected = distUnderShoot;
		if (isEdgeIntersectedPtWithCL(fEdge, projPt, distCLIntersected))
			continue;

		projPt.setZ(0);
		ign::feature::Feature fCF = _fsCP->newFeature();
		fCF.setGeometry(projPt);
		std::string idCP = _idGeneratorCP->next();
		fCF.setId(idCP);
		fCF.setAttribute(linkedFeatIdName, ign::data::String(fEdge.getId()));
		for (std::vector<ign::feature::FeatureAttributeType>::iterator vit = listAttrEdge.begin();
			vit != listAttrEdge.end(); ++vit) {
			std::string attrName = vit->getName();
			if (attrName == geomName || attrName == idName || !_fsCP->getFeatureType().hasAttribute(attrName))
				continue;
			fCF.setAttribute(attrName, fEdge.getAttribute(attrName));
		}
		//fCF.setAttribute("w_calcul", ign::data::String("1"));
		_fsCP->createFeature(fCF, idCP);
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
	ign::geometry::LineString & lsBorder,
	double distCLIntersected
)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	std::vector<ign::feature::FeatureAttributeType> listAttrEdge = _fsEdge->getFeatureType().attributes();

	//ign::feature::FeatureFilter filterFeaturesToMatch("ST_INTERSECTS(" + geomName + ", ST_GeomFromText('" + lsBorder.toString() + "'))");
	ign::feature::FeatureFilter filterFeaturesToMatch("ST_INTERSECTS(" + geomName + ", ST_SetSRID(ST_GeomFromText('" + lsBorder.toString() + "'),3035))");
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

		ign::feature::Feature fCF = _fsCP->newFeature();
		fCF.setAttribute(linkedFeatIdName, ign::data::String(fToMatch.getId()));
		for (std::vector<ign::feature::FeatureAttributeType>::iterator vit = listAttrEdge.begin();
			vit != listAttrEdge.end(); ++vit) {
			std::string attrName = vit->getName();
			if (attrName == geomName || attrName == idName || !_fsCP->getFeatureType().hasAttribute(attrName))
				continue;
			fCF.setAttribute(attrName, fToMatch.getAttribute(attrName));
		}

		if (geomPtr->isPoint())
		{
			//si l'edge sert à une CL et, ne pas créer de CP?
			bool isConnectedToCL = isEdgeIntersectedPtWithCL(fToMatch, geomPtr->asPoint(), distCLIntersected);
			if (!isConnectedToCL) {
				fCF.setGeometry(geomPtr->asPoint());
				std::string idCP = _idGeneratorCP->next();
				_fsCP->createFeature(fCF, idCP);
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
					
					bool isConnectedToCL = isEdgeIntersectedPtWithCL(fToMatch, ptIntersect, distCLIntersected);
					if (!isConnectedToCL) {
						fCF.setGeometry(ptIntersect);
						std::string idCP = _idGeneratorCP->next();
						_fsCP->createFeature(fCF, idCP);
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
	ign::geometry::Point ptIntersectBorder,
	double distCLIntersected 
)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();
	std::string idLinkedEdge = fEdge.getId();
	ign::feature::FeatureFilter filterIntersectCL (linkedFeatIdName+" like '%" + idLinkedEdge +"%'");
	filterIntersectCL.setExtent(ptIntersectBorder.getEnvelope().expandBy(distCLIntersected));
	ign::feature::FeatureIteratorPtr itIntersectedCL = _fsCL->getFeatures(filterIntersectCL);

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
	params::ThemeParameters* themeParameters = params::ThemeParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCP;
	ign::feature::FeatureIteratorPtr itCP = _fsCP->getFeatures(filterCP);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsCP, filterCP);
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
			ign::feature::Feature fCPNew = _fsCP->newFeature();
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


			_fsCP->createFeature(fCPNew, idCPNew);
		}			
	}

	for (std::set<std::string>::iterator sit = sCP2Merge.begin(); sit != sCP2Merge.end(); ++sit) {
		_fsCP->deleteFeature(*sit);
	}
}*/



void app::calcul::connectFeatGenerationOp::snapCPNearBy(
	std::string countryCodeDouble,
	double distMergeCP,
	double snapOnVertexBorder
)
{

	epg::Context* context = epg::ContextS::getInstance();
	params::ThemeParameters* themeParameters = params::ThemeParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCP;
	std::vector<std::string> vCountriesCodeName;
	epg::tools::StringTools::Split(countryCodeDouble, "#", vCountriesCodeName);
	for (size_t i = 0; i < vCountriesCodeName.size(); ++i) {
		epg::tools::FilterTools::addOrConditions(filterCP, countryCodeName + " = '" + vCountriesCodeName[i] + "'");
	}
	ign::feature::FeatureIteratorPtr itCP = _fsCP->getFeatures(filterCP);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsCP, filterCP);
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
				_fsCP->getFeatureById(mit->first, fCP2snap);
				fCP2snap.setGeometry(ptCentroidOnBorderCP);
				fCP2snap.setAttribute(themeParameters->getValue(CF_STATUS).toString(), ign::data::String("edge_matched"));
				_fsCP->modifyFeature(fCP2snap);
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
	ign::feature::FeatureIteratorPtr itArroundCP = _fsCP->getFeatures(filterArroundCP);
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

	for (size_t i = 0; i < vAttrNames.size(); ++i) {
		std::string attrValueMerged;
		std::string attrName = vAttrNames[i];
		if (_sAttrNameW.find(attrName) != _sAttrNameW.end()) //on ne fusionne pas les attributs de travail
			continue;
		std::string attrValueToMerge = featMerged.getAttribute(attrName).toString();
		std::string attrValueToAdd = featAttrToAdd.getAttribute(attrName).toString();
		if (attrName == "vertical_level")
			bool bt = true;
		if (attrValueToMerge != attrValueToAdd)
			attrValueMerged = attrValueToMerge + separator + attrValueToAdd;
		else
			attrValueMerged = attrValueToMerge;
	
		featMerged.setAttribute(attrName,ign::data::String(attrValueMerged));
	}
}

/*
void app::calcul::connectFeatGenerationOp::mergeIntersectingCL(
	std::string countryCodeDouble,
	double distMergeCL,
	double snapOnVertexBorder
)
{
	epg::log::ShapeLogger* _shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	params::ThemeParameters* themeParameters = params::ThemeParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCL;
	std::vector<std::string> vCountriesCodeName;
	epg::tools::StringTools::Split(countryCodeDouble, "#", vCountriesCodeName);
	for (size_t i = 0; i < vCountriesCodeName.size(); ++i) {
		epg::tools::FilterTools::addOrConditions(filterCL, countryCodeName + " = '" + vCountriesCodeName [i]+ "'");
	}
	ign::feature::FeatureIteratorPtr itCL = _fsCL->getFeatures(filterCL);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsCL, filterCL);
	boost::progress_display display(numFeatures, std::cout, "[ FUSION CONNECTING LINES WITH #]\n");

	std::set<std::string> sCL2Merged;
	std::string separator = "#";

	while (itCL->hasNext())
	{
		++display;
		ign::feature::Feature fCLCurr = itCL->next();

		//debug
		std::string const natIdName = themeParameters->getValue(NATIONAL_IDENTIFIER).toString();
		if(fCLCurr.getAttribute(natIdName).toString()=="TRONROUT0000000312382974" )
			bool bStop =true;
		else if(fCLCurr.getAttribute(natIdName).toString() == "TRONROUT0000000057202002")
			bool bStop = true;
		else if (fCLCurr.getAttribute(natIdName).toString() == "{8CF7D863-AF16-41A7-BF66-8135595E21A9}")
			bool bStop = true;

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
		ign::feature::FeatureIteratorPtr itArroundCL = _fsCL->getFeatures(filterArroundCL);
		while (itArroundCL->hasNext())
		{
			ign::feature::Feature fCLArround = itArroundCL->next();
			std::string idClArround = fCLArround.getId();
			ign::geometry::LineString lsClArround = fCLArround.getGeometry().asLineString();

			//debug
			std::string const natIdName = themeParameters->getValue(NATIONAL_IDENTIFIER).toString();
			if (fCLCurr.getAttribute(natIdName).toString() == "TRONROUT0000000312382974")
				bool bStop = true;
			else if (fCLCurr.getAttribute(natIdName).toString() == "TRONROUT0000000057202002")
				bool bStop = true;
			else if (fCLCurr.getAttribute(natIdName).toString() == "{8CF7D863-AF16-41A7-BF66-8135595E21A9}")
				bool bStop = true;

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

			ign::feature::Feature fCLNew = _fsCL->newFeature();

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

			_fsCL->createFeature(fCLNew, idCLNew);

		}
	}

	_logger->log(epg::log::INFO, "Nb de CL suppr apres merging " + ign::data::Integer(sCL2Merged.size()).toString());
	for (std::set<std::string>::iterator sit = sCL2Merged.begin(); sit != sCL2Merged.end(); ++sit) {
		_fsCL->deleteFeature(*sit);
	}

}*/
void app::calcul::connectFeatGenerationOp::mergeIntersectingCL(
	std::string countryCodeDouble,
	double distMergeCL,
	double snapOnVertexBorder
)
{
	epg::log::ShapeLogger* _shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	params::ThemeParameters* themeParameters = params::ThemeParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCL;
	std::vector<std::string> vCountriesCodeName;
	epg::tools::StringTools::Split(countryCodeDouble, "#", vCountriesCodeName);
	for (size_t i = 0; i < vCountriesCodeName.size(); ++i) {
		epg::tools::FilterTools::addOrConditions(filterCL, countryCodeName + " = '" + vCountriesCodeName[i] + "'");
	}
	//std::string const natIdName = themeParameters->getValue(NATIONAL_IDENTIFIER).toString();
	ign::feature::FeatureIteratorPtr itCL = _fsCL->getFeatures(filterCL);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsCL, filterCL);
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
		//debug	
		/*if (fCLCurr.getAttribute(natIdName).toString() == "TRONROUT0000000057243619")
			bool bStop = true;
		else if (fCLCurr.getAttribute(natIdName).toString() == "{2005FE06-ED7A-4CA6-A011-471ADD678B46}")
			bool bStop = true;*/
		if (countryCodeCLCurr.find("#") != std::string::npos)
			continue;
		if (sCL2Merged.find(idCLCurr) != sCL2Merged.end())
			continue;
		sCL2Merged.insert(idCLCurr);

		ign::geometry::LineString lsBorder;
		getBorderFromEdge(lsCurr, lsBorder);

		ign::feature::FeatureFilter filterArroundCL;
		filterArroundCL.setPropertyConditions(countryCodeName + " != '" + countryCodeCLCurr + "'");
		filterArroundCL.setExtent(lsCurr.getEnvelope());
		ign::feature::FeatureIteratorPtr itArroundCL = _fsCL->getFeatures(filterArroundCL);
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
			if (lsCurr.distance(lsClArround) > 0.1 )
				continue;

			//si intersection uniquement aux extremites des CLs on ne merge pas
			if (lsCurr.Intersection(lsClArround)->isPoint()) {
				ign::geometry::Point ptIntersect = lsCurr.Intersection(lsClArround)->asPoint();
				if ((ptIntersect == lsCurr.startPoint() && ptIntersect == lsClArround.startPoint())
					|| (ptIntersect == lsCurr.startPoint() && ptIntersect == lsClArround.endPoint())
					|| (ptIntersect == lsCurr.endPoint() && ptIntersect == lsClArround.startPoint())
					|| (ptIntersect == lsCurr.endPoint() && ptIntersect == lsClArround.endPoint())
					) {
					ign::math::Vec2d vecLsCurr, vecLsArround;
					if (ptIntersect == lsCurr.startPoint()) {
						vecLsCurr.x() = lsCurr.endPoint().x() - lsCurr.startPoint().x();
						vecLsCurr.y() = lsCurr.endPoint().y() - lsCurr.startPoint().y();
					}
					else {
						vecLsCurr.x() = lsCurr.startPoint().x() - lsCurr.endPoint().x();
						vecLsCurr.y() = lsCurr.startPoint().y() - lsCurr.endPoint().y();
					}
					if (ptIntersect == lsClArround.startPoint()) {
						vecLsArround.x() = lsClArround.endPoint().x() - lsClArround.startPoint().x();
						vecLsArround.y() = lsClArround.endPoint().y() - lsClArround.startPoint().y();
					}
					else {
						vecLsArround.x() = lsClArround.startPoint().x() - lsClArround.endPoint().x();
						vecLsArround.y() = lsClArround.startPoint().y() - lsClArround.endPoint().y();
					}
					double anglLs = epg::tools::geometry::angle(vecLsCurr, vecLsArround);
					if (anglLs > 0.01) //si l'angle est faible, ça peut deux tronçons superposés, s'intersectant en un seul point pour erreur d'arrondi
						continue;
				}	
			}

			ign::geometry::LineString lsIntersectedCL, lsSE, lsSS, lsES, lsEE;;

			getGeomCL(lsSE, lsBorder, lsCurr.startPoint(), lsClArround.endPoint(), snapOnVertexBorder);
			getGeomCL(lsSS, lsBorder, lsCurr.startPoint(), lsClArround.startPoint(), snapOnVertexBorder);
			getGeomCL(lsES, lsBorder, lsCurr.endPoint(), lsClArround.startPoint(), snapOnVertexBorder);
			getGeomCL(lsEE, lsBorder, lsCurr.endPoint(), lsClArround.endPoint(), snapOnVertexBorder);
			
			lsIntersectedCL = lsCurr;
			double lengthMin = lsIntersectedCL.length();
			//debug
			double testLength = lsSE.length();
			if (lsSE.length() < lengthMin && lsSE.length()> 0.1 ) {//on s'assure que la section de frontière n'est pas nulle et que les projections ne se sont pas snappés au même endroit sur la frontière
				int numSegSE = static_cast<int>(std::floor(lsSE.numSegments() / 2.));
				ign::geometry::Point ptIntLsSE = epg::tools::geometry::interpolate(lsSE, numSegSE, 0.5);
				if (ptIntLsSE.distance(lsCurr) < 0.01 //on s'assure que la section est bien l'intersection et non le complement
					&& ptIntLsSE.distance(lsClArround) < 0.01) {
					lsIntersectedCL = lsSE;
					lengthMin = lsIntersectedCL.length();
				}
			}
			if (lsSS.length() < lengthMin && lsSS.length() > 0.1 ) {
				int numSegSS = static_cast<int>(std::floor(lsSS.numSegments() / 2.));
				ign::geometry::Point ptIntLsSS = epg::tools::geometry::interpolate(lsSS, numSegSS, 0.5);
				if (ptIntLsSS.distance(lsCurr) < 0.01 && ptIntLsSS.distance(lsClArround) < 0.01) {
					lsIntersectedCL = lsSS;
					lengthMin = lsIntersectedCL.length();
				}
			}
			if (lsES.length() < lengthMin && lsES.length() > 0.1){
				int numSegES = static_cast<int>(std::floor(lsES.numSegments() / 2.));
				ign::geometry::Point ptIntLsES = epg::tools::geometry::interpolate(lsES, numSegES, 0.5);
				if (ptIntLsES.distance(lsCurr) < 0.01 && ptIntLsES.distance(lsClArround) < 0.01) {
					lsIntersectedCL = lsES;
					lengthMin = lsIntersectedCL.length();
				}
			}
			if (lsEE.length() < lengthMin && lsEE.length() > 0.1 ){
				int numSegEE = static_cast<int>(std::floor(lsEE.numSegments() / 2.));
				ign::geometry::Point ptIntLsEE = epg::tools::geometry::interpolate(lsEE, numSegEE, 0.5);
				if (ptIntLsEE.distance(lsCurr) < 0.01 && ptIntLsEE.distance(lsClArround) < 0.01) {
					lsIntersectedCL = lsEE;
					lengthMin = lsIntersectedCL.length();
				}
			}
			if (lsClArround.length() < lengthMin ) {
				lsIntersectedCL = lsClArround;
				lengthMin = lsIntersectedCL.length();
			}
			//si la proj de l'intersection sur la frontiere se fait sur le meme point avec le snap on ne crée pas de CL
			if (lsIntersectedCL.numPoints() < 2)
				continue;		

			ign::feature::Feature fCLNew = _fsCL->newFeature();

			if (countryCodeCLArround < countryCodeCLCurr) {
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

			_fsCL->createFeature(fCLNew, idCLNew);

		}
	}

	//_logger->log(epg::log::INFO, "Nb de CL suppr apres merging " + ign::data::Integer(sCL2Merged.size()).toString());
	for (std::set<std::string>::iterator sit = sCL2Merged.begin(); sit != sCL2Merged.end(); ++sit) {
		_fsCL->deleteFeature(*sit);
	}

}


/*void app::calcul::connectFeatGenerationOp::mergeCL(
	double distMergeCL,
	double snapOnVertexBorder
)
{
	epg::log::ShapeLogger* _shapeLogger = epg::log::ShapeLoggerS::getInstance();
	epg::Context* context = epg::ContextS::getInstance();
	params::ThemeParameters* themeParameters = params::ThemeParametersS::getInstance();
	std::string countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	ign::feature::FeatureFilter filterCL;
	ign::feature::FeatureIteratorPtr itCL = _fsCL->getFeatures(filterCL);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsCL, filterCL);
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

		ign::feature::Feature fCLNew = _fsCL->newFeature();
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


		_fsCL->createFeature(fCLNew, idCLNew);
	}

	for (std::set<std::string>::iterator sit = sCL2Merged.begin(); sit != sCL2Merged.end(); ++sit) {
		_fsCL->deleteFeature(*sit);
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
	ign::feature::FeatureIteratorPtr itArroundCL = _fsCL->getFeatures(filterArroundCL);
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


void app::calcul::connectFeatGenerationOp::cleanEdgesOutOfCountry(std::string countryCC
)
{
	_logger->log(epg::log::TITLE, "[ BEGIN CLEAN EDGES OUT OF COUNTRY " + countryCC + " ] : " + epg::tools::TimeTools::getTime());
	epg::Context* context = epg::ContextS::getInstance();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const edgeTableName = context->getFeatureStore(epg::EDGE)->getTableName();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
	std::string const landmaskTableName = context->getEpgParameters().getValue(TARGET_COUNTRY_TABLE).toString();

//	context->getDataBaseManager().addColumn(edgeTableName, "is_out_of_country", "boolean", "false");
//	context->getDataBaseManager().refreshFeatureStore(edgeTableName, idName, geomName);
	std::ostringstream ss;
	//ss << "UPDATE " << edgeTableName << " SET is_out_of_country = true  "
	ss << "DELETE FROM " << edgeTableName
		<< " WHERE "
		//<<" is_linked_to_cl is false AND "
		<< countryCodeName << " ='" << countryCC << "'"
		<< " AND NOT ST_Intersects( (" 
		<< "SELECT ST_Union(ARRAY(SELECT " << geomName << " FROM " << landmaskTableName << " WHERE " << countryCodeName << " ='"<< countryCC <<"'))"
		<< "), "<< geomName <<")"
		;

	context->getDataBaseManager().getConnection()->update(ss.str());
	_logger->log(epg::log::TITLE, "[ END CLEAN EDGES OUT OF COUNTRY " + countryCC + " ] : " + epg::tools::TimeTools::getTime());
}


void app::calcul::connectFeatGenerationOp::cleanAntennasOutOfCountry(std::string countryCC
)
{
	_logger->log(epg::log::TITLE, "[ BEGIN CLEAN ANTENNAS OUT OF COUNTRY " + countryCC + " ] : " + epg::tools::TimeTools::getTime());
	epg::Context* context = epg::ContextS::getInstance();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const edgeTableName = _fsEdge->getTableName();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
	std::string const landmaskTableName = context->getEpgParameters().getValue(TARGET_COUNTRY_TABLE).toString();

	context->getDataBaseManager().addColumn(edgeTableName, "is_intersected_border", "boolean", "false");
	context->getDataBaseManager().addColumn(edgeTableName, "is_antenna_out_of_country", "boolean", "false");
	context->getDataBaseManager().refreshFeatureStore(edgeTableName, idName, geomName);

	std::ostringstream ssconditionIntersectedBorder;
	ssconditionIntersectedBorder << countryCodeName << " ='" << countryCC << "'" 
		<< "AND ST_Intersects(( SELECT ST_Union(ARRAY(SELECT " << geomName << " FROM " << _fsBoundary->getTableName()
		//<< "AND ST_Intersects(( SELECT ST_Union(ARRAY(SELECT ST_SetSRID(ST_GeomFromText('" << geomName << "'),3035)) FROM " << _fsBoundary->getTableName()
		<< " WHERE " << countryCodeName<< " like '%" << countryCC + "%' and "+ countryCodeName + " like '%#%'"
		<<" and boundary_type = 'international_boundary')))," << geomName << ")";
	context->getDataBaseManager().getConnection()->update("UPDATE " + edgeTableName + " SET is_intersected_border = true where " + ssconditionIntersectedBorder.str());

	std::ostringstream ssReqEdgesExtremOutOfCountry;
	ssReqEdgesExtremOutOfCountry << countryCodeName << " ='" << countryCC << "'"
		<< " AND NOT ST_Intersects( ("
		<< "SELECT ST_Union(ARRAY(SELECT " << geomName << " FROM " << landmaskTableName << " WHERE " << countryCodeName << " ='" << countryCC << "'))"
		<< "), " << geomName << ")";

	ign::feature::FeatureFilter filterEdgesExtremOutOfCountry(ssReqEdgesExtremOutOfCountry.str());
	ign::feature::FeatureIteratorPtr it = _fsEdge->getFeatures(filterEdgesExtremOutOfCountry);

	std::set<std::string> sEdgesIdAntennasOutOfCountry;

	int numFeatures = context->getDataBaseManager().numFeatures(*_fsEdge, filterEdgesExtremOutOfCountry);
	boost::progress_display display(numFeatures, std::cout, "[ CLEAN ANTENNAS OUT OF COUNTRY ]\n");
	_logger->log(epg::log::DEBUG, "numFeatures " + ign::data::Integer(numFeatures).toString());
	_logger->log(epg::log::DEBUG, "req: " + ign::data::String(ssReqEdgesExtremOutOfCountry.str()).toString());


	while (it->hasNext()) {
		++display;
		ign::feature::Feature edge = it->next();
		ign::geometry::LineString lsEdge = edge.getGeometry().asLineString();
		//debug
		std::string idE = edge.getId();

		if (sEdgesIdAntennasOutOfCountry.find(edge.getId()) != sEdgesIdAntennasOutOfCountry.end())//edge deja dans une antenne
			continue;

		//verifier que une des extremites est un dangle
		ign::feature::FeatureFilter filterArroundNdStart(idName + " <> '" + edge.getId() + "' and "+ countryCodeName + " ='" +countryCC +"'");
		filterArroundNdStart.setExtent(lsEdge.startPoint().getEnvelope().expandBy(0.01));
		ign::feature::FeatureIteratorPtr itDangleStart = context->getFeatureStore(epg::EDGE)->getFeatures(filterArroundNdStart);
		ign::feature::FeatureFilter filterArroundNdEnd(idName + " <> '" + edge.getId() + "' and " + countryCodeName + " ='" + countryCC + "'");
		filterArroundNdEnd.setExtent(lsEdge.endPoint().getEnvelope().expandBy(0.01));
		ign::feature::FeatureIteratorPtr itDangleEnd= context->getFeatureStore(epg::EDGE)->getFeatures(filterArroundNdEnd);
		if (itDangleStart->hasNext() && itDangleEnd->hasNext()) {//n'est pas un dangle
			//verifier la dist = 0 ?
			continue;
		}
		if (!itDangleStart->hasNext() && !itDangleEnd->hasNext()) {
			sEdgesIdAntennasOutOfCountry.insert(edge.getId());
			continue;
		}

		ign::geometry::Point ptNext,ptCurr;
		ign::feature::Feature edgeNext, edgeCurr;
		edgeCurr = edge;
		if (itDangleStart->hasNext()) 
			ptCurr = lsEdge.startPoint();
		else 
			ptCurr = lsEdge.endPoint();
		
		bool hasNextEdgeInAntennas = true;
		while (hasNextEdgeInAntennas) {
			sEdgesIdAntennasOutOfCountry.insert(edgeCurr.getId());
			hasNextEdgeInAntennas = isNextEdgeInAntennas(edgeCurr, ptCurr, edgeNext,ptNext);
			ptCurr = ptNext;
			edgeCurr = edgeNext;
		}
		//recup tte l'antenne jusqu'a l'intersection de la frontiere (non incluse)
	}

	context->getDataBaseManager().getConnection()->update("ALTER TABLE " + edgeTableName + " DROP COLUMN IF EXISTS is_intersected_border");
	context->getDataBaseManager().refreshFeatureStore(edgeTableName, idName, geomName);

	for (std::set<std::string>::iterator sit = sEdgesIdAntennasOutOfCountry.begin(); sit != sEdgesIdAntennasOutOfCountry.end(); ++sit) {
		//_fsEdge->deleteFeature(*sit);
		ign::feature::Feature fEdgeTomodif;
		_fsEdge->getFeatureById(*sit, fEdgeTomodif);
		fEdgeTomodif.setAttribute("is_antenna_out_of_country", ign::data::Boolean(true));
		_fsEdge->modifyFeature(fEdgeTomodif);
	}
	_logger->log(epg::log::INFO, "Nb d'edges from antennas out of country " + ign::data::Integer(sEdgesIdAntennasOutOfCountry.size()).toString());
	_logger->log(epg::log::TITLE, "[ END CLEAN ANTENNAS OUT OF COUNTRY " + countryCC + " ] : " + epg::tools::TimeTools::getTime());
}


bool app::calcul::connectFeatGenerationOp::isNextEdgeInAntennas(ign::feature::Feature& fEdgeCurr, ign::geometry::Point& ptCurr, ign::feature::Feature&  edgeNext, ign::geometry::Point& ptNext)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
	std::string countryCodeCurr = fEdgeCurr.getAttribute(countryCodeName).toString();


	if (fEdgeCurr.getGeometry().asLineString().startPoint() == ptCurr)
		ptNext = fEdgeCurr.getGeometry().asLineString().endPoint();
	else
		ptNext = fEdgeCurr.getGeometry().asLineString().startPoint();

	ign::feature::FeatureFilter filterArroundNdNext(idName + " <> '" + fEdgeCurr.getId() + "' and " + countryCodeName + " ='" + countryCodeCurr + "'");
	filterArroundNdNext.setExtent(ptNext.getEnvelope().expandBy(0.01));
	ign::feature::FeatureIteratorPtr itNextEdge= context->getFeatureStore(epg::EDGE)->getFeatures(filterArroundNdNext);
	
	if (!itNextEdge->hasNext()) //fin de l'antenne par un cul de sac
		return false;
	edgeNext = itNextEdge->next();
	if (itNextEdge->hasNext()) //fin de l'antenne par un noeud de valence strictement sup a 2
		return false;
	if (edgeNext.getAttribute("is_intersected_border").toBoolean())//fin de l'antenne par intersection de la frontière
		return false;
	
	return true;
}


void app::calcul::connectFeatGenerationOp::updateGeomCL(std::string countryCodeDouble, double snapOnVertexBorder)
{
	_logger->log(epg::log::TITLE, "[ BEGIN UPDATE GEOM CL " + countryCodeDouble + " ] : " + epg::tools::TimeTools::getTime());
	epg::Context* context = epg::ContextS::getInstance();
	params::ThemeParameters* themeParameters = params::ThemeParametersS::getInstance();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const edgeTableName = _fsEdge->getTableName();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
	std::string const linkedFeatIdName = context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();
	//std::string const natIdName = themeParameters->getValue(NATIONAL_IDENTIFIER).toString();

	std::vector<std::string> vCountriesCodeName;
	epg::tools::StringTools::Split(countryCodeDouble, "#", vCountriesCodeName);

	if(vCountriesCodeName.size() != 2)
		_logger->log(epg::log::WARN, "Attention, le countryCode " + countryCodeDouble + " n'a pas deux country" );

	std::string countryCode1 = vCountriesCodeName[0];
	std::string countryCode2 = vCountriesCodeName[1];

	ign::geometry::MultiPolygon mPolyCountry1, mPolyCountry2;
	getGeomCountry(countryCode1, mPolyCountry1);
	getGeomCountry(countryCode2, mPolyCountry2);

	ign::feature::FeatureIteratorPtr it = _fsCL->getFeatures(ign::feature::FeatureFilter());
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsCL, ign::feature::FeatureFilter());
	boost::progress_display display(numFeatures, std::cout, "[  UPDATE GEOM CL ]\n");

	while (it->hasNext()) {
		++display;
		ign::feature::Feature fCL = it->next();
		ign::geometry::LineString lsCLCurr = fCL.getGeometry().asLineString();
		lsCLCurr.setFillZ(0);
		ign::geometry::LineString lsCLUpdated;

		std::vector<std::string> vEdgeslinked;
		epg::tools::StringTools::Split(fCL.getAttribute(linkedFeatIdName).toString(), "#", vEdgeslinked);
		std::string idEdgLinked1 = vEdgeslinked[0];
		std::string idEdgLinked2 = vEdgeslinked[1];	
		ign::feature::Feature fEdg1, fEdg2;

		_fsEdge->getFeatureById(idEdgLinked1, fEdg1);
		_fsEdge->getFeatureById(idEdgLinked2, fEdg2);

		if (fEdg1.getId().empty() || fEdg2.getId().empty()) //si on ne trouve pas l'un des troncons liés
			continue;

		ign::geometry::LineString lsEdg1 = fEdg1.getGeometry().asLineString();
		ign::geometry::LineString lsEdg2 = fEdg2.getGeometry().asLineString();

		//on coupe les edges au niveau de la projection des extremites des CLs sur ces edges, pour ne prendre que la portion d'edge que l'on apparie à la CL
		ign::geometry::Point startLsProj1 = epg::tools::geometry::project(lsEdg1, lsCLCurr.startPoint(), snapOnVertexBorder);
		startLsProj1.setZ(0);
		ign::geometry::Point startLsProj2 = epg::tools::geometry::project(lsEdg2, lsCLCurr.startPoint(), snapOnVertexBorder);
		startLsProj2.setZ(0);
		ign::geometry::Point endLsProj1 = epg::tools::geometry::project(lsEdg1, lsCLCurr.endPoint(), snapOnVertexBorder);
		endLsProj1.setZ(0);
		ign::geometry::Point endLsProj2 = epg::tools::geometry::project(lsEdg2, lsCLCurr.endPoint(), snapOnVertexBorder);
		endLsProj2.setZ(0);

		epg::tools::geometry::LineStringSplitter lsSplitter1(lsEdg1);
		lsSplitter1.addCuttingGeometry(startLsProj1);	
		lsSplitter1.addCuttingGeometry(endLsProj1);
		std::vector<ign::geometry::LineString> vLsCandidate1 = lsSplitter1.getSubLineStrings();
		for (size_t i = 0; vLsCandidate1.size(); ++i) {
			ign::geometry::LineString lsCandidate = vLsCandidate1[i];
			if ((lsCandidate.startPoint().egal2d(startLsProj1) && lsCandidate.endPoint().egal2d(endLsProj1))
				|| (lsCandidate.startPoint().egal2d(endLsProj1) && lsCandidate.endPoint().egal2d(startLsProj1))) {
				lsEdg1 = lsCandidate;
				lsEdg1.startPoint().setZ(0);
				lsEdg1.endPoint().setZ(0);
				break;
			}
		}

		epg::tools::geometry::LineStringSplitter lsSplitter2(lsEdg2);
		lsSplitter2.addCuttingGeometry(startLsProj2);
		lsSplitter2.addCuttingGeometry(endLsProj2);
		std::vector<ign::geometry::LineString> vLsCandidate2 = lsSplitter2.getSubLineStrings();
		for (size_t i = 0; vLsCandidate2.size(); ++i) {
			ign::geometry::LineString lsCandidate = vLsCandidate2[i];
			if ((lsCandidate.startPoint().egal2d(startLsProj2) && lsCandidate.endPoint().egal2d(endLsProj2))
				|| (lsCandidate.startPoint().egal2d(endLsProj2) && lsCandidate.endPoint().egal2d(startLsProj2))) {
				lsEdg2 = lsCandidate;
				lsEdg2.startPoint().setZ(0);
				lsEdg2.endPoint().setZ(0);
				break;
			}
		}

		//on s'assure que les ls1 et ls2 sont dans le même sens
		if (!lsEdg1.startPoint().egal2d(startLsProj1) )
			lsEdg1 = lsEdg1.reverse();
		if (!lsEdg2.startPoint().egal2d(startLsProj2))
			lsEdg2 = lsEdg2.reverse();

		//si les 2 edges sont dans le même pays, on prend la geom de la portion de l'edge du pays
		bool isLs1InCountry1 = lsEdg1.intersects(mPolyCountry1);
		bool isLs2InCountry1 = lsEdg2.intersects(mPolyCountry1);
		bool isLs1InCountry2 = lsEdg1.intersects(mPolyCountry2);
		bool isLs2InCountry2 = lsEdg2.intersects(mPolyCountry2);
		if (isLs1InCountry1 && !isLs1InCountry2 && isLs2InCountry1 && !isLs2InCountry2)
			getGeomCL(lsCLUpdated, lsEdg1, lsCLCurr.startPoint(), lsCLCurr.endPoint(), snapOnVertexBorder);
		else if (isLs1InCountry2 && !isLs1InCountry1 && isLs2InCountry2 && !isLs2InCountry1)
			getGeomCL(lsCLUpdated, lsEdg2, lsCLCurr.startPoint(), lsCLCurr.endPoint(), snapOnVertexBorder);
		/*else if ((lsEdg1.intersects(mPolyCountry1) && !lsEdg1.intersects(mPolyCountry2)
			&& lsEdg2.intersects(mPolyCountry2) && !lsEdg2.intersects(mPolyCountry1))
			|| (lsEdg1.intersects(mPolyCountry2) && !lsEdg1.intersects(mPolyCountry1)
				&& lsEdg2.intersects(mPolyCountry1) && !lsEdg2.intersects(mPolyCountry2)))
			lsCLUpdated = lsCLCurr;*/
		else {
			std::set<double> sAbsCurv;
			ign::geometry::tools::LengthIndexedLineString lsIndex1(lsEdg1);
			ign::geometry::tools::LengthIndexedLineString lsIndex2(lsEdg2);
			for (size_t i = 0; i < lsEdg1.numPoints()-1; ++i) {
				double abscurv = lsIndex1.getPointLocation(i)/lsEdg1.length();
				//ign::geometry::Point pt1 = lsIndex1.locateAlong(abscurv*lsEdg1.length());
				sAbsCurv.insert(abscurv);
			}
			for (size_t i = 0; i < lsEdg2.numPoints()-1; ++i) {
				double abscurv = lsIndex2.getPointLocation(i)/lsEdg2.length();
				sAbsCurv.insert(abscurv);
			}

			for (std::set<double>::iterator sit = sAbsCurv.begin(); sit != sAbsCurv.end(); ++sit) {
				ign::geometry::MultiPoint multiPt;
				double test = *sit*lsEdg1.length();
				ign::geometry::Point pt1 = lsIndex1.locateAlong(*sit*lsEdg1.length());
				ign::geometry::Point pt2 = lsIndex2.locateAlong(*sit*lsEdg2.length());
				multiPt.addGeometry(pt1);
				multiPt.addGeometry(pt2);
				ign::geometry::Point ptLsCentroid = multiPt.getCentroid();
				//ign::geometry::Point ptLsCentroid = multiPt.asMultiPoint()..getCentroid();
				bool hasPtDistMin = false;
				if (sit != sAbsCurv.begin()) {
					if (lsCLUpdated.endPoint().distance(ptLsCentroid) < 0)
						hasPtDistMin = true;
				}
				if(!hasPtDistMin)
					lsCLUpdated.addPoint(ptLsCentroid);
			}
			ign::geometry::MultiPoint multiPtEnd;
			multiPtEnd.addGeometry(endLsProj1);
			multiPtEnd.addGeometry(endLsProj2);
			lsCLUpdated.addPoint(multiPtEnd.getCentroid());
			/*
			if (startLsProj1.intersects(mPolyCountry1) && startLsProj2.intersects(mPolyCountry1))
				lsCLUpdated.addPoint(startLsProj1);
			else if (startLsProj1.intersects(mPolyCountry2) && startLsProj2.intersects(mPolyCountry2))
				lsCLUpdated.addPoint(startLsProj2);
			else
				lsCLUpdated.addPoint(lsCLCurr.startPoint());

			std::map<double,ign::geometry::Point> mAbsCurvLsPt;
			for (size_t i = 1; i < lsEdg1.numPoints() - 1; ++i) {
				ign::geometry::Point ptIntermCurr = lsEdg1.pointN(i);
				if (ptIntermCurr.intersects(mPolyCountry1)) {
					std::pair< int, double > pairAbsCurv = epg::tools::geometry::projectAlong(lsCLCurr, ptIntermCurr, snapOnVertexBorder);
					double absCurv = pairAbsCurv.first + pairAbsCurv.second;
					mAbsCurvLsPt[absCurv] = ptIntermCurr;
				}
			}
			for (size_t i = 1; i < lsEdg2.numPoints() - 1; ++i) {
				ign::geometry::Point ptIntermCurr = lsEdg2.pointN(i);
				if (ptIntermCurr.intersects(mPolyCountry2)) {
					std::pair< int, double > pairAbsCurv = epg::tools::geometry::projectAlong(lsCLCurr, ptIntermCurr, snapOnVertexBorder);
					double absCurv = pairAbsCurv.first + pairAbsCurv.second;
					mAbsCurvLsPt[absCurv] = ptIntermCurr;
				}
			}

			//parcourir map pour addPoint
			for (std::map<double, ign::geometry::Point>::iterator mit = mAbsCurvLsPt.begin();
				mit != mAbsCurvLsPt.end(); ++mit) {
				lsCLUpdated.addPoint(mit->second);
			}
			if (endLsProj1.intersects(mPolyCountry1) && endLsProj2.intersects(mPolyCountry1))
				lsCLUpdated.addPoint(endLsProj1);
			else if (endLsProj1.intersects(mPolyCountry2) && endLsProj2.intersects(mPolyCountry2))
				lsCLUpdated.addPoint(endLsProj2);
			else
				lsCLUpdated.addPoint(lsCLCurr.endPoint());
			*/

		}
		lsCLUpdated.clearZ();
		fCL.setGeometry(lsCLUpdated);
		_fsCL->modifyFeature(fCL);
		
	}

	_logger->log(epg::log::TITLE, "[ BEGIN UPDATE GEOM CL " + countryCodeDouble + " ] : " + epg::tools::TimeTools::getTime());

}
void app::calcul::connectFeatGenerationOp::deleteCLUnderThreshold(std::string countryCodeDouble)
{

	_logger->log(epg::log::TITLE, "[ BEGIN CLEAN CL UNDER THRESHOLD " + countryCodeDouble + " ] : " + epg::tools::TimeTools::getTime());

	epg::Context* context = epg::ContextS::getInstance();
	std::string const CLTableName = _fsCL->getTableName();
	std::string const geomName = context->getEpgParameters().getValue(GEOM).toString();
	std::string const idName = context->getEpgParameters().getValue(ID).toString();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();

	/*context->getDataBaseManager().addColumn(CLTableName, "delete_under_threshold", "boolean", "false");
	context->getDataBaseManager().refreshFeatureStore(CLTableName, idName, geomName);*/

	std::ostringstream ssconditionDeleteCLUnderThreshold;

	ssconditionDeleteCLUnderThreshold << " ST_LENGTH(" << geomName << ") < 10 AND "<<countryCodeName << " ='" << countryCodeDouble << "'";
		/*<< "AND NOT ST_Intersects((" << "SELECT ST_Union(ARRAY(SELECT " << geomName << " FROM " << CLTableName 
		<< " WHERE " << idName << "))"
		<< "), " << geomName << ")";*/
	std::set<std::string> sCLToDelete;
	ign::feature::FeatureFilter filterCLInf10m(ssconditionDeleteCLUnderThreshold.str());
	ign::feature::FeatureIteratorPtr it = _fsCL->getFeatures(filterCLInf10m);
	int numFeatures = context->getDataBaseManager().numFeatures(*_fsCL, filterCLInf10m);
	boost::progress_display display(numFeatures, std::cout, "[ CLEAN CL UNDER THRESHOLD ]\n");

	while (it->hasNext()) {
		++display;
		ign::feature::Feature fCL10m = it->next();
		ign::feature::FeatureFilter filterNeighbor(idName + " <> '" + fCL10m.getId()+"'");
		filterNeighbor.setExtent(fCL10m.getGeometry().getEnvelope());
		ign::feature::FeatureIteratorPtr itArround = _fsCL->getFeatures(filterNeighbor);
		bool hasNeithbor = false;
		while (itArround->hasNext()) {
			ign::feature::Feature fCLNeighbor = itArround->next();
			if (fCLNeighbor.getGeometry().distance(fCL10m.getGeometry()) < 0.1) {
				hasNeithbor = true;
				break;
			}
		}
		if(!hasNeithbor)
			sCLToDelete.insert(fCL10m.getId());
	}

	for (std::set<std::string>::iterator sit = sCLToDelete.begin(); sit != sCLToDelete.end(); ++sit) {
		_fsCL->deleteFeature(*sit);
		/*ign::feature::Feature fCLTomodif;
		_fsCL->getFeatureById(*sit, fCLTomodif);
		fCLTomodif.setAttribute("delete_under_threshold", ign::data::Boolean(true));
		_fsCL->modifyFeature(fCLTomodif);*/
	}

	//context->getDataBaseManager().getConnection()->update("DELETE FROM " + edgeTableName +" WHERE "+ ssconditionDeleteCLUnderThreshold.str());
	//context->getDataBaseManager().getConnection()->update("UPDATE " + edgeTableName + " SET delete_under_threshold = true where " + ssconditionDeleteCLUnderThreshold.str());
	_logger->log(epg::log::INFO, "Nb CL isolées inférieures a un seuil supprimées  " + ign::data::Integer(sCLToDelete.size()).toString());
	_logger->log(epg::log::TITLE, "[ END CLEAN CL UNDER THRESHOLD " + countryCodeDouble + " ] : " + epg::tools::TimeTools::getTime());

}

void app::calcul::connectFeatGenerationOp::getGeomCountry(std::string countryCodeSimple, ign::geometry::MultiPolygon& geomCountry)
{
	epg::Context* context = epg::ContextS::getInstance();
	std::string const countryCodeName = context->getEpgParameters().getValue(COUNTRY_CODE).toString();
	params::ThemeParameters* themeParameters = params::ThemeParametersS::getInstance();
	std::string const landCoverTypeName = themeParameters->getValue(LAND_COVER_TYPE).toString();
	std::string const landAreaValue = themeParameters->getValue(TYPE_LAND_AREA).toString();


	ign::feature::FeatureIteratorPtr itLandmask = _fsLandmask->getFeatures(ign::feature::FeatureFilter(landCoverTypeName + " = '" + landAreaValue + "' AND " + countryCodeName + " = '" + countryCodeSimple + "'"));

	while (itLandmask->hasNext()) {
		ign::feature::Feature const& fLandmask = itLandmask->next();
		ign::geometry::MultiPolygon const& mp = fLandmask.getGeometry().asMultiPolygon();
		for (int i = 0; i < mp.numGeometries(); ++i) {
			geomCountry.addGeometry(mp.polygonN(i));
		}
	}

	
}