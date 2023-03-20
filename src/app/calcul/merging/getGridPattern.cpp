#include <app/calcul/merging/getGridPattern.h>

# include <ign/geometry/Point.h>
# include <ign/geometry/LineString.h>


namespace app {
namespace calcul {
namespace merging {

	/// \brief Decoupe les données en un quadrillage de patternIndex*patternIndex zones géographiques
	std::vector<ign::geometry::Envelope> getGridPattern(
		double xMin,
		double yMin,
		double xMax,
		double yMax,
		int patternIndex,
		double margin,
		bool verbose
	)
	{

		std::vector< ign::geometry::Envelope > vectGrid;

		double xInterval = std::round((xMax - xMin) / patternIndex);
		double yInterval = std::round((yMax - yMin) / patternIndex);

		double x1, y1, x2, y2;

		for (int i = 0; i < patternIndex; i++) 
		{
			for (int j = 0; j < patternIndex; j++)
			{
				x1 = xMin + i*xInterval - margin;
				y1 = yMin + j*yInterval - margin;
				ign::geometry::Point point1(x1, y1);

				x2 = xMin + (i + 1)*xInterval + margin;
				y2 = yMin + (j + 1)*yInterval + margin;
				ign::geometry::Point point2(x2, y2);

				ign::geometry::Envelope env;
				ign::geometry::LineString ls(point1, point2);
				ls.envelope(env);

				vectGrid.push_back(env);
			}
		}

		return vectGrid;
	}
}
}
}