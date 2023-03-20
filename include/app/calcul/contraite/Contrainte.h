#ifndef _APP_CALCUL_CONTRAINTE_CONTRAINTE_AUTO_H_
#define _APP_CALCUL_CONTRAINTE_CONTRAINTE_AUTO_H_


#include<epg/graph/EpgGraph.h>

namespace app{
namespace calcul{
namespace contrainte{

class ContraintAuto{

    public:

    ContraintAuto();

    ~ContraintAuto();

    /// \brief recuperation des contraintes sur le reseau de l'annee N-1
    void getContraint_resOld(epg::graph::EpgGraph & graphOld);

    /// \brief appariement des noeuds de contrainte aux donnees de l'annee N
    void appariNdC(epg::graph::EpgGraph & graphNew, double dist_max);

    /// \brief calcul des itineraires de contrainte d'un type sur un reseau
    /// \param typRes2contraint:type reseau sur lequel s'applique la contrainte
    void runItiContraint(int typContrainte,std::string typRes2contraint);

    /// \brief calcul des itineraires de contrainte d'un type sur un reseau
    /// \param typRes2contraint:type reseau sur lequel s'applique la contrainte
    /// \param typRes2secondaire:type reseau sur lequel les itineraires secondaires sont calcules
    void runItiContraint(int typContrainte,std::string typRes2contraint,std::string typRes2secondaire);


/*
    //temp
    Contrainte* getContrainte(){
        return _ptr_contraintERM;
    }


    private:

    //Contrainte* _ptr_contraintERM;

*/



};
}
}
}

#endif
