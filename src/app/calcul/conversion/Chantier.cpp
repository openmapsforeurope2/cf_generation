#include <app/calcul/conversion/Chantier.h>

#include<boost/progress.hpp>

#include <ign/feature/sql/FeatureIteratorPostgis.h>

//EPG
#include <epg/Context.h>
#include <epg/log/ScopeLogger.h>
#include <epg/sql/tools/numFeatures.h>

//APP
#include <app/params/ThemeParameters.h>


using namespace std;
using namespace ign;
using namespace ign::sql;
using namespace ign::sql::pgsql;
using namespace ign::feature::sql;

namespace app{
namespace calcul{
namespace conversion{

//faire un test pour preciser si cette table n'existe pas
//faire un test pour preciser si cette colonne n'existe pas dans cette table


Chantier::Chantier(){
	_transParameter = app::params::TransParametersS::getInstance();
}


Chantier::~Chantier(){
    //context->getDataBaseManager().getConnection()->close();
}


void Chantier::addAerodromePunctual(std::string tabl_aerodromPunctual,std::string tabl_aerodrome,std::string tabl_bat_rq)
{
	epg::Context* context=epg::ContextS::getInstance();

	if( context->getDataBaseManager().tableExists( tabl_aerodromPunctual) )
		context->getDataBaseManager().getConnection()->update("DROP TABLE "+tabl_aerodromPunctual);

    context->getDataBaseManager().getConnection()->update("CREATE TABLE "+tabl_aerodromPunctual+" ("+context->getEpgParameters().getValue(ID).toString()+" character varying(24), id_aerodrome character varying(24),id_aerogare character varying(24), geometrie geometry,"+context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString()+" character varying, CONSTRAINT "+tabl_aerodromPunctual+"_pkey PRIMARY KEY ("+context->getEpgParameters().getValue(ID).toString()+"),CONSTRAINT enforce_dims_geom CHECK (st_ndims(geometrie) = 2),CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geometrie) = 'POINT'::text OR geometrie IS NULL),CONSTRAINT enforce_srid_geom CHECK (st_srid(geometrie) = (0))) WITH OIDS");


    //remplissage de la table
    ign::feature::sql::FeatureStorePostgis fs_aerodrome(*context->getDataBaseManager().getConnection()),fs_aerodromePunctual(*context->getDataBaseManager().getConnection());

    fs_aerodrome.load(tabl_aerodrome,""+context->getEpgParameters().getValue(ID).toString()+"","geometrie");
    fs_aerodromePunctual.load(tabl_aerodromPunctual,""+context->getEpgParameters().getValue(ID).toString()+"","geometrie");


    ign::feature::sql::FeatureIteratorPostgis it_aerodrome(fs_aerodrome,ign::feature::FeatureFilter());

    ign::feature::sql::FeatureStorePostgis fs_bat_rq(*context->getDataBaseManager().getConnection());
    fs_bat_rq.load(tabl_bat_rq,""+context->getEpgParameters().getValue(ID).toString()+"","geometrie");


    while (it_aerodrome.hasNext()){

        ign::feature::FeatureFilter filtr_aerogare("nature='Aérogare'");

        ign::feature::Feature aerodrome=it_aerodrome.next();

        ign::data::String idAerodrome=ign::data::String(aerodrome.getId());

        ign::feature::Feature aerodromePunctual;

        ign::geometry::Point pt_aerodrom;

        //recuperation de la geometrie: point au centroide de la plus grande surface ou aerogare(s)

        ign::geometry::MultiPolygon mPoly_aerodrom=aerodrome.getGeometry().asMultiPolygon();

        ign::geometry::Envelope env_aerodrome;
        mPoly_aerodrom.envelope(env_aerodrome);

        filtr_aerogare.setExtent(env_aerodrome);

        std::vector<std::pair<std::string,ign::geometry::Point> > v_aerogare;

        ign::feature::sql::FeatureIteratorPostgis it_aerogare(fs_bat_rq,filtr_aerogare);

        while(it_aerogare.hasNext()){
            ign::feature::Feature f_aerogare=it_aerogare.next();

            if(mPoly_aerodrom.contains(f_aerogare.getGeometry().asPoint()))
                v_aerogare.push_back(make_pair(f_aerogare.getId(),f_aerogare.getGeometry().asPoint()));
        }


        if(v_aerogare.empty()){//recuperation du centroide de la plus grande surface

            //recuperation de l'identifiant
            std::string idPunctual=ign::data::String(idAerodrome);

            aerodromePunctual.setId(idPunctual);

            //insertion dans la table des aerodromes ponctuels
            fs_aerodromePunctual.createFeature(aerodromePunctual,idPunctual);

            //recuperation de la plus grande surface

            ign::geometry::Polygon& poly_curr=mPoly_aerodrom.front();
            ign::geometry::Polygon& poly_end=mPoly_aerodrom.back();

            ign::geometry::Polygon& poly_areaMax=poly_curr;

            int i=1;

            while(!poly_curr.equals(poly_end))
            {
                  poly_curr=mPoly_aerodrom.polygonN(i);

                  if(poly_curr.area()>poly_areaMax.area())
                      poly_areaMax=poly_curr;

                  i++;
            }


            //recuperation du centroide de la plus grande surface
            pt_aerodrom=poly_areaMax.centroid();

            aerodromePunctual.setAttribute("id_aerodrome",idAerodrome);

            aerodromePunctual.setGeometry(pt_aerodrom);

            //modification de la geometrie dans la table des aerodromes ponctuels
            fs_aerodromePunctual.modifyFeature(aerodromePunctual);

            continue;//aerodrome suivant
        }


        //si il y a un ou plusieurs aerogare
        for(size_t i=0;i<v_aerogare.size();++i){

            //recuperation de l'identifiant
            std::string idPunctual;

            if(v_aerogare.size()==1)
                idPunctual=ign::data::String(idAerodrome);
            else
                idPunctual=ign::data::String(v_aerogare[i].first);

            aerodromePunctual.setId(idPunctual);

            //insertion dans la table des aerodromes ponctuels
            fs_aerodromePunctual.createFeature(aerodromePunctual,idPunctual);

            pt_aerodrom=v_aerogare[i].second;

            aerodromePunctual.setAttribute("id_aerogare",ign::data::String(v_aerogare[i].first));

            aerodromePunctual.setAttribute("id_aerodrome",idAerodrome);

            aerodromePunctual.setGeometry(pt_aerodrom);

            //modification de la geometrie dans la table des aerodromes ponctuels
            fs_aerodromePunctual.modifyFeature(aerodromePunctual);
        }
    }
}


void Chantier::addAttributes_tr_route()
{
	epg::log::ScopeLogger log( "app::calcul::conversion::Chantier::addAttributes_tr_route" );
	boost::timer timer;
	epg::Context* context = epg::ContextS::getInstance();

	std::string nameTablEdge = context->getEpgParameters().getValue( EDGE_TABLE ).toString();
	std::string nameEdgeCode = context->getEpgParameters().getValue( EDGE_CODE ).toString();
	std::string countryCodeName = context->getEpgParameters().getValue( COUNTRY_CODE ).toString();

	bool structureIsModified = false;

	if ( !context->getDataBaseManager().columnExists( nameTablEdge, context->getEpgParameters().getValue( NET_TYPE ).toString() ) ) {
		context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + nameTablEdge + " ADD COLUMN " + context->getEpgParameters().getValue( NET_TYPE ).toString() + " integer default 0" );
		structureIsModified = true;
	}
	/*if( !context->getDataBaseManager().columnExists( nameTablEdge, "reseau_principal" ) )
		context->getDataBaseManager().getConnection()->update("ALTER TABLE " + nameTablEdge + " ADD COLUMN reseau_principal character varying default ''");*/

