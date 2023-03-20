//#include <app/calcul/contraite/Contrainte.h>
//
//#include <epg/graph/EpgGraph.h>
//#include <epg/graph/concept/EpgGraphSpecializations.h>
//#include <epg/Context.h>
//#include <ign/data/Integer.h>
//
//#include <boost/progress.hpp>
//#include <boost/timer.hpp>
//#include <epg/log/ScopeLogger.h>
//
//namespace app{
//namespace alcul{
//namespace contrainte{
//
//
//ContraintAuto::ContraintAuto()
//{
//    //_ptr_contraintERM=new Contrainte(* _ptr_dataERM->getServer());
//    //_ptr_contraintERM=new Contrainte(* _ptr_dataERM->getServer(),"tabl","id","geom");
//}
//
//ContraintAuto::~ContraintAuto()
//{
//    //delete _ptr_contraintERM;
//    //_ptr_contraintERM=0;
//}
//
//
//void ContraintAuto::getContraint_resOld(epg::graph::EpgGraph & graphOld)
//{
//    //recuperation des contraintes dans la map
//
//    //GraphGeom graphOld(dataOld);
//    ign::feature::FeatureFilter filtr_true("true");
//    //*(_ptr_dataERM->getFlux()->get_oFlux())<<"construction du graphe du reseau complet de l'annee N-1"<<std::endl;
//    //graphOld.constructionGraph(filtr_true);
//
//    ign::feature::sql::FeatureIteratorPostgis it_contrainte(fs_contrainte,filtr_true);
//
//    epg::log::ScopeLogger log("[GetContraint_resOld]");
//    boost::timer timer ;
//
//    int nb_iter = fs_contrainte.numFeatures();
//
//    boost::progress_display display(nb_iter, std::cout, "[GetContraint_resOld % complete ]\n") ;
//
//    while(it_contrainte.hasNext()){
//        ign::feature::Feature f_contrainte=it_contrainte.next();
//
//        std::string id_contrainte=f_contrainte.getId();
//        ign::geometry::LineString ls_containte=f_contrainte.getGeometry().asLineString();
//
//        _ptr_contraintERM->setContrainte(id_contrainte,ls_containte,graphOld);
//
//        if(_ptr_contraintERM->get_mapContrainte().find(id_contrainte)->second.size()<2)
//			log.write("pb contrainte n°: "+ign::data::String(f_contrainte.getId()));
//    ++display;
//    }
//
//    log.write ("execution time :" + ign::data::Integer( timer.elapsed() ).toString() ) ;
//
//    //graphOld.getGraph()->clear();
//
//   log.write("recuperation terminee des contraintes sur le reseau de l'annee N-1");
//}
//
//
///*
//void ContraintAuto::getContraint_resOld(epg::graph::EpgGraph & graphOld)
//{
//    ign::feature::sql::FeatureStorePostgis fs_contrainte(* _ptr_dataERM->getServer());
//    fs_contrainte.load(_ptr_dataERM->getTablContraint(),_ptr_dataERM->getAttrIdContraint(),_ptr_dataERM->getAttrGeomContraint());
//
//    getContraint_resOld(graphOld,fs_contrainte);
//}
//*/
//
//void ContraintAuto::appariNdC(epg::graph::EpgGraph & graphNew,double dist_max)
//{
//    //GraphGeom graphNew(*_ptr_dataERM);
//    //ign::feature::FeatureFilter filtr_true("true");
//
//    //*(_ptr_dataERM->getFlux()->get_oFlux())<<"construction du graphe du reseau complet de l'annee N"<<std::endl;
//    //graphNew.constructionGraph(filtr_true);
//
//    _ptr_contraintERM->apparie_map_ndContrainte(dist_max,graphNew);
//
//    //graphNew.getGraph()->clear();
//}
//
//
//void ContraintAuto::runItiContraint(int typContrainte,std::string typRes2contraint, std::string typRes2secondaire)
//{
//    epg::Context* context=epg::ContextS::getInstance();
//
//    epg::params::EpgParameters const& params=context->getEpgParameters();
//
//
////    ign::feature::sql::FeatureStorePostgis fsNotif(*_ptr_dataERM->getServer());
////    fsNotif.load(data::paramERM::t_notif_contrainte,"id","geom");
//
//    ign::feature::sql::FeatureStorePostgis fs_tr(*_ptr_dataERM->getServer());
//    fs_tr.load(_ptr_dataERM->getTablArc(),_ptr_dataERM->getAttrIdArc(),_ptr_dataERM->getAttrGeom());
//
//
//    epg::graph::EpgGraph graph2contraint(data);
//    ign::feature::FeatureFilter const filtr2contraint(typRes2contraint);
//    *(_ptr_dataERM->getFlux()->get_oFlux())<<"construction du graphe du reseau de contrainte"<<std::endl;
//    graph2contraint.load(filtr2contraint);
//
//    epg::graph::EpgGraph graphSecondaire(data);
//    ign::feature::FeatureFilter const filtrSecondaire(typRes2secondaire);
//    *(_ptr_dataERM->getFlux()->get_oFlux())<<"construction du graphe du reseau secondaire"<<std::endl;
//    graphSecondaire.load(filtrSecondaire);
//
//    //Itineraire iti(graph2contraint);
//
//    //calcul de l'itineraire de contrainte par contrainte
//    std::map<std::string,std::list<NdContrainte> > mapC_list_ndC=_ptr_contraintERM->get_mapContrainte();
//
//    epg::log::ScopeLogger log("[runItiContraint]");
//    boost::timer timer ;
//
//    int nb_iter = mapC_list_ndC.size();
//
//    boost::progress_display display(nb_iter, std::cout, "[runItiContraint Loading % complete ]\n") ;
//
//    for(std::map<std::string,std::list<NdContrainte> >::iterator it_idContrainte=mapC_list_ndC.begin();it_idContrainte!=mapC_list_ndC.end();++it_idContrainte){
//        std::string idContrainte=it_idContrainte->first;
//        //*(_ptr_dataERM->getFlux()->get_oFlux())<<"id contrainte: "<<idContrainte<<std::endl;
//
//        std::list<std::string> itiContrainte=_ptr_contraintERM->calculItiContraint(idContrainte,graph2contraint,graphSecondaire);
//        if (!itiContrainte.empty())
//        {
//            //bool b_valiIti;
//            //b_valiIti=Itineraire::addIti(fs_tr,fsIti,itiContrainte,typContrainte);
//
//            //if(b_valiIti){
//            //    Itineraire::saveIti(fs_tr,itiContrainte,typContrainte);
//
//            //    //ajout du lien de la contrainte et de son type
//            //    ign::feature::Feature fIti;
//            //    fsIti.getFeatureById(ign::data::Integer(fsIti.numFeatures()).toString(),fIti);
//            //    fIti.setAttribute("lien_contrainte",ign::data::String(idContrainte));
//            //    fIti.setAttribute("type_contrainte",ign::data::Integer(typContrainte));
//            //    fsIti.modifyFeature(fIti);
//            //}
//            //else
//            //    addNotifOp(fsNotif,idContrainte,typContrainte);
//            
//            for (std::list<std::string>::iterator itit=itiContrainte.begin();itit!=itiContrainte.end();++itit){
//                epg::graph::EpgGraph::edge_descriptor ed=graph2contraint.getEdgeById(*itit).second;
//                epg::graph::concept::setAttribute(graph2contraint[ed],params.getValue(NET_TYPE),ign::data::Integer(typContrainte));
//            }
//        }
////        else
////            addNotifOp(fsNotif,idContrainte,typContrainte);
//
//        ++display;
//    }
//    log.write ("execution time :" + ign::data::Integer( timer.elapsed() ).toString() ) ;
//
//	log.write ("calcul itineraire des contraintes de type "+ign::data::String(typContrainte)+" termine");
//}
//
//
//void ContraintAuto::runItiContraint(int typContrainte,std::string typRes2contraint)
//{
//    epg::Context* context=epg::ContextS::getInstance();
//
//    epg::params::EpgParameters const& params=context->getEpgParameters();
//
//	epg::log::ScopeLogger log( "[ app::calcul::contrainte::runItiContraint ]" );
//
//    ign::feature::sql::FeatureStorePostgis fsIti(*context->getDataBaseManager().getConnection());
//    fsIti.load(data::paramERM::t_iti,"id","geometrie");
//
//    ign::feature::sql::FeatureStorePostgis fsNotif(*context->getDataBaseManager().getConnection());
//    fsNotif.load(data::paramERM::t_notif_contrainte,"id","geom");
//
//    ign::feature::sql::FeatureStorePostgis fs_tr(*context->getDataBaseManager().getConnection());
//    fs_tr.load(params.getValue(EDGE_TABLE),params.getEdgeId(),params.getValue(GEOM));
//
//
//    
//
//    epg::graph::EpgGraph graph2contraint(data);
//    ign::feature::FeatureFilter filtr2contraint(typRes2contraint);
//    log.write("construction du graphe du reseau de contrainte");
//    graph2contraint.load(filtr2contraint);
//
//
//    //Itineraire iti(graph2contraint);
//
//
//    //calcul de l'itineraire de contrainte par contrainte
//    std::map<std::string,std::list<NdContrainte> > mapC_list_ndC=_ptr_contraintERM->get_mapContrainte();
//    for(std::map<std::string,std::list<NdContrainte> >::iterator it_idContrainte=mapC_list_ndC.begin();it_idContrainte!=mapC_list_ndC.end();it_idContrainte++){
//        std::string idContrainte=it_idContrainte->first;
//        //*(_ptr_dataERM->getFlux()->get_oFlux())<<"id contrainte= "<<idContrainte<<std::endl;
//        std::list<std::string> itiContrainte=_ptr_contraintERM->calculItiContraint(idContrainte,graph2contraint);
//
//        if (!itiContrainte.empty())
//        {
//            //bool b_valiIti;
//            //ajout ds la table iti
//            //b_valiIti=Itineraire::addIti(fs_tr,fsIti,itiContrainte,typContrainte);
//
//            //if(b_valiIti){
//            //    Itineraire::saveIti(fs_tr,itiContrainte,typContrainte);
//
//                //ajout du lien de la contrainte et de son type
//            //    ign::feature::Feature fIti;
//            //    fsIti.getFeatureById(ign::data::Integer(fsIti.numFeatures()).toString(),fIti);
//            //    fIti.setAttribute("lien_contrainte",ign::data::String(idContrainte));
//            //    fIti.setAttribute("type_contrainte",ign::data::Integer(typContrainte));
//            //    fsIti.modifyFeature(fIti);
//            //}
//            //else
//            //    addNotifOp(fsNotif,idContrainte,typContrainte);
//            for (std::list<std::string>::iterator itit=itiContrainte.begin();itit!=itiContrainte.end();++itit){
//                epg::graph::EpgGraph::edge_descriptor ed=graph2contraint.getEdgeById(*itit).second;
//                epg::graph::concept::setAttribute(graph2contraint[ed],params.getValue(NET_TYPE),ign::data::Integer(typContrainte));
//            }
//        }
//        else
//            addNotifOp(fsNotif,idContrainte,typContrainte);
//    }
//	log.write("calcul itineraire des contraintes de type "+ign::data::String(typContrainte)+" termine"<<std::endl);
//}
//
//
//S