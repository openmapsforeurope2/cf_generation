#ifndef _APP_CONVERSION_ISTENRWL_H_
#define _APP_CONVERSION_ISTENRWL_H_

#include <ign/feature/sql/FeatureStorePostgis.h>

namespace app{
namespace calcul{
namespace conversion{

	/// \brief compute edges with a ten=value 1 for the edge's attribute ten using the previous version of erm
	void isTenRwL(ign::feature::sql::FeatureStorePostgis* fsRwLOld, ign::feature::sql::FeatureStorePostgis * fsRailRdCOld, ign::feature::sql::FeatureStorePostgis* fsConnectingPoint, ign::feature::sql::FeatureStorePostgis* fsRailRdC);

}
}
}

#endif