	if ( !context->getDataBaseManager().columnExists( nameTablEdge, "isLinkable" ) ) {
		context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + nameTablEdge + " ADD COLUMN isLinkable boolean default false" );
		structureIsModified = true;
	}

	if ( !context->getDataBaseManager().columnExists( nameTablEdge, nameEdgeCode ) ) {
		context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + nameTablEdge + " ADD COLUMN " + nameEdgeCode + " character varying(18) default 'UNK'" );
		structureIsModified = true;
	}

	if ( !context->getDataBaseManager().columnExists( nameTablEdge, countryCodeName ) ) {
		context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + nameTablEdge + " ADD COLUMN " + countryCodeName + " character varying(18) default 'XX'" );
		structureIsModified = true;
	}

//	context->getDataBaseManager().getConnection()->update( "UPDATE " + nameTablEdge + " SET " + nameEdgeCode + " = (SELECT numero_de_route FROM " + _transParameter->getValue( BDUNI_ROUTE_NUMEROTEE ).toString() + " where " + _transParameter->getValue( BDUNI_ROUTE_NUMEROTEE ).toString() + "." + context->getEpgParameters().getValue( ID ).toString() + "=" + nameTablEdge + ".liens_vers_route_numerotee AND numero_de_route!='') WHERE liens_vers_route_numerotee!='';" );


	if ( structureIsModified ) context->refreshFeatureStore( epg::EDGE );


	log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() );

}

void Chantier::addAttributes_nd_routier()
{
	epg::log::ScopeLogger log( "app::calcul::conversion::Chantier::addAttributes_nd_routier" );
	boost::timer timer;
	epg::Context* context = epg::ContextS::getInstance();

	std::string nameTablVertex = context->getEpgParameters().getValue( VERTEX_TABLE ).toString();
	std::string countryCodeName = context->getEpgParameters().getValue( COUNTRY_CODE ).toString();
	std::string vertexFictitiousName = context->getEpgParameters().getValue( VERTEX_FICTITIOUS ).toString();

	bool structureIsModified = false;

	if ( !context->getDataBaseManager().columnExists( nameTablVertex, context->getEpgParameters().getValue( NET_TYPE ).toString() ) ) {
		context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + nameTablVertex + " ADD COLUMN " + context->getEpgParameters().getValue( NET_TYPE ).toString() + " integer default 0" );
		structureIsModified = true;
	}

	//rattachement que sur un troncon maintenant donc les vertex n'on plus besoin d'attribut isLinkable
	/*
   if( !context->getDataBaseManager().columnExists( nameTablVertex, "isLinkable" ) )
	   context->getDataBaseManager().getConnection()->update("ALTER TABLE " + nameTablVertex + " ADD COLUMN isLinkable boolean default false");
	   */

	if ( !context->getDataBaseManager().columnExists( nameTablVertex, "dangles" ) ) {
		context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + nameTablVertex + " ADD COLUMN dangles boolean default false" );
		structureIsModified = true;
	}

	if ( !context->getDataBaseManager().columnExists( nameTablVertex, vertexFictitiousName ) ) {
		context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + nameTablVertex + " ADD COLUMN " + vertexFictitiousName + " boolean default true" );
		structureIsModified = true;
	}

	if ( !context->getDataBaseManager().columnExists( nameTablVertex, countryCodeName ) ) {
		context->getDataBaseManager().getConnection()->update( "ALTER TABLE " + nameTablVertex + " ADD COLUMN " + countryCodeName + " character varying default 'XX'" );
		structureIsModified = true;
	}

	if ( structureIsModified ) context->refreshFeatureStore( epg::VERTEX );

	log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() );

}

