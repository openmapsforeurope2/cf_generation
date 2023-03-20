#include <app/calcul/selection/selectionInterCC.h>

// BOOST
#include <boost/progress.hpp>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/graph/EpgGraph.h>
#include <epg/graph/concept/EpgGraphSpecializations.h>
#include <epg/tools/FilterTools.h>
#include <epg/sql/tools/numFeatures.h>

////TRANS_ERM
#include <app/params/ThemeParameters.h>


namespace app{
namespace calcul{
namespace selection{


	void selectionInterCC(std::string const &  interCCTableName, ign::feature::FeatureFilter filterProcessed, double linkThreshold )
	{
		
		epg::log::ScopeLogger log( "app::calcul::selection::selectionInterCC" );
		epg::Context* context = epg::ContextS::getInstance();

		epg::params::EpgParameters & params = context->getEpgParameters();
		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		std::string const netTypeName = params.getValue( NET_TYPE ).toString();
		std::string const idName = params.getValue( ID ).toString();
		std::string const geomName = params.getValue( GEOM ).toString();
		std::string const& linkedFeatidName = params.getValue(LINKED_FEATURE_ID).toString();

		std::string interccTypeName = "rjc";

		if (!context->getDataBaseManager().columnExists(interCCTableName, netTypeName))
			context->getDataBaseManager().getConnection()->update("ALTER TABLE " + interCCTableName + " ADD COLUMN " + netTypeName + " integer default 0");

		if (!context->getDataBaseManager().columnExists(interCCTableName, linkedFeatidName))
			context->getDataBaseManager().getConnection()->update("ALTER TABLE " + interCCTableName + " ADD COLUMN " + linkedFeatidName + " character varying");

		if (!context->getDataBaseManager().columnExists(interCCTableName, interccTypeName))
			context->getDataBaseManager().getConnection()->update("ALTER TABLE " + interCCTableName + " ADD COLUMN " + interccTypeName + " integer default -32768");

		ign::feature::sql::FeatureStorePostgis* fsEdge = context->getFeatureStore(epg::EDGE);
		ign::feature::sql::FeatureStorePostgis* fsVertex = context->getFeatureStore(epg::VERTEX);
		ign::feature::sql::FeatureStorePostgis* fsIntercC = context->getDataBaseManager().getFeatureStore(interCCTableName, idName, geomName);
		ign::feature::FeatureIteratorPtr itIntercC = fsIntercC->getFeatures(filterProcessed);

		int nb_iter = epg::sql::tools::numFeatures(*fsIntercC, filterProcessed);
		log.log(epg::log::INFO, "Nombre d'objets à lier : " + ign::data::Integer(nb_iter).toString());

		boost::progress_display display(nb_iter, std::cout, "[link table " + interCCTableName + " with a threshold of " + ign::data::Double(linkThreshold).toString() + " running % complete ]\n");


		//************************************************************************************************/
		//* Deselection des echangeurs connectes a moins 3 troncons et recuperation des toponymes        */
		//************************************************************************************************/
		
		while (itIntercC->hasNext())
		{
			++display;

			ign::feature::Feature fIntercC = itIntercC->next();
			std::string idIntercC = fIntercC.getId();

			std::string idLinkedVertex = "";
			double distMin = linkThreshold + 1;
			int scoreMax = -1;
			int newRjc;

			//Recuperation de la zone de recherche des edges en fonction du seuil
			ign::geometry::Envelope envPoi;
			fIntercC.getGeometry().envelope(envPoi);
			envPoi.expandBy(linkThreshold);

			ign::feature::FeatureFilter filterSelectedVertex(netTypeName + " >= 10");
			filterSelectedVertex.setExtent(envPoi);
			ign::feature::FeatureIteratorPtr vit = fsVertex->getFeatures(filterSelectedVertex);

			// Recherche des vertices selectionnes a une distance inferieur au seuil
			while (vit->hasNext())
			{
				ign::feature::Feature fVertex = vit->next();
				std::string idVertex = fVertex.getId();

				// Si le vertex est déjà utilisé, on continue
				if (fVertex.getAttribute(linkedFeatidName).toString() != "")
					continue;

				int scoreVertex = 0;

				// Recherche des edges connectés au vertex courant
				ign::feature::FeatureFilter filterConnectedEdges(netTypeName + " >= 10 AND (" + params.getValue(EDGE_SOURCE).toString() + " = '" + idVertex + "' OR " + params.getValue(EDGE_TARGET).toString() + " = '" + idVertex + "')");
				int nbConnectedEdges = epg::sql::tools::numFeatures(*fsEdge, filterConnectedEdges);

				if (nbConnectedEdges < 3)
					continue;

				// Parcours des edges sélectionnés pour déterminer le score du vertex
				ign::feature::FeatureIteratorPtr eit = fsEdge->getFeatures(filterConnectedEdges);
				std::set< std::string > sNature;
				std::set< std::string > sGroundPosition;
				std::set< std::string > sRoadNumber;
				std::set< std::string > sMotorwayNumber;
				int nbMotorway = 0;

				while (eit->hasNext())
				{
					ign::feature::Feature fConnectedEdge = eit->next();
					sNature.insert(fConnectedEdge.getAttribute("nature").toString());
					sGroundPosition.insert(fConnectedEdge.getAttribute("position_par_rapport_au_sol").toString());

					std::string roadNumber = fConnectedEdge.getAttribute("cpx_numero").toString();

					if (roadNumber != "NULL" && roadNumber != "null")
						sRoadNumber.insert(fConnectedEdge.getAttribute("cpx_numero").toString());

					if (fConnectedEdge.getAttribute("nature").toString() == "Type autoroutier")
					{
						nbMotorway++;
						if (roadNumber != "NULL" && roadNumber != "null")
							sMotorwayNumber.insert(roadNumber);
					}
				}

				// Si les edges ont tous la même position par rapport au sol, on passe
				if (sGroundPosition.size() == 1)
					continue;

				// Si aucun edge n'est de nature 'Type autoroutier' ou 'Route à 2 chaussées', on passe
				if (sNature.find("Type autoroutier") == sNature.end() && sNature.find("Route à 2 chaussées") == sNature.end() && sNature.find("Route Ã  2 chaussÃ©es") == sNature.end())
					continue;

				// Calcul du score du vertex (plus il est élevé, plus le vertex est pertinent)
				if (sNature.find("Type autoroutier") != sNature.end())
					scoreVertex += 10;

				if (nbMotorway >= 3)
					scoreVertex += 5;

				if (sRoadNumber.size() >= 2)
					scoreVertex += 5;

				if (sRoadNumber.size() == 1)
					scoreVertex += 1;

				if (scoreVertex < scoreMax)
					continue;

				double dist = fIntercC.getGeometry().asPoint().distance(fVertex.getGeometry().asPoint());

				if (scoreVertex > scoreMax || (scoreVertex == scoreMax && dist < distMin))
				{
					scoreMax = scoreVertex;
					distMin = dist;
					idLinkedVertex = idVertex;

					// Calcul du RJC du vertex
					if (sNature.size() == 1 && nbMotorway >= 1) // Echangeur entre des autoroutes
						newRjc = 1;
					else
					{
						if (sNature.size() > 1 && sMotorwayNumber.size() >= 2) 
							newRjc = 3;
						else
							newRjc = 2;
					}
				}
			}

			if (idLinkedVertex != "")
			{
				// Récupération du feature correspondant au vertex retenu
				ign::feature::Feature fVertexSelected;
				fsVertex->getFeatureById(idLinkedVertex, fVertexSelected);

				// Mise à jour des attributs et de la géométrie de l'objet IntercC
				fIntercC.setAttribute(linkedFeatidName, ign::data::String(idLinkedVertex));
				fIntercC.setAttribute(netTypeName, ign::data::Integer(10));
				fIntercC.setAttribute(geomName, fVertexSelected.getGeometry().asPoint());
				fIntercC.setAttribute(interccTypeName, ign::data::Integer(newRjc));
				
				fsIntercC->modifyFeature(fIntercC);

				// Remplissage du linkedFeatidName du vertex retenu pour qu'il ne soit pas réutilisé
				fVertexSelected.setAttribute(linkedFeatidName, ign::data::String(idIntercC));
				fsVertex->modifyFeature(fVertexSelected);
			}
		}
	
		
		/*
		
		
		ign::feature::sql::FeatureStorePostgis* fscarrefourComplexe = context->getDataBaseManager().getFeatureStore( carrefourComplexeTableName, idName, geomName );

		epg::graph::EpgGraph graphSelect;

		ign::feature::FeatureFilter filterSelect = filterProcessed;
		epg::tools::FilterTools::addAndConditions( filterSelect, netTypeName + " > 9" );

		graphSelect.load( filterSelect );


		ign::feature::FeatureIteratorPtr itInterCC = fsInterCC->getFeatures( filterSelect );

		while ( itInterCC->hasNext() ) {

			ign::feature::Feature fInterCC = itInterCC->next();


			std::string idCarrefourComplexe = fInterCC.getAttribute( "liens_vers_carrefour_complexe" ).toString();

//DEBUG
//			log.log( epg::log::INFO, fInterCC.getId() + " - " + idCarrefourComplexe );

			// Deselection des echangeurs connectes a moins 3 troncons
			IGN_ASSERT( graphSelect.getVertexById( fInterCC.getId() ).first );


			epg::graph::EpgGraph::vertex_descriptor vInterCC = graphSelect.getVertexById( fInterCC.getId() ).second;


			if ( graphSelect.degree( vInterCC ) < 3 )

				epg::graph::concept::setAttribute( graphSelect[vInterCC], netTypeName, ign::data::Integer( 1 ) );


			else if ( !idCarrefourComplexe.empty() ) {// Recuperation des toponymes dans le cas ou l'intercc est selectionne

				ign::feature::Feature fCarrefourComplexe;

				fscarrefourComplexe->getFeatureById( idCarrefourComplexe, fCarrefourComplexe );


				//IGN_ASSERT( !fCarrefourComplexe.getId().empty() );
				if( fCarrefourComplexe.getId().empty() )
				{
					log.log( epg::log::ERROR, idCarrefourComplexe + " n'existe pas dans la table des carrefours complexes - toponyme non recupere pour l'intercC " + fInterCC.getId() + "." );
					continue;
				}


				if ( fCarrefourComplexe.getAttribute( "toponyme_denomination" ).toString() == "''" ) continue;

				fInterCC.setAttribute( "toponyme_denomination", fCarrefourComplexe.getAttribute( "toponyme_denomination" ) );

				fInterCC.setAttribute( "toponyme_specification", fCarrefourComplexe.getAttribute( "toponyme_specification" ) );

				fInterCC.setAttribute( "toponyme_article", fCarrefourComplexe.getAttribute( "toponyme_article" ) );

				fsInterCC->modifyFeature( fInterCC );

			}

		} */

	}


}
}
}
