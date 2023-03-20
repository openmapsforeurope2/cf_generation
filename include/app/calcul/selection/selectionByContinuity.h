#ifndef _APP_CALCUL_SELECTION_SELECTIONBYCONTINUITY_H_
#define _APP_CALCUL_SELECTION_SELECTIONBYCONTINUITY_H_


#include <set>

#include< ign/feature/FeatureFilter.h>

namespace app{
namespace calcul{
namespace selection{


	/*
	* Selectionne le reseau par continuite
	* @param[in] netType des antennes selectionneees
	* @param[in] set des netType utilises pour rattacher un poi
	* @param[in] netType minimum du reseau selectionne utilise comme target des itineraires recherches
	* @return[out] nombre de dangles
	*/
	int selectionByContinuity(ign::feature::FeatureFilter filterCompletNetwork, int netTypeDangles, std::set<int> & sNetTypePoi, int netTypeMain);



}
}
}

#endif