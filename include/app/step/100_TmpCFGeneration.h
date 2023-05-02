#ifndef _APP_STEP_TMP_CF_GENERATION_H_
#define _APP_STEP_TMP_CF_GENERATION_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>


#include <epg/sql/tools/IdGeneratorFactory.h>

namespace app{
namespace step{

	class TmpCFGeneration : public epg::step::StepBase< params::TransParametersS > {

	public:

		/// \brief
		int getCode() { return 100; };

		/// \brief
		std::string getName() { return "TmpCFGeneration"; };

		/// \brief
		void onCompute( bool );

		/// \brief
		void init();


	private:

		void getCLfromBorder(ign::feature::sql::FeatureStorePostgis* fsTmpCL, epg::sql::tools::IdGeneratorInterfacePtr idGeneratorCL, ign::geometry::LineString & lsBorder, ign::geometry::GeometryPtr& buffBorder,  double distBuffer, double thresholdNoCL, double angleMax, double ratioInBuff, double snapOnVertexBorder);

		double getAngleEdgeWithBorder(ign::geometry::LineString& lsEdge, ign::geometry::LineString& lsBorder);

		void getGeomCL(ign::geometry::LineString& lsCL, ign::geometry::LineString& lsBorder, ign::geometry::Point ptStartToProject, ign::geometry::Point ptEndToProject, double snapOnVertexBorder);
	};

}
}

#endif