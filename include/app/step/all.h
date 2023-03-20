#ifndef _APP_STEP_ALL_H_
#define _APP_STEP_ALL_H_

#include <app/step/100_TmpCFGeneration.h>


namespace app{
namespace step{

	class AllTrans : public epg::step::StepBase< params::TransParametersS > {

	public:

		/// \brief
		int getCode() { return 0; };

		/// \brief
		std::string getName() { return "allTrans"; };

		/// \brief
		void onCompute( bool );
		

	};

}
}



#endif