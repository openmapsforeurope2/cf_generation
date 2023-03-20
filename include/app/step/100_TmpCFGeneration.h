#ifndef _APP_STEP_TMP_CF_GENERATION_H_
#define _APP_STEP_TMP_CF_GENERATION_H_

#include <epg/step/StepBase.h>
#include <app/params/ThemeParameters.h>

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

	};

}
}

#endif