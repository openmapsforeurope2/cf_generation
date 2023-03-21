#include <app/tools/CountryQueryErmTrans.h>

#include <epg/io/query/CountryQueryBase.h>

namespace app{
namespace tools{

	///
	///
	///
	std::string toString( QUERY_TAG_ERM_TRANS tag )
	{
		static std::map< QUERY_TAG_ERM_TRANS, std::string > mTags;
		if( mTags.empty() )
		{
			mTags.insert(std::make_pair(CONDITION_TERRITOIRE, "CONDITION_TERRITOIRE"));
			mTags.insert(std::make_pair(SELECTION_SQL_RAILRDC, "SELECTION_SQL_RAILRDC"));
			mTags.insert(std::make_pair(SELECTION_SQL_ROADL_MAIN, "SELECTION_SQL_ROADL_MAIN"));
			mTags.insert(std::make_pair(SELECTION_SQL_RAILRDC, "SELECTION_SQL_ROADL_LINKABLE")); 
			mTags.insert(std::make_pair(SELECTION_SQL_ROADL_IMPORTANCE, "SELECTION_SQL_ROADL_IMPORTANCE"));

						
		}
		return mTags[ tag ];
	}

	///
	///
	///
	CountryQueryErmTrans::CountryQueryErmTrans( ign::data::Object const& countryQuery )
		: epg::io::query::CountryQueryBase< QUERY_TAG_ERM_TRANS >( countryQuery )
	{
	}

	///
	///
	///
	void CountryQueryErmTrans::check()const
	{
		checkParameter(SELECTION_SQL_RAILRDC, epg::io::query::SQL);
		checkParameter(CONDITION_TERRITOIRE, epg::io::query::SQL );
		checkParameter(SELECTION_SQL_ROADL_MAIN, epg::io::query::SQL);
		checkParameter(SELECTION_SQL_ROADL_LINKABLE, epg::io::query::SQL);
		checkParameter(SELECTION_SQL_ROADL_IMPORTANCE, epg::io::query::SQL);
	}
}
}
