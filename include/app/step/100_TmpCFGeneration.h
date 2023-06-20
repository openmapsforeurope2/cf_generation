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

		void getCLfromBorder(ign::geometry::LineString & lsBorder, ign::geometry::GeometryPtr& buffBorder,  double distBuffer, double thresholdNoCL, double angleMax, double ratioInBuff, double snapOnVertexBorder);

		double getAngleEdgeWithBorder(ign::geometry::LineString& lsEdge, ign::geometry::LineString& lsBorder);

		void getGeomCL(ign::geometry::LineString& lsCL, ign::geometry::LineString& lsBorder, ign::geometry::Point ptStartToProject, ign::geometry::Point ptEndToProject, double snapOnVertexBorder);
	

		void addToUndershootNearBorder(ign::geometry::LineString & lsBorder, ign::geometry::GeometryPtr& buffBorder, double distBuffer);

		void getCPfromIntersectBorder(ign::geometry::LineString & lsBorder);



		bool isEdgeIntersectedPtWithCL(ign::feature::Feature& fEdge, ign::geometry::Point ptIntersectBorder);


		void mergeCPNearBy(double distMergeCP, double snapOnVertexBorder);

		bool getNearestCP(ign::feature::Feature fCP,double distMergeCP, std::map < std::string, ign::feature::Feature>& mCPNear);

		void addFeatAttributeMergingOnBorder(ign::feature::Feature& featMergedAttr, ign::feature::Feature& featAttrToAdd, std::string separator);



		void mergeCL(double distMergeCL, double snapOnVertexBorder);

		bool getCLToMerge(ign::feature::Feature fCL, double distMergeCL, std::map < std::string, ign::feature::Feature>& mCL2merge, std::set<std::string>& sCountryCode);


		void getBorderFromEdge(ign::geometry::LineString& lsEdgeOnBorder, ign::geometry::LineString& lsBorder);
		
	private:
		ign::feature::sql::FeatureStorePostgis* _fsBoundary;
		ign::feature::sql::FeatureStorePostgis* _fsTmpCP;
		ign::feature::sql::FeatureStorePostgis* _fsTmpCL;

		epg::sql::tools::IdGeneratorInterfacePtr _idGeneratorCP;
		epg::sql::tools::IdGeneratorInterfacePtr _idGeneratorCL;

		std::set<std::string> _sAttrNameToConcat;//set?

	};

}
}

#endif