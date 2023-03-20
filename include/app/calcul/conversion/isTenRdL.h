#ifndef _APP_CONVERSION_ISTEN_H_
#define _APP_CONVERSION_ISTEN_H_

#include <ign/feature/sql/FeatureStorePostgis.h>

namespace app{
namespace calcul{
namespace conversion{

	/// \brief compute edges with a ten=value 1 for the edge's attribute ten using the previous version of erm
	void isTenRdL(ign::feature::sql::FeatureStorePostgis* fsEdgeOld);

}
}
}

#endif