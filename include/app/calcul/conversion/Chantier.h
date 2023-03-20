#ifndef _APP_DATA_CHANTIER_H
#define _APP_DATA_CHANTIER_H

//
//#include <reqSQL.h>

#include <epg/Context.h>

/*#include <app/data/parametrage_r500.h>
#include <app/data/creation_sql.h>*/

#include <epg/SingletonT.h>
#include <app/params/ThemeParameters.h>


//appel ici des creation sql et series de test

namespace app{
namespace calcul{
namespace conversion{

class Chantier{

	public:

    Chantier();

	~Chantier();

    /*void initial();
	void selec_reseau_principal();
	void selec_rattach();
	void supprime_non_revetu();//ne supprime pas le non revetu principal
	void ecarte_bretelle();
	void create_index();
	void supprime_enConstruction();
    void runOldRes();*/

	/// \brief ajout des attributs sur la table des troncons de route
	void addAttributes_tr_route();

	/// \brief ajout des attributs sur la table des noeuds routiers
	void addAttributes_nd_routier();

	/// \brief ajout des attributs sur les tables des points d'interet(gare, aerodrome, chef lieu)
    void addAttributes_pt_int(std::string buitpName, std::string tabl_arrondt_paris);

	/// \brief creation de la table des itineraires, de la table des cycles, de la table de contrainte et de la table de lien contrainte-nd
    void createTableTravail_anneeN(std::string tabl_itineraire,std::string tabl_cycle);

	/// \brief supprime le reseau en contrcuction
    void supprNonCirculable();

	/// \brief supprime le reseau en contrcuction
    //void supprEnConstruction(std::string tableEnConstruction);

    /// \brief supprime les bretelles et copie dans une autre table
    void supprBretelle(std::string tabl_bretelle);

    /// \brief supprime les acces interdit et copie dans une autre table
    void suppr_accesInterdit(std::string tabl_accesInterdit);

    /// \brief ajoute les embarcaderes dans une table
    void add_embarcadere(std::string tabl_embarcadere);

	/*/// \brief supprime les liens dans les table avec les troncons supprimes + nd sans lien avec des troncons
	void supprInutil(std::string tabl_tr_route,std::string tabl_nd_routier,std::string tabl_franchissement);*/

	/// \brief ecarte l'etranger
    void supprEtranger(std::string codeTerritory, std::string tabl_etranger, int distBuffer);

	/// \brief mise en place de la base de travail
	void run();

    /// \brief ajout d'une table aerodrome ponctuel
    void addAerodromePunctual(std::string tabl_aerodromPunctual,std::string tabl_aerodrome,std::string tabl_bat_rq);

    /// \brief copie de la table des arcs d'une table des arcs sans franchissement
    void copyTablArcNoFranchissement();

    /// \brief recup des nd ini et fin des troncons  non remplis
    void correcTopo();

    void add_arrondt_paris(std::string  buitpName,std::string tablArrondt_paris);

    void supprNd_degNull();

    void addEmpreinte(std::string tabl,std::string idName, std::string geomName);

    void prepGCVS();


	private:
	typedef epg::Singleton< app::params::TransParameters >   TransParametersSingleton;
	app::params::TransParameters* _transParameter;
/*
	private:

	std::string nom_tabl;//
    //ign::sql::pgsql::ConnectionPgsql* _connect_bdd;
    //Creation_sql _requete_sql;//

    //ReqSql _requeteSql;
    //ReqSql _requeteSql;
*/
};

}
}
}

#endif
