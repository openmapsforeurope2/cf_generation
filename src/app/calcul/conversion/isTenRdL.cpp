#include<app/calcul/conversion/isTenRdL.h>

// BOOST
#include <boost/progress.hpp>

//SOCLE
#include <ign/geometry/algorithm/BufferOpGeos.h>
#include <ign/graph/algorithm/DijkstraShortestPaths.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/graph/EpgGraph.h>
#include <epg/graph/concept/EpgGraphSpecializations.h>
#include <epg/sql/tools/numFeatures.h>
#include <epg/tools/geometry/LineStringSplitter.h>
#include <epg/tools/geometry/interpolate.h>
#include <epg/calcul/selection/getDanglingNodes.h>
#include <epg/calcul/selection/selectionForDangles.h>
#include <epg/utils/updateVerticesNetType.h>


//APP
#include <app/params/ThemeParameters.h>
#include <app/sql/addColumn.h>


namespace app{
namespace calcul{
namespace conversion{

	void isTenRdL(ign::feature::sql::FeatureStorePostgis* fsEdgeOld)
	{

		epg::log::ScopeLogger log("[ app::calcul::conversion::istenRdL ]") ;
		boost::timer timer;

		epg::Context* context=epg::ContextS::getInstance();

		epg::params::EpgParameters & params=context->getEpgParameters();

		app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();

		std::string edgeCodeName = params.getValue( EDGE_CODE ).toString();
		std::string linkedFeatureidName = params.getValue( LINKED_FEATURE_ID ).toString();
		std::string edgeSourceName = params.getValue( EDGE_SOURCE ).toString();
		std::string edgeTargetName = params.getValue( EDGE_TARGET ).toString();
		std::string netTypeNameDefault =params.getValue(NET_TYPE).toString();


		//ajout d'un attribut netType=isten
		sql::addColumn(params.getValue( EDGE_TABLE ).toString() , "isten", "integer","0"  );
		sql::addColumn(params.getValue( VERTEX_TABLE ).toString() , "isten", "integer","0"  );

		ign::feature::sql::FeatureStorePostgis * fsEdge = context->refreshFeatureStore( epg::EDGE );
		context->refreshFeatureStore( epg::VERTEX );

		
		params.setParameter(NET_TYPE,ign::data::String("isten"));

		//ign::feature::sql::FeatureStorePostgis * fsEdge= context->getFeatureStore( epg::EDGE );

		std::set<std::string> sEdgesOnTen;
		//std::set<std::string> sNodesOnTen;

		
		double distBuffer=10;

		double ratio = 0.7;


		ign::feature::FeatureFilter filterTen("ten=1" );

		ign::feature::FeatureIteratorPtr itTenOld = fsEdgeOld->getFeatures( filterTen ) ;

		int nb_iter=epg::sql::tools::numFeatures(*fsEdgeOld,filterTen);

		boost::progress_display display(nb_iter, std::cout, " Attribute TEN RoadL % complete ]\n") ;

		while ( itTenOld->hasNext() ) {
			++display;

			ign::feature::Feature fEdgeOld = itTenOld->next() ;

			ign::geometry::LineString lsOldTen = fEdgeOld.getGeometry().asLineString();

			ign::geometry::algorithm::BufferOpGeos buffOp;
			ign::geometry::GeometryPtr buffOldTen( buffOp.buffer(lsOldTen,distBuffer,0,ign::geometry::algorithm::BufferOpGeos::CAP_FLAT) );

			ign::feature::FeatureFilter filter;
			filter.setExtent(lsOldTen.getEnvelope());
			ign::feature::FeatureIteratorPtr itEdgeNew= fsEdge->getFeatures( filter ) ;

			while (itEdgeNew->hasNext() ) {
			
				ign::feature::Feature fEdgeCandidat=itEdgeNew->next();

				ign::geometry::LineString lsCandidat = fEdgeCandidat.getGeometry().asLineString();

				if (sEdgesOnTen.find(fEdgeCandidat.getId())!=sEdgesOnTen.end()) continue;

				epg::tools::geometry::LineStringSplitter splitterCandidat( lsCandidat ) ;

				splitterCandidat.addCuttingGeometry(*buffOldTen);

				std::vector< ign::geometry::LineString > vLsCandidat=splitterCandidat.getSubLineStrings();

				double lengthInBuff = 0;

				for( size_t i=0; i<vLsCandidat.size();++i) {
					int numSeg = static_cast<int>( std::floor( vLsCandidat[i].numSegments()/2. ) );
					ign::geometry::Point interiorPoint = epg::tools::geometry::interpolate( vLsCandidat[i], numSeg, 0.5 );
					if( buffOldTen->contains( interiorPoint ) ) lengthInBuff += vLsCandidat[i].length();
				}

				if ( lengthInBuff > ratio*lsCandidat.length() ) {

					sEdgesOnTen.insert( fEdgeCandidat.getId() ) ;

					IGN_ASSERT( fEdgeCandidat.hasAttribute("ten") );

					IGN_ASSERT( fEdgeCandidat.hasAttribute("isten") );

					fEdgeCandidat.setAttribute("ten", ign::data::Integer(1));

					fEdgeCandidat.setAttribute("isten", ign::data::Integer(10));

					fsEdge->modifyFeature( fEdgeCandidat ) ;

					/*sNodesOnTen.insert(fEdgeCandidat.getAttribute(edgeSourceName).toString());

					sNodesOnTen.insert(fEdgeCandidat.getAttribute(edgeTargetName).toString());*/

					continue;

				}

				
				ign::geometry::GeometryPtr buffCandidat( buffOp.buffer(lsCandidat,distBuffer,0,ign::geometry::algorithm::BufferOpGeos::CAP_FLAT) );

				epg::tools::geometry::LineStringSplitter splitterOldTen( lsOldTen ) ;

				splitterOldTen.addCuttingGeometry(*buffCandidat);

				std::vector< ign::geometry::LineString > vLsOldTen=splitterOldTen.getSubLineStrings();

				lengthInBuff = 0;

				for( size_t i=0; i<vLsOldTen.size();++i) {
					int numSeg = static_cast<int>( std::floor( vLsOldTen[i].numSegments()/2. ) );
					ign::geometry::Point interiorPoint = epg::tools::geometry::interpolate( vLsOldTen[i], numSeg, 0.5 );
					if( buffCandidat->contains( interiorPoint ) ) lengthInBuff += vLsOldTen[i].length();
				}

				if ( lengthInBuff > ratio*lsOldTen.length() ) {

					sEdgesOnTen.insert( fEdgeCandidat.getId() ) ;

					IGN_ASSERT( fEdgeCandidat.hasAttribute("ten") );

					IGN_ASSERT( fEdgeCandidat.hasAttribute("isten") );

					fEdgeCandidat.setAttribute("ten", ign::data::Integer(1));

					fEdgeCandidat.setAttribute("isten", ign::data::Integer(10));

					fsEdge->modifyFeature( fEdgeCandidat ) ;
					
					/*sNodesOnTen.insert(fEdgeCandidat.getAttribute(edgeSourceName).toString());

					sNodesOnTen.insert(fEdgeCandidat.getAttribute(edgeTargetName).toString());*/

				}

			}
			
		}


		int netTypeDanglesTen=40;


		//recuperation des dangles du reseau ten
		std::set< std::pair< std::string, std::string> > sDangles;

		std::map< std::string, epg::calcul::selection::Antenna>  mAntennas;

		epg::utils::updateVerticesNetType(ign::feature::FeatureFilter());
		
		//std::set<std::string> sDanglesOnTen;
		//creation du graphe
	
		epg::graph::EpgGraph graphTen;

		ign::feature::FeatureFilter filterTenWithAttributes=filterTen;
		filterTenWithAttributes.addAttribute(edgeCodeName);

		graphTen.load(filterTenWithAttributes);
		
		std::set<int> sNetTypePoi;
		epg::calcul::selection::getDanglingNodes(graphTen,mAntennas,sDangles,sNetTypePoi);

		for(std::set< std::pair< std::string, std::string> >::iterator sit = sDangles.begin(); sit != sDangles.end(); ++sit ) { 
			IGN_ASSERT( graphTen.getVertexById( sit->first ).first );
			epg::graph::EpgGraph::vertex_descriptor vDangle = graphTen.getVertexById( sit->first ).second;
			epg::graph::concept::setNetType(graphTen[vDangle],ign::data::Integer(2));
		}

		


			/*
			epg::graph::EpgGraph::vertex_iterator vit, vitEnd;
			graphTen.vertices(vit,vitEnd);
			while( vit!=vitEnd ) {
				if( graphTen.degree(*vit)==1 )
					sDanglesOnTen.insert( graphTen.getId(*vit) ) ;
				++vit;
			}
			log.log( epg::log::INFO, "nombre de dangles du TEN : " + ign::data::Integer( sDanglesOnTen.size() ).toString() );
			log.log( epg::log::INFO, "nombre de node target du TEN : " + ign::data::Integer( sNodesOnTen.size() ).toString() );
		}*/



		//creation du graphe
		//epg::graph::EpgGraph graph;

		itTenOld = fsEdgeOld->getFeatures( filterTen ) ;

		ign::feature::FeatureFilter filterEdgeGraph("ten!=1");

		while ( itTenOld->hasNext() ) {

			ign::feature::Feature fEdgeOld = itTenOld->next() ;

			filterEdgeGraph.setExtent( fEdgeOld.getGeometry().getEnvelope().expandBy(100) );
			ign::feature::FeatureIteratorPtr itEdgeGraph= fsEdge->getFeatures( filterEdgeGraph ) ;

			while ( itEdgeGraph->hasNext()) 
				graphTen.addFeatureEdge( itEdgeGraph->next() );
				//graph.addFeatureEdge( itEdgeGraph->next() );
		}

		log.log( epg::log::INFO, "num vertices graph: " + ign::data::Integer( graphTen.numVertices() ).toString() );
		log.log( epg::log::INFO, "num edges graph: " + ign::data::Integer( graphTen.numEdges() ).toString() );


		int thresholMinLengthAntennaToContinue=50;
		int rateDistAntenna=4;
		int thresholMinLengthdAntennaToKeep=50000;
		int distMinFromBorder=300;
		int distOnBorder=50;//10
		int netTypeTarget = 4;

		ign::feature::FeatureFilter filterUpdateVertices("false");

		// calcul des itineraires de continuite
		epg::calcul::selection::selectionForDangles(graphTen,thresholMinLengthAntennaToContinue,rateDistAntenna,thresholMinLengthdAntennaToKeep,distMinFromBorder,distOnBorder,mAntennas,sDangles,netTypeTarget,10000,netTypeDanglesTen,filterUpdateVertices,true);


		ign::feature::FeatureFilter filterIsTen("isten="+ign::data::Integer(netTypeDanglesTen).toString()+ " and ten != 1");

		ign::feature::FeatureIteratorPtr itIsTen = fsEdge->getFeatures( filterIsTen ) ;

		while( itIsTen->hasNext() ) {
			ign::feature::Feature fEdge = itIsTen->next();
			//if( sEdgesOnTen.find(fEdge.getId()) != sEdgesOnTen.end() ) continue;
			sEdgesOnTen.insert( fEdge.getId() ) ;
			IGN_ASSERT( fEdge.hasAttribute("ten") ) ;
			fEdge.setAttribute("ten",ign::data::Integer(1));
			fsEdge->modifyFeature(fEdge);
			
		}





		//continuite

		//creation du graphe
	/*	epg::graph::EpgGraph graph;
		itTenOld = fsEdgeOld->getFeatures( filterTen ) ;
		ign::feature::FeatureFilter filterEdgeGraph("ten!=1");
		while ( itTenOld->hasNext() ) {

			ign::feature::Feature fEdgeOld = itTenOld->next() ;

			filterEdgeGraph.setExtent( fEdgeOld.getGeometry().getEnvelope().expandBy(100) );
			ign::feature::FeatureIteratorPtr itEdgeGraph= fsEdge->getFeatures( filterEdgeGraph ) ;

			while ( itEdgeGraph->hasNext()) 
				graph.addFeatureEdge( itEdgeGraph->next() );
		}

		log.log( epg::log::INFO, "num vertices graph: " + ign::data::Integer( graph.numVertices() ).toString() );
		log.log( epg::log::INFO, "num edges graph: " + ign::data::Integer( graph.numEdges() ).toString() );*/


//DEBUG
/*
		log.log( epg::log::INFO, "BDCTRORO0000000020211313 est dans le graphe: " + ign::data::String( graph.getEdgeById("BDCTRORO0000000020211313").first ).toString() );
		std::string testNd;
		if( sDanglesOnTen.begin()!=sDanglesOnTen.end())
			testNd=" !=end";
		else 
			testNd=" ==end";
		log.log( epg::log::INFO, "BDCNDROU0000000021788838 est dans le sDanglesOnTen: " + ign::data::String( testNd ).toString() );
*/
//DEBUG

		//calcul d'itineraires d'un noeud du reseau TEN aux autres
		/*
		boost::progress_display display2(sDanglesOnTen.size(), std::cout, " Attribute TEN Road part 2/2 % complete ]\n") ;

		for( std::set<std::string>::iterator sit = sDanglesOnTen.begin(); sit != sDanglesOnTen.end(); ++sit ) {
			++display2;

			//noeud n'est pas dans le graphe du ten !=1
			if( ! graph.getVertexById(*sit).first ) continue;

			epg::graph::EpgGraph::vertex_descriptor vNodeOnTen = graph.getVertexById(*sit).second;
			
			ign::graph::algorithm::DijkstraShortestPaths<epg::graph::EpgGraph> dijkOnTen(graph,true);
			dijkOnTen.computePaths( vNodeOnTen ) ;

			epg::graph::EpgGraph::edges_path pathOnTen;
			
			double distMinOnTen = std::numeric_limits<double>::max() ;

			for( std::set<std::string>::iterator sitTarget=sNodesOnTen.begin();sitTarget!=sNodesOnTen.end();++sitTarget) {

				if( *sit==*sitTarget ) continue;

				if( ! graph.getVertexById(*sitTarget).first ) continue;
								
				epg::graph::EpgGraph::vertex_descriptor vNodeTarget = graph.getVertexById(*sitTarget).second;

				epg::graph::EpgGraph::edges_path path;

				double dist = dijkOnTen.getPath(vNodeOnTen,vNodeTarget,path);


				if( dist < distMinOnTen && dist != -1 ) {
					distMinOnTen = dist;
					pathOnTen = path;
				}

			}
			
			for(epg::graph::EpgGraph::edges_path_iterator epit=pathOnTen.begin();epit!=pathOnTen.end();++epit) {

				std::string idEdgeOnTen=graph.getId(epit->descriptor);

				if( sEdgesOnTen.find(idEdgeOnTen)!=sEdgesOnTen.end()) continue;

				sEdgesOnTen.insert(idEdgeOnTen);

				ign::feature::Feature fEdge;
				fsEdge->getFeatureById(idEdgeOnTen,fEdge);

				IGN_ASSERT( fEdge.hasAttribute("ten") ) ;

				fEdge.setAttribute("ten",ign::data::Integer(1));

				fsEdge->modifyFeature(fEdge);
			}

		}

		for( std::set<std::string>::iterator sit = sEdgesOnTen.begin(); sit != sEdgesOnTen.end(); ++sit ){
			ign::feature::Feature fEdge;
			fsEdge->getFeatureById(*sit,fEdge);
			IGN_ASSERT( fEdge.hasAttribute("ten") ) ;
			if(fEdge.getAttribute("ten").toInteger() == 1) continue;
			fEdge.setAttribute("ten",ign::data::Integer(1));
			fsEdge->modifyFeature(fEdge);
		}
		*/
	    

		params.setParameter(NET_TYPE,ign::data::String(netTypeNameDefault));

		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+params.getValue( EDGE_TABLE ).toString()+" DROP COLUMN IF EXISTS isten");
		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+params.getValue( VERTEX_TABLE ).toString()+" DROP COLUMN IF EXISTS isten");

		context->refreshFeatureStore( epg::EDGE );
		context->refreshFeatureStore( epg::VERTEX );


		log.log( epg::log::INFO, "nombre d arcs avec un ten=1 :" + ign::data::Integer( sEdgesOnTen.size() ).toString() );
		log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() ) ;



	}

}
}
}