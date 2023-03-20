#ifndef _APP_CALCUL_CONSISTENCY_RESTCPLANARIZE_H_
#define _APP_CALCUL_CONSISTENCY_RESTCPLANARIZE_H_

#include <ign/feature/sql/FeatureStorePostgis.h>

namespace app{
namespace calcul{
namespace consistency{

	/// \brief Decoupe le reseau routier et ferre quand il y a un level crossing ( passage a niveau )
	void restCMerge(
		ign::feature::sql::FeatureStorePostgis* fsRestC,
		ign::feature::FeatureFilter filterProcessed
		);


	void projOnRoadRestC(
		ign::feature::sql::FeatureStorePostgis* fsRestC, 
		ign::feature::FeatureFilter filterProcessed
		);



}
}
}

#endif