void Chantier::addAttributes_pt_int(std::string  buitpName, std::string tabl_arrondt_paris )
{
	epg::Context* context=epg::ContextS::getInstance();
	
	epg::log::EpgLogger* logger= epg::log::EpgLoggerS::getInstance();

	app::params::TransParameters* transParameter = app::params::TransParametersS::getInstance();
/*
	std::string const& linkedFeatidName=context->getEpgParameters().getValue(LINKED_FEATURE_ID).toString();

	std::string const& airfldcName=transParameter->getValue( AIRFLDC_TABLE ).toString();

	std::string const& airfldpName=transParameter->getValue( AIRFLDP_TABLE ).toString();

	std::string const& harborcName=transParameter->getValue( HARBORC_TABLE ).toString();

	std::string const& harborpName=transParameter->getValue( HARBORP_TABLE ).toString();

	std::string const& ferryName=transParameter->getValue( FERRYC_TABLE ).toString();

	std::string const& railrdcName=transParameter->getValue( TARGET_RAILRDC_TABLE ).toString();

	std::string const& buitpName=transParameter->getValue(BUILTUPP_TABLE).toString();


	if( !context->getDataBaseManager().tableExists( airfldcName ) )
		logger->log(epg::log::ERROR,"la table airfldc n'existe pas");

	else {
		if( context->getDataBaseManager().columnExists( airfldcName, linkedFeatidName ) )
			context->getDataBaseManager().getConnection()->update("ALTER TABLE "+airfldcName+" DROP COLUMN "+linkedFeatidName);

		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+airfldcName+" ADD COLUMN "+linkedFeatidName+" character varying default ''");
	}


    if( !context->getDataBaseManager().tableExists( airfldpName ) )
		logger->log(epg::log::ERROR,"la table airfldp n'existe pas");

	else {
		if( context->getDataBaseManager().columnExists( airfldpName, linkedFeatidName ) )
			context->getDataBaseManager().getConnection()->update("ALTER TABLE "+airfldpName+" DROP COLUMN "+linkedFeatidName);

		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+airfldpName+" ADD COLUMN "+linkedFeatidName+" character varying default ''");
	}


    if( !context->getDataBaseManager().tableExists( harborcName ) )
		logger->log(epg::log::ERROR,"la table harborc n'existe pas");

	else {
		if( context->getDataBaseManager().columnExists( harborcName, linkedFeatidName ) )
			context->getDataBaseManager().getConnection()->update("ALTER TABLE "+harborcName+" DROP COLUMN "+linkedFeatidName);

		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+harborcName+" ADD COLUMN "+linkedFeatidName+" character varying default ''");
	}

	
	if( !context->getDataBaseManager().tableExists( harborpName ) )
		logger->log(epg::log::ERROR,"la table harborp n'existe pas");

	else {
		if( context->getDataBaseManager().columnExists( harborpName, linkedFeatidName ) )
			context->getDataBaseManager().getConnection()->update("ALTER TABLE "+harborpName+" DROP COLUMN "+linkedFeatidName);

		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+harborpName+" ADD COLUMN "+linkedFeatidName+" character varying default ''");
	}


    if( !context->getDataBaseManager().tableExists( ferryName ) )
        logger->log(epg::log::ERROR,"la table ferryc n'existe pas");

	else { 
		if( context->getDataBaseManager().columnExists( ferryName, linkedFeatidName ) )
			context->getDataBaseManager().getConnection()->update("ALTER TABLE "+ferryName+" DROP COLUMN "+linkedFeatidName);
		
		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+ferryName+" ADD COLUMN "+linkedFeatidName+" character varying default ''");
	}


    if( !context->getDataBaseManager().tableExists( railrdcName ) )
		logger->log(epg::log::ERROR,"la table railrdc n'existe pas");

	else {
		if( context->getDataBaseManager().columnExists( railrdcName, linkedFeatidName ) )
			context->getDataBaseManager().getConnection()->update("ALTER TABLE "+railrdcName+" DROP COLUMN "+linkedFeatidName);
	
		context->getDataBaseManager().getConnection()->update("ALTER TABLE "+railrdcName+" ADD COLUMN "+linkedFeatidName+" character varying default ''");
	}

*/
    //chef lieu
	//temporaire, ensuite utiliser builtp->modifier avec aurore la construction de cette table

	if( !context->getDataBaseManager().tableExists( buitpName ) )
		logger->log(epg::log::ERROR,"la table buitpName n'existe pas");

	else {
		context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY "+buitpName+" WHERE importance !='Chef-lieu de commune' or toponyme ='paris'");

		add_arrondt_paris(buitpName,tabl_arrondt_paris);
/*
		if( context->getDataBaseManager().columnExists( buitpName, linkedFeatidName ) )
			context->getDataBaseManager().getConnection()->update("ALTER TABLE "+buitpName+" DROP COLUMN "+linkedFeatidName);

			context->getDataBaseManager().getConnection()->update("ALTER TABLE " + buitpName + " ADD COLUMN "+linkedFeatidName+" character varying default ''");*/
	}

}


