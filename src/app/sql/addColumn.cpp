#include <app/sql/addColumn.h>

#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>


namespace app{
namespace sql{


	void addColumn(std::string tableName, std::string columnName, std::string ValueTypeName, std::string defaultValue )
{
	epg::Context* context=epg::ContextS::getInstance();
	
	epg::log::EpgLogger* logger= epg::log::EpgLoggerS::getInstance();


	if( !context->getDataBaseManager().tableExists( tableName ) )

		logger->log(epg::log::ERROR,"la table "+tableName+" n'existe pas");


	else {
		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+tableName+" DROP COLUMN IF EXISTS "+columnName);


		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+tableName+" ADD COLUMN "+columnName+" "+ValueTypeName + " default "+defaultValue);
	}


}


	void addColumn(std::string tableName, std::string columnName, std::string ValueTypeName )
{
	epg::Context* context=epg::ContextS::getInstance();
	
	epg::log::EpgLogger* logger= epg::log::EpgLoggerS::getInstance();


	if( !context->getDataBaseManager().tableExists( tableName ) )

		logger->log(epg::log::ERROR,"la table "+tableName+" n'existe pas");


	else {

		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+tableName+" DROP COLUMN IF EXISTS  "+columnName);


		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+tableName+" ADD COLUMN "+columnName+" "+ValueTypeName );

	}


}


}
}

