#ifndef _APP_CALCUL_CONSISTENCY_PLANARIZENETWORK_H_
#define _APP_CALCUL_CONSISTENCY_PLANARIZENETWORK_H_

#include <ign/feature/sql/FeatureStorePostgis.h>

namespace app{
namespace calcul{
namespace consistency{


	/*
	* Rend le reseau planaire en decoupant au niveau des franchissements filtres
	* @param[in] fsFranchissement feature store des objets au niveau desquels les edges seront decoupes
	* @param[in] filterProcessed filtre applique aux objets de fsFranchissement
	* @param[in] pseudoNodesAllowed; vrai -> creation de noeuds de degre 2 autorisee; faux -> decoupage seulement si creation d'un noeud de degre >= 3
	*/
	void planarizeNetwork(
		ign::feature::sql::FeatureStorePostgis* fsFranchissement,
		ign::feature::FeatureFilter filterProcessed
		);

}
}
}

#endif