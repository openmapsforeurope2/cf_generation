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
		//shapeLogger->addShape("CPUndershoot", epg::log::ShapeLogger::POINT);


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
		_fsBoundary = _context.getDataBaseManager().getFeatureStore(boundaryTableName, idName, geomName);
		ign::feature::FeatureIteratorPtr itBoundary = _fsBoundary->getFeatures(ign::feature::FeatureFilter(countryCodeName + " = 'be#fr'"));

		_fsTmpCP = _context.getDataBaseManager().getFeatureStore(tmpCpTableName, idName, geomName);
		_fsTmpCL = _context.getDataBaseManager().getFeatureStore(tmpClTableName, idName, geomName);

		// id generator
		_idGeneratorCP = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsTmpCP, "CONNECTINGPOINT"));
		_idGeneratorCL = epg::sql::tools::IdGeneratorInterfacePtr(epg::sql::tools::IdGeneratorFactory::getNew(*_fsTmpCL, "CONNECTINGLINE"));


		while (itBoundary->hasNext())
		{
			ign::feature::Feature fBoundary = itBoundary->next();
			ign::geometry::LineString lsBoundary = fBoundary.getGeometry().asLineString();


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
			getCLfromBorder(lsBoundary, buffBorder, distBuffer, thresholdNoCL, angleMaxBorder, ratioInBuff, snapOnVertexBorder);


			getCPfromIntersectBorder(lsBoundary);

			addToUndershootNearBorder(lsBoundary, buffBorder, distBuffer);

			//si CL se superpose -> ?

			//CL et CP en fonction des autres pays?

			//limiter les CL si passage proche frontière juste sur une section de l'edge, repart et pas de CL de l'autre pays

			//si CL trop courte suppr? inf à 10m?

			//fusion des CP (surtout si diff pays) proche (5m?)

			//ajout de CP si pas d'intersection mais si route proche (1m?5m? taille du buffer des CL?), en direction de la frontiere? en fonction de la topo, si lié à un CL..?
			//ex tr 87f8eb7c-391c-4c7a-ad00-58560f812d24 ou 134f5cf7-a961-4187-9417-6f0d0c38984c ?
			//ex: cf53daf6-2486-47d0-8e74-fdca28d1670c / 4e96d959-bc8c-4e8d-a8cb-0aab4b5feb63 ou ddbf46ac-0db7-4197-af6f-d1d55676b09a

			//TODO -> mutualisation des CL/CP pour les garder, si existe dans les  2 pays..?

			//ajouter des CP aux extremites des CLs (surtout si pas de CP déjà créé et si un bout du edge de la CL continue hors frontière)
			//ex:8768ef03-90f4-4996-a800-016c04796352 ou 91a58d0d-b3a6-4184-bf5c-f65b31a40398/9740a593-d695-4926-816b-2a8784a7579d

			//CP 2 routes en une?

			//si CP sur une CL -> snapper le CP sur le bout de la CL ou raccourcir la CL jusqu'au CP (= mettre en coherence CP/CL)
			//rapport à la dist? ou a un rapport à la dist totale de la CL?

			//si qu'un pays (pas de CL ni CP de l'autre pays) -> longueure sous un seuil on vire, compare dist tr/CL



			///CL si 2 pays
			///CP fusion si plusieurs proches (+ que deux eventuellement)
			//
			//
			double distMergeCP = 5;
			mergeCPNearBy(distMergeCP, 0);

		}

		shapeLogger->closeShape("getCLfromBorder_buffers");
		//shapeLogger->closeShape("CPUndershoot");//
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

		//DEBUG
		/*if (fEdge.getId() == "dbb6d870-4781-43e8-860e-bbbbba68bb6a")
			bool bStop = true;*/
	
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
			isPtADangle = false;
			break;
		}
		if (!isPtADangle)
			continue;

		ign::geometry::Point projPt;
		std::vector< ign::geometry::Point > vPtIntersect;
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
		fCF.setAttribute("w_step", ign::data::String("1"));
		_fsTmpCP->createFeature(fCF, idCP);
		//shapeLogger->writeFeature("CPUndershoot", fCF);
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
	ign::feature::FeatureFilter filterIntersectCL ("w_national_identifier = '" + idLinkedEdge +"'");
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
	std::string condNotAlreadyNearestCP;
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

