#ifndef _APP_CALCUL_MATCHING_ACTIONBORDERRELOCATION_H_
#define _APP_CALCUL_MATCHING_ACTIONBORDERRELOCATION_H_


#include <epg/calcul/matching/borderRelocation/detail/ActionInterface.h>


namespace app{
namespace calcul{
namespace matching{


	class ActionBorderRelocation : public epg::calcul::matching::borderRelocation::detail::ActionInterface {
		

	public:

		/// \brief
		ActionBorderRelocation( );

		/// \brief
		~ActionBorderRelocation();

		virtual void alongCoastAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face );

		virtual void overCoastAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face );

		virtual void bridgeCoastAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face );

		virtual void alongBorderAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face );

		virtual void overBorderAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face );

		virtual void bridgeBorderAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face );

		virtual void bridgeBorderActionAlternate( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face );


	};


}
}
}

#endif