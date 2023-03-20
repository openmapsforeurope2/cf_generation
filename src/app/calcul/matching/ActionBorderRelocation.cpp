#include <app/calcul/matching/ActionBorderRelocation.h>


#include <epg/calcul/matching/borderRelocation/detail/action/deleteBeyondBorder.h>
#include <epg/calcul/matching/borderRelocation/detail/action/snapAlongBorder.h>





namespace app{
namespace calcul{
namespace matching{

		///
		///
		///
		ActionBorderRelocation::ActionBorderRelocation()
		{
		}

		///
		///
		///
		ActionBorderRelocation::~ActionBorderRelocation()
		{
		}

		///
		///
		///
		void ActionBorderRelocation::alongCoastAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face )
		{
			epg::calcul::matching::borderRelocation::detail::action::snapAlongBorder( *_graph, face ) ;
			
		}

		///
		///
		///
		void ActionBorderRelocation::overCoastAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face )
		{

			epg::calcul::matching::borderRelocation::detail::action::snapAlongBorder( *_graph, face ) ;

		}

		///
		///
		///
		void ActionBorderRelocation::bridgeCoastAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face )
		{
		}

		///
		///
		///
		void ActionBorderRelocation::alongBorderAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face )
		{
			epg::calcul::matching::borderRelocation::detail::action::snapAlongBorder( *_graph, face ) ;
		}

		///
		///
		///
		void ActionBorderRelocation::overBorderAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face )
		{
	
			log( epg::calcul::matching::borderRelocation::detail::action::deleteBeyondBorder( *_graph, face ) ) ;
		}

		///
		///
		///
		void ActionBorderRelocation::bridgeBorderAction( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face )
		{
			
			log( epg::calcul::matching::borderRelocation::detail::action::deleteBeyondBorder( *_graph, face ) ) ;
		}

		///
		///
		///
		void ActionBorderRelocation::bridgeBorderActionAlternate( epg::calcul::matching::borderRelocation::detail::graph::BorderRelocationGraph::face_descriptor face )
		{
			
			log( epg::calcul::matching::borderRelocation::detail::action::deleteBeyondBorder( *_graph, face ) ) ;
		}

}
}
}

