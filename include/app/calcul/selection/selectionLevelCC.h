#ifndef _APP_CALCUL_SELECTION_SELECTIONLEVELCC_H_
#define _APP_CALCUL_SELECTION_SELECTIONLEVELCC_H_


#include<string>
#include <ign/feature/FeatureFilter.h>

namespace app{
namespace calcul{
namespace selection{

	/// \brief
	void selectionLevelCC( 
		std::string levelCCTableName, 
		std::string edgeRailwayTableName, 
		std::string edgeRoadTableName,
		ign::feature::FeatureFilter filterProcessed
		) ;

}
}
}

#endif