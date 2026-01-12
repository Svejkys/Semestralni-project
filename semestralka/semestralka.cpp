// semestralka.cpp : Tento soubor obsahuje funkci main. Provádění programu se tam zahajuje a ukončuje.
//

#include <iostream>
#include <vector>
#include <limits>
#include <iomanip>
#include <filesystem>
#include <sstream>
#include <map>
#include <cstdlib>   // pro system()
#include <cmath>     // pro round()

#include "Activity.h"
#include "Stats.h"
#include "GpxParser.h"

namespace fs = std::filesystem;
using namespace std;

// ---------------------------------------------------------
// Pomocná funkce: spočítá vzdálenost a čas aktivity
// ---------------------------------------------------------
void SpocitejStatistikyAktivity(Activity& akt)
{
    const auto& body = akt.getPoints();
    double vzd = Stats::SpocitejCelkovoVzdalenostKm(body);
    akt.setTotalDistanceKm(vzd);

    if (body.size() >= 2) {
        int startSec = Stats::PrevodCasuNaSekundy(body.front().time);
        int endSec = Stats::PrevodCasuNaSekundy(body.back().time);

        int rozdil = endSec - startSec;

        // Kdyby aktivita presla pres pulnoc, cas by byl zaporny -> osetrime.
        if (rozdil < 0) rozdil += 24 * 3600;

        // Kdyby i tak byl zaporny (divna data), dame 0
        if (rozdil < 0) rozdil = 0;

        akt.setTotalTimeSeconds(rozdil);
    }
    else {
        akt.setTotalTimeSeconds(0);
    }
}

// ---------------------------------------------------------
// Výpis souhrnu jedné aktivity
// ---------------------------------------------------------
void VypisSouhrnAktivity(const Activity& akt)
{
    cout << fixed << setprecision(2);
    cout << "Aktivita: " << akt.getName() << "\n";
    cout << "  Vzdalenost: " << akt.getTotalDistanceKm() << " km\n";
    cout << "  Cas: " << akt.getTotalTimeSeconds() / 60.0 << " min\n";
    cout << "  Prumerne tempo: " << akt.getAveragePaceMinPerKm() << " min/km\n";
}

// ---------------------------------------------------------
// Načtení všech GPX aktivit ze složky (např. "data")
// ---------------------------------------------------------
void NactiAktivityZeSlozky(const string& slozka, vector<Activity>& historie)
{
    if (!fs::exists(slozka)) {
        cout << "Slozka \"" << slozka << "\" neexistuje.\n";
        return;
    }

    int pocetNalezenych = 0;

    for (const auto& entry : fs::directory_iterator(slozka)) {
        if (!entry.is_regular_file()) continue;

        auto path = entry.path();
        if (path.extension() == ".gpx") {
            string cesta = path.string();
            string nazev = path.stem().string();

            Activity akt = GpxParser::NactiAktivituZeSouboru(cesta, nazev);
            SpocitejStatistikyAktivity(akt);
            historie.push_back(akt);
            ++pocetNalezenych;
        }
    }

    cout << "Nacteno " << pocetNalezenych << " aktivit ze slozky \"" << slozka << "\".\n";
}

// ---------------------------------------------------------
// Výpis kilometrových úseků (splitů) aktivity
// ---------------------------------------------------------
void VypisKilometroveUseky(const Activity& akt)
{
    auto splity = Stats::SpocitejKilometroveUseky(akt);
    if (splity.empty()) {
        cout << "Zadne kilometrove useky (malo bodu nebo kratka trasa).\n";
        return;
    }

    cout << "Kilometrove useky:\n";
    cout << fixed << setprecision(2);

    for (const auto& s : splity) {
        double minuty = s.tempoMinNaKm;
        int m = static_cast<int>(minuty);
        int sec = static_cast<int>(round((minuty - m) * 60.0));
        if (sec == 60) { sec = 0; m++; }

        cout << "  " << s.kmPoradi << ". km"
            << "  vzdalenost: " << s.vzdalenostKm << " km"
            << "  cas: " << s.casSekundy / 60.0 << " min"
            << "  tempo: " << m << ":" << setw(2) << setfill('0')
            << sec << " min/km\n";
    }
}

// ---------------------------------------------------------
// Otevření TRASY aktivity na Mapy.com (route)
// - start + end + max 15 waypointů
// - Mapy očekávají souřadnice ve formátu lon,lat
// ---------------------------------------------------------
static string BuildMapyRouteUrl(const Activity& akt, int maxWaypoints = 15)
{
    const auto& pts = akt.getPoints();
    if (pts.size() < 2) return "";

    auto fmt = [](const TrackPoint& p) {
        ostringstream os;
        os << fixed << setprecision(6)
            << p.longitude << "," << p.latitude; // lon,lat !!!
        return os.str();
        };

    const TrackPoint& start = pts.front();
    const TrackPoint& end = pts.back();

    vector<size_t> idx;
    size_t n = pts.size();

    int k = maxWaypoints;
    if (n <= 2) k = 0;
    if (n - 2 < (size_t)k) k = (int)(n - 2);

    // rovnoměrně vybereme body z vnitřku (bez start/end)
    for (int i = 1; i <= k; ++i) {
        size_t pos = (size_t)((double)i * (double)(n - 1) / (double)(k + 1));
        if (pos == 0) pos = 1;
        if (pos >= n - 1) pos = n - 2;
        idx.push_back(pos);
    }

    ostringstream url;
    url << "https://mapy.com/fnc/v1/route"
        << "?routeType=foot_fast"
        << "&start=" << fmt(start)
        << "&end=" << fmt(end);

    if (!idx.empty()) {
        url << "&waypoints=";
        for (size_t i = 0; i < idx.size(); ++i) {
            if (i) url << ";";
            url << fmt(pts[idx[i]]);
        }
    }

    return url.str();
}