void Chantier::add_arrondt_paris(std::string  buitpName,std::string tablArrondt_paris)
{
	epg::Context* context=epg::ContextS::getInstance();

	epg::log::EpgLogger* logger= epg::log::EpgLoggerS::getInstance();

	if( !context->getDataBaseManager().tableExists( tablArrondt_paris ) ){
		logger->log(epg::log::ERROR,"la table "+tablArrondt_paris+" n'existe pas");
		return;
	}

    ign::feature::sql::FeatureStorePostgis fs_arrondt_p(*context->getDataBaseManager().getConnection()),fs_chef_lieu(*context->getDataBaseManager().getConnection());
	fs_arrondt_p.load(tablArrondt_paris,"insee",context->getEpgParameters().getValue(GEOM).toString());
    fs_chef_lieu.load(buitpName,context->getEpgParameters().getValue(ID).toString(),context->getEpgParameters().getValue(GEOM).toString());

    ign::feature::sql::FeatureIteratorPostgis  it_arrondt_p(fs_arrondt_p,ign::feature::FeatureFilter());

    while(it_arrondt_p.hasNext()){
        ign::feature::Feature f_arrondt_p=it_arrondt_p.next();
        ign::feature::Feature f_chef_lieu;
        std::string id_chef_lieu="BDC_ZHAB000000000PA";
        id_chef_lieu+=f_arrondt_p.getId();
        f_chef_lieu.setId(id_chef_lieu);
        fs_chef_lieu.createFeature(f_chef_lieu,id_chef_lieu);
        f_chef_lieu.setGeometry(f_arrondt_p.getGeometry());
        f_chef_lieu.setAttribute("toponyme",ign::data::String("paris"));
        f_chef_lieu.setAttribute("numero_insee",ign::data::String(f_arrondt_p.getId()));
        fs_chef_lieu.modifyFeature(f_chef_lieu);
    }
}


void Chantier::createTableTravail_anneeN(std::string tabl_itineraire, std::string tabl_cycle)
{
	epg::Context* context=epg::ContextS::getInstance();

	//itineraire
	if( context->getDataBaseManager().tableExists( tabl_itineraire) )
		context->getDataBaseManager().getConnection()->update("DROP TABLE "+tabl_itineraire);

	context->getDataBaseManager().getConnection()->update("CREATE TABLE "+tabl_itineraire+" (id serial NOT NULL, geometrie geometry,id_troncon character varying,id_nd_routier character varying,type_reseau integer,id_pt_int character varying,lien_contrainte character varying,type_contrainte integer, CONSTRAINT "+tabl_itineraire+"_pkey PRIMARY KEY (id),CONSTRAINT enforce_dims_geometrie CHECK (st_ndims(geometrie) = 2),CONSTRAINT enforce_geotype_geometrie CHECK (geometrytype(geometrie) = 'LINESTRING'::text OR geometrie IS NULL),CONSTRAINT enforce_srid_geometrie CHECK (st_srid(geometrie) = (0)))");


	//cycle
	if( context->getDataBaseManager().tableExists( tabl_cycle) )
		context->getDataBaseManager().getConnection()->update("DROP TABLE "+tabl_cycle);

    context->getDataBaseManager().getConnection()->update("CREATE TABLE "+tabl_cycle+" (id serial, geom geometry, CONSTRAINT "+tabl_cycle+"_pkey PRIMARY KEY (id),CONSTRAINT enforce_dims_geom CHECK (st_ndims(geom) = 2),CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geom) = 'POLYGON'::text OR geom IS NULL),CONSTRAINT enforce_srid_geom CHECK (st_srid(geom) = (0)))");

	//contrainte et lien contrainte - nd de contrainte
    //context->getDataBaseManager().getConnection()->update("DROP TABLE IF EXISTS "+tabl_contrainte+";CREATE TABLE "+tabl_contrainte+" (id serial NOT NULL,type_contrainte integer,lien_obj_contraint character varying,lien_rattach character varying, geometrie geometry, CONSTRAINT "+tabl_contrainte+"_pkey PRIMARY KEY (id))");
    //context->getDataBaseManager().getConnection()->update("DROP TABLE IF EXISTS "+tabl_lien_contrainte_nd+";CREATE TABLE "+tabl_lien_contrainte_nd+" (id serial,id_contrainte integer,id_ndcontraint integer,type_contrainte integer,id_obj_contraint character varying, geom_ndcontrainte geometry,CONSTRAINT "+tabl_lien_contrainte_nd+"_pkey PRIMARY KEY (id),CONSTRAINT enforce_dims_geom_ndcontrainte CHECK (st_ndims(geom_ndcontrainte) = 2),CONSTRAINT enforce_geotype_geom_ndcontrainte CHECK (geometrytype(geom_ndcontrainte) = 'POINT'::text OR geom_ndcontrainte IS NULL),CONSTRAINT enforce_srid_geom_ndcontrainte CHECK (st_srid(geom_ndcontrainte) = (0)))");
    //context->getDataBaseManager().getConnection()->update(_requete_sql.add_index(tabl_lien_contrainte_nd,"id_contrainte","btree"));
}


