#ifndef _APP_CALCUL_CONSISTENCY_LEVELCROSSINGPLANARIZE_H_
#define _APP_CALCUL_CONSISTENCY_LEVELCROSSINGPLANARIZE_H_

#include <ign/feature/sql/FeatureStorePostgis.h>

namespace app{
namespace calcul{
namespace consistency{

	/// \brief Decoupe le reseau routier et ferre quand il y a un level crossing ( passage a niveau )
	void levelCrossingPlanarize(
		ign::feature::sql::FeatureStorePostgis* fsLevelCC,
		ign::feature::sql::FeatureStorePostgis* fsRailwayL,
		ign::feature::sql::FeatureStorePostgis* fsRailwayNode,
		ign::feature::FeatureFilter filterProcessed
		);



}
}
}

#endif