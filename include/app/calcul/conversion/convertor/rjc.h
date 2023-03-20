#ifndef _APP_CALCUL_CONVERSION_CONVERTOR_RJC_H_
#define _APP_CALCUL_CONVERSION_CONVERTOR_RJC_H_

#include <epg/calcul/conversion/Convertor.h>

#include <ign/feature/Feature.h>

//#include <epg/graph/EpgGraph.h>
//#include <epg/graph/concept/EpgGraphSpecializations.h>

namespace app{
namespace calcul{
namespace conversion{
namespace convertor{

    class rjc : public epg::calcul::conversion::Convertor
    {

    public:


		rjc( std::string const& targetAttributeName,
             epg::sql::SqlType::Type const& targetType
			 //epg::graph::EpgGraph & graph
            ):
            epg::calcul::conversion::Convertor( targetAttributeName, targetType )//,
			//_graph(graph)
            {
            }


        virtual void convert( ign::feature::Feature const& featureSource, ign::feature::Feature & featureTarget )const
        {
			/*
			epg::Context* context = epg::ContextS::getInstance();
			std::string const& codeName=context->getEpgParameters().getValue(EDGE_CODE).toString();

            ign::data::Variant targetValue ;


			IGN_ASSERT(_graph.getVertexById(featureSource.getId()).first);

			epg::graph::EpgGraph::vertex_descriptor vInterCC=_graph.getVertexById(featureSource.getId()).second;
			

			size_t nb_autoroute=0;

			size_t degree=_graph.degree(vInterCC);
	

			std::vector<epg::graph::EpgGraph::edge_descriptor> vEdgeInc=_graph.incidentEdges(vInterCC);

			std::set<std::string> sNumAutoroutier;
			for(size_t i=0;i<vEdgeInc.size();++i){

				ign::feature::Feature fEdgeInc;

				epg::graph::concept::feature(_graph[vEdgeInc[i]],fEdgeInc);

				IGN_ASSERT( fEdgeInc.hasAttribute( "vocation" ) ) ;

				if(fEdgeInc.getAttribute( "vocation" ).toString() == "Type autoroutier" ) {
					++nb_autoroute;
					sNumAutoroutier.insert( fEdgeInc.getAttribute( codeName ).toString() );
				}
			}


			if ( nb_autoroute == degree )
				targetValue = ign::data::Integer( 1 );
			else if ( sNumAutoroutier.size() == 1 )
				targetValue = ign::data::Integer( 2 );
			else if ( nb_autoroute == 0 )
				targetValue = ign::data::Integer( 0 );
			else 
				targetValue = ign::data::Integer( 3 );


			featureTarget.setAttribute( targetAttribute.getName(), targetValue  );

			*/

			IGN_ASSERT(featureSource.hasAttribute("rjc"));

			ign::data::Integer rjcValue = featureSource.getAttribute("rjc").toInteger();

			featureTarget.setAttribute(targetAttribute.getName(), rjcValue);


        }

//	private: 

//	epg::graph::EpgGraph &  _graph;

    };


}
}
}
}


#endif