void Chantier::supprNonCirculable()
{
	epg::log::ScopeLogger log("app::calcul::conversion::Chantier::supprNonCirculable" );
	boost::timer timer;
	epg::Context* context=epg::ContextS::getInstance() ;

	std::string const& edgeTableName=context->getEpgParameters().getValue(EDGE_TABLE).toString() ;

	context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY " +edgeTableName+" WHERE vocation = 'Piste cyclable' or etat_physique ='Sentier' or etat_physique ='Chemin d''exploitation'");

	context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY " +context->getEpgParameters().getValue(EDGE_TABLE).toString()+" WHERE etat_de_l_objet='En projet'");

	log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() ) ;
}

/*
void Chantier::supprEnConstruction(std::string tableEnConstruction)
{
	epg::Context* context=epg::ContextS::getInstance();

	std::string nameTablEdge=context->getEpgParameters().getValue(EDGE_TABLE).toString();

	if( context->getDataBaseManager().tableExists( tableEnConstruction) )
		context->getDataBaseManager().getConnection()->update("DROP TABLE "+tableEnConstruction);
		
	context->getDataBaseManager().getConnection()->update("CREATE TABLE "+tableEnConstruction+" AS (SELECT * FROM "+nameTablEdge+" WHERE (etat_de_l_objet='En construction' or etat_de_l_objet='En projet') and net)");

    context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY " +context->getEpgParameters().getValue(EDGE_TABLE).toString()+" WHERE etat_de_l_objet='En construction' or etat_de_l_objet='En projet'");
}
*/


void Chantier::supprBretelle(std::string tabl_bretelle)
{
	epg::log::ScopeLogger log("app::calcul::conversion::Chantier::supprBretelle" );
	boost::timer timer;
	epg::Context* context=epg::ContextS::getInstance();

	std::string nameTablEdge=context->getEpgParameters().getValue(EDGE_TABLE).toString();

	if( context->getDataBaseManager().tableExists( tabl_bretelle) )
		context->getDataBaseManager().getConnection()->update("DROP TABLE "+tabl_bretelle);
		
	context->getDataBaseManager().getConnection()->update("CREATE TABLE "+tabl_bretelle+" AS (SELECT * FROM "+nameTablEdge+" WHERE vocation = 'Bretelle')");
    
	context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY "+nameTablEdge+" WHERE vocation = 'Bretelle'");
    //context->getDataBaseManager().getConnection()->update(_requete_sql.drop_obj(tabl_nd_routier,tabl_nd_routier+".id_bdcarto IN (SELECT "+tabl_nd_routier+".id_bdcarto FROM "+tabl_nd_routier+" EXCEPT ( SELECT id_bdcarto_bdc_noeud_routier_ini FROM  "+tabl_tr_route+" UNION SELECT id_bdcarto_bdc_noeud_routier_fin FROM  "+tabl_tr_route+"))"));
	
	log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() ) ;
}

void Chantier::suppr_accesInterdit(std::string tabl_accesInterdit)
{
	epg::log::ScopeLogger log("app::calcul::conversion::Chantier::supprAccesInterdit" );
	boost::timer timer;
	epg::Context* context=epg::ContextS::getInstance();

	std::string nameTablEdge=context->getEpgParameters().getValue(EDGE_TABLE).toString();

	if( context->getDataBaseManager().tableExists( tabl_accesInterdit) )
		 context->getDataBaseManager().getConnection()->update("DROP TABLE IF EXISTS "+tabl_accesInterdit);

    context->getDataBaseManager().getConnection()->update("CREATE TABLE "+tabl_accesInterdit+" AS (SELECT * FROM "+nameTablEdge+" WHERE acces = 'Interdit')");

    context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY "+nameTablEdge+" WHERE acces = 'Interdit'");
    //context->getDataBaseManager().getConnection()->update(_requete_sql.drop_obj(tabl_nd_routier,tabl_nd_routier+".id_bdcarto IN (SELECT "+tabl_nd_routier+".id_bdcarto FROM "+tabl_nd_routier+" EXCEPT ( SELECT id_bdcarto_bdc_noeud_routier_ini FROM  "+tabl_tr_route+" UNION SELECT id_bdcarto_bdc_noeud_routier_fin FROM  "+tabl_tr_route+"))"));

	log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() ) ;
}

