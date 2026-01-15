#pragma once
#include <vector>
#include <string>
#include <map>
#include "Activity.h"

namespace Stats {

    // haversinova formule pro výpoèet vzdálenosti mezi dvìma GPS body
    double VzdalenostMeziBody(const TrackPoint& a, const TrackPoint& b);

    // spoèítá celkovou vzdálenost trasy
    double SpocitejCelkovoVzdalenostKm(const std::vector<TrackPoint>& body);

    // pøevede èas ze stringu na sekundy
    int PrevodCasuNaSekundy(const std::string& cas);

    // struktura pro jeden kilometr
    struct KmSplit {
        int kmPoradi;
        double vzdalenostKm;
        double casSekundy;
        double tempoMinNaKm;
    };

    // poèítání kilometrových úsekù
    std::vector<KmSplit> SpocitejKilometroveUseky(const Activity& akt);


    bool ZiskejPrvniKmSplit(const Activity& akt, KmSplit& out);
    bool ZiskejNejlepsiKmSplit(const Activity& akt, KmSplit& out);
    // opravena deklarace: odstranìno 'static' a použito plné jmenné prost?edí std::vector
    std::vector<KmSplit> ZiskejVsechnyKmSplity(const Activity& akt);

    std::string FormatTempoMmSsNaKm(double tempoMinNaKm);

    struct IsoTydenKlic {
        int isoRok;
        int isoTyden;
        bool operator<(const IsoTydenKlic& other) const {
            if (isoRok != other.isoRok) return isoRok < other.isoRok;
            return isoTyden < other.isoTyden;
        }
    };

    struct TrendTydne {
        int pocetBehu = 0;
        double celkemKm = 0.0;
        double celkemCasSek = 0.0;

        double PrumerneTempoMinNaKm() const {
            if (celkemKm <= 0.0) return 0.0;
            return (celkemCasSek / 60.0) / celkemKm;
        }
    };

    // vezme èas z prvního bodu aktivity a vrátí rok + týden
    bool ZiskejIsoTydenZRetezce(const std::string& isoTime,
        int& outIsoRok,
        int& outIsoTyden);

    // spoèítá týdenní trendy pro více aktivit
    std::map<IsoTydenKlic, TrendTydne> SpocitejTydenniTrend(
        const std::vector<Activity>& aktivity);

}
