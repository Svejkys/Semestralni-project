#include "Stats.h"
#include "Activity.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <map>

using namespace std;

namespace Stats {

    vector<KmSplit> ZiskejVsechnyKmSplity(const Activity& akt) {
		return SpocitejKilometroveUseky(akt);
    }

    static double StupneNaRadiany(double deg) {
        return deg * (3.14159265358979323846 / 180.0);
    }

    static bool JePrestupnyRok(int rok) {
        return (rok % 4 == 0 && rok % 100 != 0) || (rok % 400 == 0);
    }

    static int DenVRoce(int rok, int mesic, int den) {
        static const int kumul[12] = { 0,31,59,90,120,151,181,212,243,273,304,334 };
		int doy = kumul[mesic - 1] + den; //kumulativni dny do minuleho mesice + dny v aktualnim mesici
        if (mesic > 2 && JePrestupnyRok(rok)) doy += 1;
		return doy; //day of year
    }

    // 0=Po ... 6=Ne
    static int DenVTydnuPo0(int rok, int mesic, int den) {
        // Sakamoto: 0=Ne..6=So, upravíme na 0=Po..6=Ne
        static int t[] = { 0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4 };
        int y = rok;
        if (mesic < 3) y -= 1;
        int wSun0 = (y + y / 4 - y / 100 + y / 400 + t[mesic - 1] + den) % 7; // 0=Ne
        int wMon0 = (wSun0 + 6) % 7; // posun: Po=0 ... Ne=6
        return wMon0;
    }

    static bool ParsujIsoDatum(const string& iso, int& rok, int& mesic, int& den) {
        if (iso.size() < 10) return false; // "rok-mesic-den"
        try {
            rok = stoi(iso.substr(0, 4));
            mesic = stoi(iso.substr(5, 2));
            den = stoi(iso.substr(8, 2));
        }
        catch (...) {
            return false;
        }
        return true;
    }

    // sekundy dne z "YYYY-MM-DDTHH:MM:SSZ"
    static long long SekundyDneZIsoCasu(const string& iso) {
        if (iso.size() < 19) return 0;
        try {
            int hh = stoi(iso.substr(11, 2));
            int mm = stoi(iso.substr(14, 2));
            int ss = stoi(iso.substr(17, 2));
            return (long long)hh * 3600LL + (long long)mm * 60LL + (long long)ss;
        }
        catch (...) {
            return 0;
        }
    }

    static int PocetIsoTydnuVRoce(int rok) {
        // ISO: 53 týdnù když 1.1. je ètvrtek nebo pøestupný rok a 1.1. je støeda
        int dowJan1_iso = DenVTydnuPo0(rok, 1, 1) + 1; // 1=Po..7=Ne
        if (dowJan1_iso == 4) return 53;
        if (JePrestupnyRok(rok) && dowJan1_iso == 3) return 53;
        return 52;
    }

    static string FormatMmSsZSekund(long long sek) {
        if (sek < 0) sek = 0;
        long long m = sek / 60;
        long long s = sek % 60;
        ostringstream oss;
        oss << m << ":" << setw(2) << setfill('0') << s;
        return oss.str();
    }

   

    // Haversinova formule
    double VzdalenostMeziBody(const TrackPoint& a, const TrackPoint& b) {
        const double R = 6371.0;
        double lat1 = StupneNaRadiany(a.latitude);
        double lat2 = StupneNaRadiany(b.latitude);
        double dLat = lat2 - lat1;
        double dLon = StupneNaRadiany(b.longitude - a.longitude);

        double sinDLat = sin(dLat / 2.0);
        double sinDLon = sin(dLon / 2.0);

        double h = sinDLat * sinDLat + cos(lat1) * cos(lat2) * sinDLon * sinDLon;
        double c = 2.0 * asin(min(1.0, sqrt(h)));
        return R * c;
    }

    // Celková vzdálenost trasy v km
    double SpocitejCelkovoVzdalenostKm(const vector<TrackPoint>& body) {
        if (body.size() < 2) return 0.0;
        double sum = 0.0;
        for (size_t i = 1; i < body.size(); ++i) {
            sum += VzdalenostMeziBody(body[i - 1], body[i]);
        }
        return sum;
    }

    // Pøevede "HH:MM:SS" nebo "MM:SS" na sekundy
    int PrevodCasuNaSekundy(const string& cas) {
        int h = 0, m = 0, s = 0;
        char c1 = 0, c2 = 0;

        stringstream ss(cas);
        if (ss >> h >> c1 >> m >> c2 >> s) {
            if (c1 == ':' && c2 == ':') return h * 3600 + m * 60 + s;
        }

        ss.clear();
        ss.str(cas);
        h = 0; m = 0; s = 0; c1 = 0;
        if (ss >> m >> c1 >> s) {
            if (c1 == ':') return m * 60 + s;
        }
        return 0;
    }

    string FormatTempoMmSsNaKm(double tempoMinNaKm) {
        if (tempoMinNaKm <= 0.0) return "0:00 /km";
        long long sekNaKm = (long long)llround(tempoMinNaKm * 60.0);
        return FormatMmSsZSekund(sekNaKm) + " /km";
    }