void Chantier::add_embarcadere(std::string tabl_embarcadere)
{
	epg::Context* context=epg::ContextS::getInstance();

	std::string nameTablVertex=context->getEpgParameters().getValue(VERTEX_TABLE).toString();


	if( context->getDataBaseManager().tableExists( tabl_embarcadere) )
		context->getDataBaseManager().getConnection()->update("DROP TABLE IF EXISTS "+tabl_embarcadere);

    context->getDataBaseManager().getConnection()->update("CREATE TABLE "+tabl_embarcadere+" ("+context->getEpgParameters().getValue(ID).toString()+" character varying(24) NOT NULL, id_nd_routier integer, geometrie geometry,id_rattach character varying,id_iti_rattach character varying, CONSTRAINT "+tabl_embarcadere+"_pkey PRIMARY KEY ("+context->getEpgParameters().getValue(ID).toString()+"),CONSTRAINT enforce_dims_geom CHECK (st_ndims(geometrie) = 2),CONSTRAINT enforce_geotype_geom CHECK (geometrytype(geometrie) = 'POINT'::text OR geometrie IS NULL),CONSTRAINT enforce_srid_geom CHECK (st_srid(geometrie) = (0))) WITH OIDS");

    ign::feature::sql::FeatureStorePostgis fs_embarcadere(*context->getDataBaseManager().getConnection()),fs_nd_routier(*context->getDataBaseManager().getConnection());
    fs_nd_routier.load(nameTablVertex,context->getEpgParameters().getValue(ID).toString(),context->getEpgParameters().getValue(GEOM).toString());
    fs_embarcadere.load(tabl_embarcadere,""+context->getEpgParameters().getValue(ID).toString()+"","geometrie");

    ign::feature::FeatureFilter filtr_embarcadere("nature = 'Embarcadère'");
    ign::feature::sql::FeatureIteratorPostgis it_embarcadere(fs_nd_routier,filtr_embarcadere);


    while(it_embarcadere.hasNext()){

        ign::feature::Feature f_embarcadere=it_embarcadere.next();

        ign::feature::Feature f_ptInt_embarcadere;

        std::string id_ptInt_embarcadere=f_embarcadere.getId();

        f_ptInt_embarcadere.setId(ign::data::String(id_ptInt_embarcadere));

        f_ptInt_embarcadere.setAttribute("id_nd_routier", ign::data::Integer(f_embarcadere.getAttribute("id_bdcarto").toInteger()));

        fs_embarcadere.createFeature(f_ptInt_embarcadere,id_ptInt_embarcadere);

        f_ptInt_embarcadere.setGeometry(f_embarcadere.getGeometry());

        fs_embarcadere.modifyFeature(f_ptInt_embarcadere);
    }
}



/*void Chantier::supprInutil(std::string tabl_tr_route, std::string tabl_nd_routier, std::string tabl_franchissement)
{
    context->getDataBaseManager().getConnection()->update(_requete_sql.drop_obj(tabl_nd_routier,tabl_nd_routier+".id_bdcarto IN (SELECT "+tabl_nd_routier+".id_bdcarto FROM "+tabl_nd_routier+" EXCEPT ( SELECT id_bdcarto_bdc_noeud_routier_ini FROM  "+tabl_tr_route+" UNION SELECT id_bdcarto_bdc_noeud_routier_fin FROM  "+tabl_tr_route+"))"));

}*/

void Chantier::supprEtranger( std::string codeTerritory, std::string tabl_etranger, int distBuffer )
{
	epg::log::ScopeLogger log( "app::calcul::conversion::Chantier::supprEtranger" );
	boost::timer timer;

	epg::Context* context = epg::ContextS::getInstance();

	std::string queryTerritory = "";
	if ( codeTerritory == "FR" )
		queryTerritory = "icc like '%FR%' or icc like '%MC%'";
	else
		queryTerritory = "icc like '%" + codeTerritory + "%'";

	if ( !context->getDataBaseManager().columnExists( context->getEpgParameters().getValue( EDGE_TABLE ).toString(), "in_france" ) )
		context->getDataBaseManager().getConnection()->update( "alter table " + context->getEpgParameters().getValue( EDGE_TABLE ).toString() + " add column in_france boolean default false;" );
	else
		context->getDataBaseManager().getConnection()->update( "update " + context->getEpgParameters().getValue( EDGE_TABLE ).toString() + " set in_france = false" );

	context->getDataBaseManager().getConnection()->update( "update " + context->getEpgParameters().getValue( EDGE_TABLE ).toString() + " set in_france = true where st_intersects(st_buffer(" + context->getEpgParameters().getValue( GEOM ).toString() + "," + ign::data::Integer( 1 ).toString() + "),(select st_collect( " + context->getEpgParameters().getValue( GEOM ).toString() + ") from " + context->getEpgParameters().getValue( TARGET_COUNTRY_TABLE ).toString() + " where " + queryTerritory + "))" );

	context->getDataBaseManager().getConnection()->update( "DROP TABLE IF EXISTS " + tabl_etranger + "; CREATE TABLE " + tabl_etranger + " AS (SELECT * FROM " + context->getEpgParameters().getValue( EDGE_TABLE ).toString() + " WHERE in_france=false)" );

	context->getDataBaseManager().getConnection()->update( "DELETE FROM ONLY " + context->getEpgParameters().getValue( EDGE_TABLE ).toString() + " WHERE in_france=false" );

	context->getDataBaseManager().getConnection()->update( "alter table " + context->getEpgParameters().getValue( EDGE_TABLE ).toString() + " drop column in_france;" );


	context->refreshFeatureStore( epg::EDGE );

	log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() );

}

