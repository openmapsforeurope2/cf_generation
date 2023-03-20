#ifndef _APP_STEP_TOOLS_INITSTEPS_H_
#define _APP_STEP_TOOLS_INITSTEPS_H_

//EPG
#include <epg/step/StepSuite.h>
#include <epg/step/factoryNew.h>

//APP
#include <app/step/all.h>

/*
namespace epg{
namespace step{
	class StepSuite;
}
}*/

namespace app{
namespace step{
namespace tools{

	template<  typename StepSuiteType >
	void initSteps( StepSuiteType& stepSuite )
	{

		//stepSuite.addStep( epg::step::factoryNew< AllTrans >()  );

		stepSuite.addStep( epg::step::factoryNew< TmpCFGeneration >() );


	}


}
}
}

#endif