//BOOST
#include <boost/program_options.hpp>

//SOCLE
#include <ign/Exception.h>

//EPG
#include <epg/Context.h>
#include <app/params/ThemeParameters.h>
#include <epg/log/EpgLogger.h>
#include <epg/tools/TimeTools.h>
#include <epg/SingletonT.h>
#include <epg/params/tools/loadParameters.h>
#include <epg/dialog/ConsoleOp.h>
#include <app/step/tools/initSteps.h>



namespace po = boost::program_options;

int main(int argc, char *argv[])
{

	epg::Context* context=epg::ContextS::getInstance();

	char **newargv = new  char*[argc+6]();
	for( int i = 0 ; i < argc ; ++i ) newargv[i] = argv[i];

	epg::step::StepSuite< app::params::TransParametersS > stepSuite;
	app::step::tools::initSteps( stepSuite );

	epg::dialog::ConsoleOp< app::params::TransParametersS > consoleOp( stepSuite );

	ign::geometry::PrecisionModel::SetDefaultPrecisionModel(ign::geometry::PrecisionModel(ign::geometry::PrecisionModel::FIXED, 1.0e5, 1.0e7) );

	return consoleOp.run( argc, newargv );


}