void Chantier::run()
{
	/*
    epg::Context* context=epg::ContextS::getInstance();

    supprDetruit();

	supprEnConstruction(transParameter->getValue(BDUNI_TRONCON_ROUTIER_EN_CONSTRUCTION));

    //suppression ici des chemins exploitation et du non revetu

    supprBretelle(transParameter->getValue(BDUNI_BRETELLE));

	suppr_accesInterdit(transParameter->getValue(BDUNI_ACCES_INTERDIT_TABLE));

	supprEtranger(paramERM::t_etranger,100);

    context->getData().computeTopo();

    add_embarcadere(paramERM::t_embarcadere);

    addAttributes_tr_route();

    addAttributes_nd_routier();

	addAttributes_pt_int(paramERM::t_zone_habit,paramERM::t_arrondt_paris);

	//plus besoin si utilisation des airfldp et airfldc
	//a voir si il faut les produire
	//addAerodromePunctual(paramERM::t_aerodrome_ponctuel,paramERM::t_aerodrome, paramERM::t_batiment_remarquable);

	selectReseauPrincipal();
	
    //suppr des troncons classes isoles ou ajouter pr eviter discontinuite si possible
    //->module pr eviter discontinuite (d'abord ajout puis suppr si pas de solution)
    //prendre en compte la longueur et les distances

	supprNonUtile(paramERM::t_tr_non_revetu);

	selectRattach();

	//plus necessairement utile
	//createTableTravail_anneeN(paramERM::t_iti,paramERM::t_cycle);

    supprNd_degNull();
	*/

}


/* 2022
void Chantier::copyTablArcNoFranchissement()
{
	epg::Context* context=epg::ContextS::getInstance();
	std::string nameTablEdge=context->getEpgParameters().getValue(EDGE_TABLE).toString();

	if( context->getDataBaseManager().tableExists( _transParameter->getValue(BDUNI_ROUTIER_NON_FRANCHISSEMENT).toString() ) )
		context->getDataBaseManager().getConnection()->update("DROP TABLE "+_transParameter->getValue(BDUNI_ROUTIER_NON_FRANCHISSEMENT).toString());

	std::string conditionNoFranchissement=""+context->getEpgParameters().getValue(ID).toString()+" in (SELECT "+context->getEpgParameters().getValue(ID).toString()+" from "+nameTablEdge+" EXCEPT SELECT liens_vers_troncon FROM "+_transParameter->getValue(BDUNI_LIEN_FRANCHISSEMENT).toString() +" WHERE liens_vers_franchissement IN (SELECT liens_vers_franchissement FROM bdc_lien_troncon_franchissement WHERE liens_vers_troncon LIKE 'BDCTROR%' GROUP BY liens_vers_franchissement, niveau HAVING count(*) >= 2)";

	context->getDataBaseManager().getConnection()->update("CREATE TABLE "+_transParameter->getValue(BDUNI_ROUTIER_NON_FRANCHISSEMENT).toString()+" AS(SELECT * FROM "+nameTablEdge+"WHERE "+conditionNoFranchissement+")");

    //std::cout<<"creation table arc sans franchissement"<<std::endl;
}
*/



void Chantier::correcTopo()
{

    epg::Context* context=epg::ContextS::getInstance();

    context->getDataBaseManager().getConnection()->update("alter table "+ context->getEpgParameters().getValue(EDGE_TABLE).toString()+" add "+ context->getEpgParameters().getValue(EDGE_SOURCE).toString()+" character varying");
    context->getDataBaseManager().getConnection()->update("alter table "+ context->getEpgParameters().getValue(EDGE_TABLE).toString()+" add "+ context->getEpgParameters().getValue(EDGE_TARGET).toString()+" character varying");

    ign::feature::sql::FeatureStorePostgis fs_arc(*context->getDataBaseManager().getConnection()),fs_nd(*context->getDataBaseManager().getConnection());
    fs_arc.load(context->getEpgParameters().getValue(EDGE_TABLE).toString(),context->getEpgParameters().getValue(ID).toString(),context->getEpgParameters().getValue(GEOM).toString());
    fs_nd.load(context->getEpgParameters().getValue(VERTEX_TABLE).toString(),context->getEpgParameters().getValue(ID).toString(),context->getEpgParameters().getValue(GEOM).toString());

    //ign::feature::FeatureFilter filtr_noTopo("id_bdcarto_bdc_noeud_routier_ini is null or id_bdcarto_bdc_noeud_routier_fin is null");
    ign::feature::FeatureFilter filtr_noTopo;

    ign::feature::sql::FeatureIteratorPostgis it_arcNoTopo(fs_arc,filtr_noTopo);

    //std::vector<std::string> v_idArc;
    //fs_arc.getAllFeaturesKeys(v_idArc);

    int nb_iter = epg::sql::tools::numFeatures( context->getEpgParameters().getValue(EDGE_TABLE).toString(), context->getEpgParameters().getValue(GEOM).toString(), filtr_noTopo );

    boost::progress_display display(nb_iter, std::cout, "[correcTopo % complete ]\n") ;

    while(it_arcNoTopo.hasNext())
    //for(int i=0;i<v_idArc.size();i++)
    {

        ign::feature::Feature arc=it_arcNoTopo.next();
        /*ign::feature::Feature arc;
        fs_arc.getFeatureById(v_idArc[i],arc);*/

        ign::geometry::Point ptStart=arc.getGeometry().asLineString().startPoint();
        ign::geometry::Point ptEnd=arc.getGeometry().asLineString().endPoint();

        ign::geometry::Envelope envStart,envEnd;

        ptStart.envelope(envStart);
        ptEnd.envelope(envEnd);

        ign::feature::FeatureFilter filterStart,filterEnd;
        filterStart.setExtent(envStart.expandBy(1));
        filterEnd.setExtent(envEnd.expandBy(1));

        ign::feature::sql::FeatureIteratorPostgis it_ndStart(fs_nd,filterStart);

        while(it_ndStart.hasNext())
        {
        ign::feature::Feature nd=it_ndStart.next();

        ign::geometry::Point pt_nd=nd.getGeometry().asPoint();

        if(pt_nd.equals(ptStart))
            arc.setAttribute(context->getEpgParameters().getValue(EDGE_SOURCE).toString(),ign::data::String(nd.getId()));
        }

        ign::feature::sql::FeatureIteratorPostgis it_ndEnd(fs_nd,filterEnd);

        while(it_ndEnd.hasNext())
        {
        ign::feature::Feature nd=it_ndEnd.next();

        ign::geometry::Point pt_nd=nd.getGeometry().asPoint();

        if(pt_nd.equals(ptEnd))
            arc.setAttribute(context->getEpgParameters().getValue(EDGE_TARGET).toString(),ign::data::String(nd.getId()));
        }

        ++display;
        fs_arc.modifyFeature(arc);
    }
}