void OtevriTrasouNaMapyCom(const Activity& akt)
{
    string url = BuildMapyRouteUrl(akt);
    if (url.empty()) {
        cout << "Aktivita nema dost bodu pro trasu.\n";
        return;
    }

    string cmd = "start \"\" \"" + url + "\"";
    system(cmd.c_str());
}

// ---------------------------------------------------------
// Získání data (YYYY-MM-DD) aktivity z prvního bodu
// ---------------------------------------------------------
string ZiskejDatumAktivity(const Activity& akt)
{
    const auto& body = akt.getPoints();
    if (body.empty()) return "N/A";

    const string& t = body.front().time;
    if (t.size() < 10) return "N/A";
    return t.substr(0, 10);
}

// ---------------------------------------------------------
// Trend tempa podle dní
// ---------------------------------------------------------
void VypisTempoTrenduPodleDnu(const vector<Activity>& historie)
{
    if (historie.empty()) {
        cout << "Zadne aktivity.\n";
        return;
    }

    map<string, pair<double, int>> denNaTempo;

    for (const auto& akt : historie) {
        if (akt.getTotalDistanceKm() <= 0.0 || akt.getTotalTimeSeconds() <= 0.0)
            continue;

        string datum = ZiskejDatumAktivity(akt);
        double tempo = akt.getAveragePaceMinPerKm();

        auto& ref = denNaTempo[datum];
        ref.first += tempo;
        ref.second += 1;
    }

    if (denNaTempo.empty()) {
        cout << "Neni dost dat pro vypocet trendu.\n";
        return;
    }

    cout << "\n--- Trend tempa podle dnu ---\n";
    cout << fixed << setprecision(2);

    for (const auto& [datum, data] : denNaTempo) {
        double avgTempo = data.first / data.second;
        cout << datum << "  prumerne tempo: " << avgTempo
            << " min/km"
            << "  (" << data.second << " behu)\n";
    }
}

// ---------------------------------------------------------
// MAIN
// ---------------------------------------------------------
int main()
{
    vector<Activity> historie;

    // 1) Automaticky nacist vsechny .gpx ze slozky "data"
    NactiAktivityZeSlozky("data", historie);

    int volba = 0;

    while (true)
    {
        cout << "\n==== BEZECKA ANALYZA (mini Strava v C++) ====\n";
        cout << "1) Vypsat vsechny aktivity\n";
        cout << "2) Zobrazit kilometrove useky vybrane aktivity\n";
        cout << "3) Otevrit vybranou aktivitu na Mapy.com (trasa)\n";
        cout << "4) Zobrazit trend tempa podle dnu\n";
        cout << "5) Konec\n";
        cout << "Zadej volbu: ";
        cin >> volba;

        if (!cin) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            cout << "Neplatny vstup, zkus to znovu.\n";
            continue;
        }

        if (volba == 1) {
            if (historie.empty()) {
                cout << "Zadne aktivity v historii.\n";
            }
            else {
                cout << "\n--- Historie aktivit ---\n";
                for (size_t i = 0; i < historie.size(); ++i) {
                    cout << "[" << (i + 1) << "] ";
                    VypisSouhrnAktivity(historie[i]);
                }
            }
        }
        else if (volba == 2) {
            if (historie.empty()) {
                cout << "Zadne aktivity.\n";
            }
            else {
                cout << "Zadej cislo aktivity: ";
                size_t idx;
                cin >> idx;
                if (idx == 0 || idx > historie.size()) {
                    cout << "Neplatne cislo.\n";
                }
                else {
                    VypisKilometroveUseky(historie[idx - 1]);
                }
            }
        }
        else if (volba == 3) {
            if (historie.empty()) {
                cout << "Zadne aktivity.\n";
            }
            else {
                cout << "Zadej cislo aktivity: ";
                size_t idx;
                cin >> idx;
                if (idx == 0 || idx > historie.size()) {
                    cout << "Neplatne cislo.\n";
                }
                else {
                    OtevriTrasouNaMapyCom(historie[idx - 1]);
                }
            }
        }
        else if (volba == 4) {
            VypisTempoTrenduPodleDnu(historie);
        }
        else if (volba == 5) {
            cout << "Ukoncuji program.\n";
            break;
        }
        else {
            cout << "Neplatna volba.\n";
        }
    }

    return 0;
}
