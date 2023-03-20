#ifndef _APP_CALCUL_SELECTION_SELECTIONRESTC_H_
#define _APP_CALCUL_SELECTION_SELECTIONRESTC_H_

#include <string>

//Socle
#include <ign/feature/FeatureFilter.h>

namespace app{
namespace calcul{
namespace selection{

	/// \brief Selectionne les objets de la classe RestC
	void selectionRestC( std::string  const & restCTableName, ign::feature::FeatureFilter filterProcessed, double linkThreshold );



}
}
}

#endif