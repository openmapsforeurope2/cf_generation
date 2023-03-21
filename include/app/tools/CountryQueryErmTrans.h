#ifndef _APP_TOOLS_COUNTRYQUERYERMTRANS_H_
#define _APP_TOOLS_COUNTRYQUERYERMTRANS_H_

#include <epg/io/query/CountryQueryBase.h>

namespace app{
namespace tools{

	enum QUERY_TAG_ERM_TRANS 
	{
		CONDITION_TERRITOIRE,
		SELECTION_SQL_RAILRDC,
		SELECTION_SQL_ROADL_MAIN,
		SELECTION_SQL_ROADL_LINKABLE,
		SELECTION_SQL_ROADL_IMPORTANCE
	};

	std::string toString( QUERY_TAG_ERM_TRANS tag );
	

	/**
	* Give all needed informations and parameters to process selection on the associated country
	*/
	class CountryQueryErmTrans : public epg::io::query::CountryQueryBase< QUERY_TAG_ERM_TRANS > {

	public:

		/// \brief Constructor
		CountryQueryErmTrans( ign::data::Object const& countryQuery );

		/// \brief check queries
		void check()const;

	};

}
}


#endif