#ifndef _APP_SQL_ADDCOLUMN_H_
#define _APP_SQL_ADDCOLUMN_H_

#include <string>

//SOCLE
#include <ign/data/Value.h>

namespace app{
namespace sql{


	/// \brief Ajoute une colonne a une table avec une valeur par defaut
	void addColumn(std::string tableName, std::string columnName, std::string ValueTypeName,std::string defaultValue  );

	/// \brief Ajoute une colonne a une table
	void addColumn(std::string tableName, std::string columnName, std::string ValueTypeName );


}
}

#endif