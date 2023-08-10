#ifndef _APP_CALCUL_CONNECTFEATGENERATION_H_
#define _APP_CALCUL_CONNECTFEATGENERATION_H_


#include <epg/log/EpgLogger.h>
#include <epg/log/ShapeLogger.h>
#include <epg/sql/tools/IdGeneratorFactory.h>

namespace app{
namespace calcul{

	class connectFeatGenerationOp {

	public:

		static void compute(std::string countryCode, bool verbose);


	private:


		connectFeatGenerationOp(std::string countryCode, bool verbose);
		~connectFeatGenerationOp();

		void _init(std::string countryCode, bool verbose);
		void _compute();
		void _computeOnDoubleCC(std::string countryCodeDouble);



		void getCountryCodeDoubleFromCC(std::string countryCC, std::set<std::string>& sCountryCodeDouble);

		void getCLfromBorder(ign::geometry::LineString & lsBorder, ign::geometry::GeometryPtr& buffBorder,  double distBuffer, double thresholdNoCL, double angleMax, double ratioInBuff, double snapOnVertexBorder);

		double getAngleEdgeWithBorder(ign::geometry::LineString& lsEdge, ign::geometry::LineString& lsBorder);

		void getGeomCL(ign::geometry::LineString& lsCL, ign::geometry::LineString& lsBorder, ign::geometry::Point ptStartToProject, ign::geometry::Point ptEndToProject, double snapOnVertexBorder);
	

		void addToUndershootNearBorder(ign::geometry::LineString & lsBorder, ign::geometry::GeometryPtr& buffBorder, double distBuffer);

		void getCPfromIntersectBorder(ign::geometry::LineString & lsBorder, double distCLIntersected);



		bool isEdgeIntersectedPtWithCL(ign::feature::Feature& fEdge, ign::geometry::Point ptIntersectBorder, double distCLIntersected);


		//void mergeCPNearBy(double distMergeCP, double snapOnVertexBorder);
		void snapCPNearBy(std::string countryCodeDouble,double distMergeCP, double snapOnVertexBorder);

		bool getNearestCP(ign::feature::Feature fCP,double distMergeCP, std::map < std::string, ign::feature::Feature>& mCPNear);

		void addFeatAttributeMergingOnBorder(ign::feature::Feature& featMergedAttr, ign::feature::Feature& featAttrToAdd, std::string separator);



		//void mergeCL(double distMergeCL, double snapOnVertexBorder);
		void mergeIntersectingCL(std::string countryCodeDouble, double distMergeCL, double snapOnVertexBorder);

		bool getCLToMerge(ign::feature::Feature fCL, double distMergeCL, std::map < std::string, ign::feature::Feature>& mCL2merge, std::set<std::string>& sCountryCode);


		void getBorderFromEdge(ign::geometry::LineString& lsEdgeOnBorder, ign::geometry::LineString& lsBorder);
		
	private:
		ign::feature::sql::FeatureStorePostgis* _fsEdge;
		ign::feature::sql::FeatureStorePostgis* _fsBoundary;
		ign::feature::sql::FeatureStorePostgis* _fsTmpCP;
		ign::feature::sql::FeatureStorePostgis* _fsTmpCL;

		//ign::feature::FeatureFilter _filterEdges2generateCF;
		std::string _reqFilterEdges2generateCF;

		epg::log::EpgLogger*                               _logger;
		//--
		epg::log::ShapeLogger*                             _shapeLogger;
		//--
		std::string                                        _countryCode;
		//--
		bool                                               _verbose;

		epg::sql::tools::IdGeneratorInterfacePtr _idGeneratorCP;
		epg::sql::tools::IdGeneratorInterfacePtr _idGeneratorCL;

		std::set<std::string> _sAttrNameToConcat;
		std::set<std::string> _sAttrNameW;
		

	};

}
}

#endif