#include <app/calcul/selection/selectionByContinuity.h>

//EPG
#include <epg/graph/EpgGraph.h>
#include <epg/graph/concept/EpgGraphSpecializations.h>
#include <epg/calcul/selection/getDanglingNodes.h>
#include <epg/calcul/selection/selectionForDangles.h>
#include <epg/utils/updateVerticesNetType.h>


namespace app{
namespace calcul{
namespace selection{

int selectionByContinuity(ign::feature::FeatureFilter filterCompletNetwork, int netTypeDangles, std::set<int> & sNetTypePoi, int netTypeMain)
{

		epg::Context* context=epg::ContextS::getInstance();
		epg::params::EpgParameters & params=context->getEpgParameters();

		std::set< std::pair< std::string, std::string> > sDangles;
		std::map< std::string, epg::calcul::selection::Antenna>  mAntennas;

		ign::feature::sql::FeatureStorePostgis* fsVertex = context->getFeatureStore(epg::VERTEX);


		// load graphe du reseau selectionne
		{
			epg::graph::EpgGraph graphSelected;
			ign::feature::FeatureFilter filterSelected(params.getValue(NET_TYPE).toString()+" > 9 ");		    
			filterSelected.addAttribute(params.getValue(EDGE_CODE).toString());
			graphSelected.load(filterSelected);

			// detection des dangles
			epg::calcul::selection::getDanglingNodes(graphSelected, mAntennas, sDangles, sNetTypePoi);
		}

		{

			// load graphe du reseau complet
			epg::graph::EpgGraph graphLocalAll;
			ign::feature::FeatureFilter filter = filterCompletNetwork;
			filter.addAttribute(params.getValue(EDGE_CODE).toString());

			//graphLocalAll.load(filter);

			int thresholMinLengthAntennaToContinue = 50;
			int rateDistAntenna = 5;
			int thresholMinLengthdAntennaToKeep = 1600;
			int distMinFromBorder = 300;
			int distOnBorder = 10;

			ign::feature::FeatureFilter filterUpdateVertices("true");

			// Parcours du set sDangles pour créer des graphes plus petits

			typename std::set<std::pair<std::string, std::string > >::const_iterator sitDangles;
			for (sitDangles = sDangles.begin(); sitDangles != sDangles.end(); ++sitDangles)
			{
				std::string idDanglingNode = sitDangles->first;
				ign::feature::Feature fDanglingNode;
				fsVertex->getFeatureById(idDanglingNode, fDanglingNode);

				ign::geometry::Envelope envDanglingNode;
				fDanglingNode.getGeometry().envelope(envDanglingNode);
				envDanglingNode.expandBy(5000);

				filter.setExtent(envDanglingNode);

				graphLocalAll.load(filter);

				std::set<std::pair<std::string, std::string >> sStart;
				std::map< std::string, epg::calcul::selection::Antenna>  mAntennaStart;

				sStart.insert(std::make_pair(sitDangles->first, sitDangles->second));
				mAntennaStart.insert(std::make_pair(sitDangles->first, mAntennas[sitDangles->first]));

				// calcul des itineraires de continuite
				epg::calcul::selection::selectionForDangles(
					graphLocalAll,
					thresholMinLengthAntennaToContinue,
					rateDistAntenna,
					thresholMinLengthdAntennaToKeep,
					distMinFromBorder,
					distOnBorder,
					mAntennaStart,
					sStart,
					netTypeMain,
					10000,
					netTypeDangles,
					filterUpdateVertices,
					true
				);
			}
		}
		
		epg::utils::updateVerticesNetType();
		return mAntennas.size();

}



}
}
}