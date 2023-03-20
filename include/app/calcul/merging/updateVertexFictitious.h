#ifndef _APP_CALCUL_MERGING_UPDATEVERTEXFICTITIOUS_H_
#define _APP_CALCUL_MERGING_UPDATEVERTEXFICTITIOUS_H_

#include<ign/feature/FeatureFilter.h>
#include<ign/feature/sql/FeatureStorePostgis.h>

namespace app {
	namespace calcul {
		namespace merging {

			/// \brief Met à jour le champ is_fictitious des noeuds du réseau
			void updateVertexFictitious(
				std::string edgeTableName,
				std::string vertexTableName,
				std::string interccTableName,
				std::string sqlProcessedCountries,
				std::vector< ign::feature::sql::FeatureStorePostgis* > vectPoiTables
			);


		}
	}
}

#endif