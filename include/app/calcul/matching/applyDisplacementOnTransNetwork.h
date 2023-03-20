#ifndef _APP_CALCUL_MATCHING_APPLYDISPLACEMENTONTRANSNETWORK_H_
#define _APP_CALCUL_MATCHING_APPLYDISPLACEMENTONTRANSNETWORK_H_

#include <ign/feature/sql/FeatureStorePostgis.h>

#include <epg/sql/tools/UidGenerator.h>

namespace app{
namespace calcul{
namespace matching{

	class ApplyDisplacementOnTransNetwork {

	public:
		///\brief
		ApplyDisplacementOnTransNetwork(
			ign::feature::sql::FeatureStorePostgis& fsRoad,
			ign::feature::sql::FeatureStorePostgis& fsRoadNode,
			ign::feature::sql::FeatureStorePostgis& fsRailway,
			ign::feature::sql::FeatureStorePostgis& fsRailwayNode
		);

		///\brief
		~ApplyDisplacementOnTransNetwork();

	/// \brief Planarise le reseau en coupant les edges la ou il y a des franchissements du reseau
	void compute(
		std::string countryCode,
		std::map< std::string, ign::math::Vec2d > const& mRefDisplacements,
		bool verbose
		);


	private:
		//--
		ign::feature::sql::FeatureStorePostgis&		_fsRoad;
		//--
		ign::feature::sql::FeatureStorePostgis&		_fsRoadNode;
		//--
		ign::feature::sql::FeatureStorePostgis&		_fsRailway;
		//--
		ign::feature::sql::FeatureStorePostgis&		_fsRailwayNode;
		//--
		std::set<std::string>						_sVisitedEdges;
		//--
		std::set<std::string>						_sVisitedVertices;

		epg::sql::tools::UidGenerator*				_uidGeneratorRoadNode;

		epg::sql::tools::UidGenerator*				_uidGeneratorRailwayNode;

		std::map<std::string, std::string>			_mModifiedMergedVertex;


	private:
		//--
		void _updateVertex( ign::feature::Feature& vertexToUpdate );
		//--
		//void _updatetEdge();


	};


}
}
}

#endif