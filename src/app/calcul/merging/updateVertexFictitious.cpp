#include <app/calcul/merging/updateVertexFictitious.h>

//EPG
#include <epg/Context.h>
#include <epg/log/EpgLogger.h>
#include <epg/log/ScopeLogger.h>
#include <epg/tools/FilterTools.h>
#include <epg/sql/tools/numFeatures.h>
#include <app/params/ThemeParameters.h>

#include <boost/progress.hpp>


namespace app {
namespace calcul {
namespace merging {

	/// \brief Decoupe les données en un quadrillage de patternIndex*patternIndex zones géographiques
	void updateVertexFictitious(
		std::string edgeTableName,
		std::string vertexTableName,
		std::string interccTableName,
		std::string sqlProcessedCountries,
		std::vector< ign::feature::sql::FeatureStorePostgis* > vectPoiTables
	)
	{

		epg::Context* context = epg::ContextS::getInstance();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();
		epg::params::EpgParameters & params = context->getEpgParameters();
//		epg::log::EpgLogger* logger = epg::log::EpgLoggerS::getInstance();
		epg::log::ScopeLogger scopeLogger("[ SET VERTEX FICTITIOUS ]");
		
		std::string const idName = params.getValue(ID).toString();
		std::string const geomName = params.getValue(GEOM).toString();
		std::string const netTypeName = params.getValue(NET_TYPE).toString();
		std::string const& vertexFictitiousName = params.getValue(VERTEX_FICTITIOUS).toString();
		std::string const& linkedFeatureIdName = params.getValue(LINKED_FEATURE_ID).toString();
		std::string const& edgeSource = params.getValue(EDGE_SOURCE).toString();
		std::string const& edgeTarget = params.getValue(EDGE_TARGET).toString();
		std::string const& pointToRoadVertexIdName = transParameter->getValue(POINT_TO_ROAD_VERTEX_ID).toString();

		ign::feature::sql::FeatureStorePostgis* fsEdge = context->getFeatureStore(epg::EDGE);
		ign::feature::sql::FeatureStorePostgis* fsVertex = context->getFeatureStore(epg::VERTEX);
		ign::feature::sql::FeatureStorePostgis* fsInterCC = context->getDataBaseManager().getFeatureStore(interccTableName, idName, geomName);

		ign::feature::FeatureFilter filterProcessedCountries(sqlProcessedCountries);

		//************************************************************************************************/
		//* Réinitialisation de vertexFictitiousName à true pour tous les objets                         */
		//************************************************************************************************/

		std::ostringstream ss;
		ss << "UPDATE " << vertexTableName << " SET " << vertexFictitiousName << " = true;";
		context->getDataBaseManager().getConnection()->update(ss.str());

		//************************************************************************************************/
		//* Cas des vertex de degré supérieur à 3									                     */
		//************************************************************************************************/

		// Création d'une nouvelle colonne vertex_degree pour stocker le degré des vertices
		std::string vertexDegreeName = "vertex_degree";
		context->getDataBaseManager().addColumn(vertexTableName, vertexDegreeName, "integer", "0");

		// Remplissage de vertexFictitiousName à False pour les vertices de degré >= 3
		std::ostringstream ssCreateJoinTable;
		ssCreateJoinTable << "DROP TABLE IF EXISTS _600_join_vertex;"
			<< "CREATE TABLE _600_join_vertex AS SELECT vertex_tb." << idName
			<< ", count(edge_tb." << idName << ") count_edge "
			<< "FROM " << vertexTableName << " AS vertex_tb "
			<< " JOIN " << edgeTableName << " AS edge_tb "
			<< " ON vertex_tb." << idName << " = edge_tb." << edgeSource << " OR vertex_tb." << idName << " = edge_tb." << edgeTarget
			<< " GROUP BY (vertex_tb." << idName << ");";
		context->getDataBaseManager().getConnection()->update(ssCreateJoinTable.str());

		std::ostringstream ssUpdateDegree;
		ssUpdateDegree << "UPDATE " << vertexTableName
			<< " SET " << vertexDegreeName << " = _600_join_vertex.count_edge "
			<< " FROM _600_join_vertex WHERE " << vertexTableName << "." << idName << " = _600_join_vertex." << idName << ";";
		context->getDataBaseManager().getConnection()->update(ssUpdateDegree.str());

		std::ostringstream ssUpdateFictitiousDegree;
		ssUpdateFictitiousDegree << "UPDATE " << vertexTableName
			<< " SET " << vertexFictitiousName << " = false "
			<< " WHERE " << vertexFictitiousName << " AND " << vertexDegreeName << " >= 3 ;";
		context->getDataBaseManager().getConnection()->update(ssUpdateFictitiousDegree.str());

		//************************************************************************************************/
		//* Cas des vertex superposés à des POI										                     */
		//************************************************************************************************/

		int countLinkedVertex = 0;

		std::vector< ign::feature::sql::FeatureStorePostgis* >::const_iterator vitPoiTable;
		for (vitPoiTable = vectPoiTables.begin(); vitPoiTable != vectPoiTables.end(); ++vitPoiTable)
		{
			ign::feature::sql::FeatureStorePostgis* fsPoi = *vitPoiTable;
			ign::feature::FeatureIteratorPtr itPoi = fsPoi->getFeatures(filterProcessedCountries);

			//patience
			int nb_iter = epg::sql::tools::numFeatures(*fsPoi, filterProcessedCountries);
			boost::progress_display display(nb_iter, std::cout, "[ Set vertex fictitious for table " + fsPoi->getTableName() + " % complete ]\n");

			while (itPoi->hasNext())
			{
				++display;

				ign::feature::Feature fPoi = itPoi->next();
				ign::geometry::Envelope envPoi;
				fPoi.getGeometry().asPoint().envelope(envPoi);

				ign::feature::FeatureFilter filterVertexLinkedToPoi("(" + sqlProcessedCountries + ") AND " + vertexFictitiousName);
				filterVertexLinkedToPoi.setExtent(envPoi.expandBy(1));

				ign::feature::FeatureIteratorPtr itVertexLinkedToPoi = fsVertex->getFeatures(filterVertexLinkedToPoi);
				while (itVertexLinkedToPoi->hasNext())
				{
					ign::feature::Feature fVertex = itVertexLinkedToPoi->next();
					fVertex.setAttribute(vertexFictitiousName, ign::data::Boolean(false));
					fVertex.setAttribute(linkedFeatureIdName, ign::data::String(fPoi.getId()));
					fsVertex->modifyFeature(fVertex);

					countLinkedVertex++;
				}
			}
		}



		//************************************************************************************************/
		//* Cas des vertex appartenant à des connecting lines						                     */
		//************************************************************************************************/

		std::ostringstream ssUpdateFictitiousCL;
		ssUpdateFictitiousCL << "UPDATE " << vertexTableName
			<< " SET " << vertexFictitiousName << " = false "
			<< " FROM " << edgeTableName << " WHERE " << edgeTableName << "." << netTypeName << " = 56"
			<< " AND (" << vertexTableName << "." << idName << " = " << edgeTableName << "." << edgeSource
			<< " OR " << vertexTableName << "." << idName << " = " << edgeTableName << "." << edgeTarget << ");";
		context->getDataBaseManager().getConnection()->update(ssUpdateFictitiousCL.str());

		//************************************************************************************************/
		//* Cas des vertex liés à des tronçons connectés à des IntercC					                 */
		//************************************************************************************************/

		ign::feature::FeatureIteratorPtr itIntercc = fsInterCC->getFeatures(filterProcessedCountries);
		context->refreshFeatureStore(epg::VERTEX);


		//patience
		int nb_iter = epg::sql::tools::numFeatures(*fsInterCC, filterProcessedCountries);
		boost::progress_display display(nb_iter, std::cout, "[ Set vertex fictitious for table " + interccTableName + " % complete ]\n");

		while (itIntercc->hasNext())
		{
			++display;
			
			// Rechercher les tronçons connectés à IntercC
			ign::feature::Feature fIntercc = itIntercc->next();			
			std::string idVertexIntercc = fIntercc.getAttribute(pointToRoadVertexIdName).toString();

			// Parcours des tronçons connectés à IntercC
			ign::feature::FeatureFilter filterEdgeConnectedToIntercc( edgeSource + " = '" + idVertexIntercc + "' OR " + edgeTarget + " = '" + idVertexIntercc + "'");

			ign::feature::FeatureIteratorPtr itEdgeConnected = fsEdge->getFeatures(filterEdgeConnectedToIntercc);
			while (itEdgeConnected->hasNext())
			{
				std::string idCurrentVertex = idVertexIntercc;
				//double totalLength = 0;
				std::set <int> sLLE;
				std::set < std::string > sVisitedEdges;
				std::set< std::string > sVisitedVertices;
				bool endVertexFound = false;

				sVisitedVertices.insert(idCurrentVertex);

				// On récupère le 2ème vertex du tronçon connecté
				ign::feature::Feature fCurrentEdge = itEdgeConnected->next();
				ign::feature::Feature fCurrentVertex, fNextVertex;

				// On parcourt les tronçons de degré 2 de proche en proche
				// Si on arrive à un vertex séparant des tronçons de LLE différents à moins de 50 m d'IntercC, on met is_fictitious à faux pour éviter 
				// que le LLE disparaisse dans mergingByLength (sinon IntercC se retrouve sur des tronçons de même niveau vertical)
				while (sLLE.size() < 2 && !endVertexFound)
				{					
					/*if (sVisitedEdges.size() >= 2 && totalLength >= 50)
					{
						endVertexFound = true;
						continue;
					}*/
					
					fsVertex->getFeatureById(idCurrentVertex, fCurrentVertex);
					
					std::string idCurrentEdge = fCurrentEdge.getId();
					std::string idNextVertex = fCurrentEdge.getAttribute(edgeSource).toString();

					if (idNextVertex == idCurrentVertex)
						idNextVertex = fCurrentEdge.getAttribute(edgeTarget).toString();
					
					if (sVisitedVertices.find(idNextVertex) != sVisitedVertices.end())
					{
						endVertexFound = true;
						continue;
					}
					sVisitedVertices.insert(idNextVertex);

					//totalLength += fCurrentEdge.getGeometry().asLineString().length();
					sLLE.insert(fCurrentEdge.getAttribute("lle").toInteger());

					// S'il n'est pas de degré 2 ou s'il est déjà non fictitious, on passe
					fsVertex->getFeatureById(idNextVertex, fNextVertex);

					if (fNextVertex.getAttribute(vertexDegreeName).toInteger() != 2 || !fNextVertex.getAttribute(vertexFictitiousName).toBoolean())
					{
						endVertexFound = true;
						continue;
					}

					// S'il est de degré 2, on compare les LLE des deux edges connectés
					// Parcours des tronçons connectés à IntercC
					ign::feature::FeatureFilter filterIncidentEdge(edgeSource + " = '" + idNextVertex + "' OR " + edgeTarget + " = '" + idNextVertex + "'");
					epg::tools::FilterTools::addAndConditions(filterIncidentEdge, idName + " != '" + idCurrentEdge + "'");
					ign::feature::FeatureIteratorPtr itIncidentEdge = fsEdge->getFeatures(filterIncidentEdge);

					fCurrentEdge = itIncidentEdge->next();		
					idCurrentVertex = idNextVertex;

					sVisitedEdges.insert(idCurrentEdge);					
				}

				// Si le dernier vertex rencontré sépare deux tronçons de LLE différents, on met is_fictitious à faux
				if (sLLE.size() == 2)
				{
					fCurrentVertex.setAttribute(vertexFictitiousName, ign::data::Boolean(false));
					fsVertex->modifyFeature(fCurrentVertex);
				}
			}

		}


	}
}
}
}