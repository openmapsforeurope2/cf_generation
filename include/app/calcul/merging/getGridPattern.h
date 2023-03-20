#ifndef _APP_CALCUL_MERGING_GETGRIDPATTERN_H_
#define _APP_CALCUL_MERGING_GETGRIDPATTERN_H_

#include<string>
#include<vector>
#include <ign/geometry/Envelope.h>

namespace app {
	namespace calcul {
		namespace merging {

			/// \brief Decoupe les données en un quadrillage de patternIndex*patternIndex  zones géographiques
			std::vector<ign::geometry::Envelope> getGridPattern(
				double xMin,
				double yMin,
				double xMax,
				double yMax,
				int patternIndex,
				double margin,
				bool verbose = false
			);


		}
	}
}

#endif