void Chantier::supprNd_degNull()
{	
	epg::log::ScopeLogger log("app::calcul::conversion::Chantier::supprNd_degNull" );
	boost::timer timer;
	epg::Context* context=epg::ContextS::getInstance();

	std::string nameTablEdge=context->getEpgParameters().getValue(EDGE_TABLE).toString();
	std::string nameTablVertex=context->getEpgParameters().getValue(VERTEX_TABLE).toString();

    context->getDataBaseManager().getConnection()->update("DELETE FROM ONLY "+nameTablVertex+" where "+nameTablVertex+"."+context->getEpgParameters().getValue(ID).toString()+" IN (select "+context->getEpgParameters().getValue(ID).toString()+" from " +nameTablVertex+" except(select "+nameTablVertex+"."+context->getEpgParameters().getValue(ID).toString()+" from "+nameTablVertex+","+nameTablEdge+" where "+nameTablVertex+"."+context->getEpgParameters().getValue(ID).toString()+"="+nameTablEdge+".noeud_ini or "+nameTablVertex+"."+context->getEpgParameters().getValue(ID).toString()+"="+nameTablEdge+".noeud_fin))");

	log.log( epg::log::INFO, "Execution time :" + ign::data::Double( timer.elapsed() ).toString() ) ;

}

void Chantier::prepGCVS()
{
	/*
	epg::Context* context=epg::ContextS::getInstance();

    addEmpreinte(paramERM::t_aerodrome_ponctuel,""+context->getEpgParameters().getValue(ID).toString()+"","geometrie");


    std::string reqSQL=ReqSQL.add_attribut(paramERM::t_aerodrome_ponctuel,"numrec","integer");
    context->getDataBaseManager().getConnection()->update(reqSQL);

    reqSQL=ReqSQL.add_attribut(paramERM::t_aerodrome_ponctuel,"detruit","character varying");
    context->getDataBaseManager().getConnection()->update(reqSQL);

    reqSQL=ReqSQL.add_attribut(paramERM::t_aerodrome_ponctuel,"nom_lot","character varying");
    context->getDataBaseManager().getConnection()->update(reqSQL);
	*/

}

void Chantier::addEmpreinte(std::string tabl,std::string idName, std::string geomName)
{
	epg::Context* context=epg::ContextS::getInstance();

    std::string reqSql;
    reqSql="ALTER TABLE  "+tabl+" ADD COLUMN empreinte geometry;";
    reqSql+="ALTER TABLE "+tabl+" ADD CONSTRAINT enforce_dims_empreinte CHECK (st_ndims(empreinte) = 2);";
    reqSql+="ALTER TABLE "+tabl+" ADD CONSTRAINT enforce_geotype_empreinte CHECK (geometrytype(empreinte) = 'LINESTRING'::text OR empreinte IS NULL);";
    reqSql+="ALTER TABLE "+tabl+" ADD CONSTRAINT enforce_srid_empreinte CHECK (st_srid(empreinte) = (0));";

    context->getDataBaseManager().getConnection()->update(reqSql);


    ign::feature::FeatureFilter filtr("true");
    ign::feature::sql::FeatureStorePostgis fs(*context->getDataBaseManager().getConnection());
    fs.load(tabl,idName,geomName);
    ign::feature::sql::FeatureIteratorPostgis it(fs,filtr);

    while(it.hasNext()){
        ign::feature::Feature f=it.next();

        ign::geometry::Envelope envlp;
        f.getGeometry().envelope(envlp);
        ign::geometry::LineString ls_empreinte=ign::geometry::LineString(ign::geometry::Point(envlp.xmin(),envlp.ymin()),ign::geometry::Point(envlp.xmax(),envlp.ymax()));

        f.setAttribute("empreinte",ls_empreinte);

        fs.modifyFeature(f);
    }

}

}
}
}