    vector<KmSplit> SpocitejKilometroveUseky(const Activity& akt) {
        vector<KmSplit> splits;
        const auto& body = akt.getPoints();
        if (body.size() < 2) return splits;

        // Budeme držet “virtuální start splitu” pøes èas i vzdálenost,
        // aby split navazoval hezky (bez velkých skokù).
        double celkovaVzd = 0.0;
        double hraniceKm = 1.0;

        // èasové reference
        long long startSek = SekundyDneZIsoCasu(body[0].time);
        long long lastSek = startSek;

        // start aktuálního splitu
        long long splitStartSek = startSek;
        double splitStartVzd = 0.0;
        int poradiKm = 1;

        for (size_t i = 1; i < body.size(); ++i) {
            double dKm = VzdalenostMeziBody(body[i - 1], body[i]);
            double vzdPred = celkovaVzd;
            celkovaVzd += dKm;

            long long sekNow = SekundyDneZIsoCasu(body[i].time);
            if (sekNow < lastSek) sekNow += 24LL * 3600LL; // pùlnoc
            long long sekPrev = SekundyDneZIsoCasu(body[i - 1].time);
            if (sekPrev < startSek) sekPrev += 24LL * 3600LL; // pro jistotu
            lastSek = sekNow;

            // Pokud jsme pøesáhli 1km hranici, mùže se stát, že v jednom segmentu uzavøeme více km.
            while (celkovaVzd >= hraniceKm && dKm > 0.0) {
                double need = hraniceKm - vzdPred; // kolik km chybí do hranice uvnitø segmentu
                double ratio = need / dKm;
                if (ratio < 0.0) ratio = 0.0;
                if (ratio > 1.0) ratio = 1.0;


                // interpolace èasu na hranici
                long long sekHranice = sekPrev + (long long)llround((sekNow - sekPrev) * ratio);

                double splitVzd = hraniceKm - splitStartVzd; // ~1.0
                double splitCas = (double)(sekHranice - splitStartSek);

                KmSplit s;
                s.kmPoradi = poradiKm;
                s.vzdalenostKm = splitVzd;
                s.casSekundy = splitCas;
                s.tempoMinNaKm = (splitVzd > 0.0) ? ((splitCas / 60.0) / splitVzd) : 0.0;

                splits.push_back(s);

                // další split zaèíná pøesnì na hranici
                poradiKm++;
                splitStartSek = sekHranice;
                splitStartVzd = hraniceKm;

                hraniceKm += 1.0;
                // pozor: vzdPred i dKm se vztahují k pùvodnímu segmentu, ale pro vícekilometrový skok je to OK,
                // protože celkovaVzd a hraniceKm pokraèují a ratio se poèítá vždy vùèi stejné dKm a vzdPred (pùvodní).
                // Reálnì vícekilometrové pøeskoèení v bìhu skoro nenastane (body jsou èasto), ale je to ošetøené.
            }
        }

        return splits;
    }

    bool ZiskejPrvniKmSplit(const Activity& akt, KmSplit& out) {
        auto splits = SpocitejKilometroveUseky(akt);
        if (splits.empty()) return false;
        out = splits.front();
        return true;
    }

    bool ZiskejNejlepsiKmSplit(const Activity& akt, KmSplit& out) {
        auto splits = SpocitejKilometroveUseky(akt);
        if (splits.empty()) return false;

        auto it = min_element(splits.begin(), splits.end(),
            [](const KmSplit& a, const KmSplit& b) {
                return a.casSekundy < b.casSekundy;
            });

        out = *it;
        return true;
    }

    // ------------------------------------------------------------
    // ISO TÝDEN: rok + èíslo týdne z ISO èasu (YYYY-MM-DD...)
    // ------------------------------------------------------------
    bool ZiskejIsoTydenZRetezce(const string& isoTime, int& outIsoRok, int& outIsoTyden) {
        int rok, mesic, den;
        if (!ParsujIsoDatum(isoTime, rok, mesic, den)) return false;

        int doy = DenVRoce(rok, mesic, den);
        int dowIso = DenVTydnuPo0(rok, mesic, den) + 1; // 1=Po..7=Ne

        int tyden = (doy - dowIso + 10) / 7;
        int isoRok = rok;

        if (tyden < 1) {
            isoRok = rok - 1;
            tyden = PocetIsoTydnuVRoce(isoRok);
        }
        else {
            int maxTydnu = PocetIsoTydnuVRoce(rok);
            if (tyden > maxTydnu) {
                isoRok = rok + 1;
                tyden = 1;
            }
        }

        outIsoRok = isoRok;
        outIsoTyden = tyden;
        return true;
    }

    // ------------------------------------------------------------
    // Týdenní trend (ISO týdny): prùmìrné tempo + km + poèet bìhù
    // ------------------------------------------------------------
    map<IsoTydenKlic, TrendTydne> SpocitejTydenniTrend(const vector<Activity>& aktivity) {
        map<IsoTydenKlic, TrendTydne> trend;

        for (const auto& akt : aktivity) {
            const auto& pts = akt.getPoints();
            if (pts.empty()) continue;

            int isoRok = 0, isoTyden = 0;
            if (!ZiskejIsoTydenZRetezce(pts.front().time, isoRok, isoTyden)) {
                continue;
            }

            IsoTydenKlic k{ isoRok, isoTyden };
            TrendTydne& t = trend[k];

            t.pocetBehu += 1;
            t.celkemKm += akt.getTotalDistanceKm();
            t.celkemCasSek += akt.getTotalTimeSeconds();
        }

        return trend;
    }

} // namespace Stats
