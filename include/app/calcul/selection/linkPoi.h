#ifndef _APP_CALCUL_SELECTION_LINKPOI_H_
#define _APP_CALCUL_SELECTION_LINKPOI_H_

#include<string>
#include<set>

namespace app{
namespace calcul{
namespace selection{

	/// \brief Rattache les poi a un objet (troncon ou noeud)
	void linkPoi(
		std::string poiTable,
		std::string poiId,
		std::string poiGeom, 
		double linkThreshold, 
		std::set< std::pair< std::string/*idVertexStart*/, std::string/*idEdgeStart*/ > > & sStartIti,
		int netTypeMainPoi, //netType of edges from the main network used to link a poi
		bool verbose = false
		);


}
}
}

#endif