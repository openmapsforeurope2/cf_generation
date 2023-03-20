#ifndef _APP_CALCUL_SELECTION_SELECTIONINTERCC_H_
#define _APP_CALCUL_SELECTION_SELECTIONINTERCC_H_

#include <string>
#include <ign/feature/FeatureFilter.h>

namespace app{
namespace calcul{
namespace selection{

	/// \brief Selectionne les objets de la classe InterCC
	void selectionInterCC( std::string const &  interCCTableName, ign::feature::FeatureFilter filterProcessed, double linkThreshold );



}
}
}

#endif