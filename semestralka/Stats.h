#pragma once
#include <vector>
#include <string>
#include "Activity.h"

namespace Stats {

    // Spoèítá vzdálenost mezi dvìma GPS body (v kilometrech)
    double VzdalenostMeziBody(const TrackPoint& a, const TrackPoint& b);

    // Spoèítá celkovou délku trasy (souèet vzdáleností všech po sobì jdoucích bodù)
    double SpocitejCelkovoVzdalenostKm(const std::vector<TrackPoint>& body);

    // Pøevod ISO èasu "2023-01-01T12:34:56Z" na sekundy od pùlnoci
    int PrevodCasuNaSekundy(const std::string& cas);

    // Struktura pro kilometrový split
    struct KmSplit {
        int kmPoradi;          // 1, 2, 3, ...
        double vzdalenostKm;   // zhruba 1.0 km
        double casSekundy;     // èas tohoto úseku
        double tempoMinNaKm;   // pace pro tento úsek
    };

    // Výpoèet kilometrových úsekù pro jednu aktivitu
    std::vector<KmSplit> SpocitejKilometroveUseky(const Activity& akt);
}
