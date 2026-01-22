#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <iomanip>
#include <sstream>
#include "Activity.h"
#include "GpxParser.h"
#include "Stats.h"
#include "GpxExporter.h"

using namespace std;
namespace fs = std::filesystem;

static string FormatCasHhMmSs(long long sekundy) {
    if (sekundy < 0) sekundy = 0;
    long long h = sekundy / 3600;
    long long m = (sekundy % 3600) / 60;
    long long s = sekundy % 60;

    ostringstream oss;
    oss << h << ":" << setw(2) << setfill('0') << m << ":" << setw(2) << setfill('0') << s;
    return oss.str();
}

// cas ve formatu "rok-mesic-den hodiny-minuty-sekundy"
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

static string DatumCasZIso(const string& iso) {
    // vypise cas pokud to jde
    if (iso.size() < 19) return "neznamy cas";
    return iso.substr(0, 10) + " " + iso.substr(11, 8);
}

static void SpocitejStatistikyAktivity(Activity& akt) {
    const auto& body = akt.getPoints();
    if (body.size() < 2) {
        akt.setTotalDistanceKm(0.0);
        akt.setTotalTimeSeconds(0.0);
        return;
    }

    // vzdálenost
    double vzd = Stats::SpocitejCelkovoVzdalenostKm(body);
    akt.setTotalDistanceKm(vzd);

    // čas z prvního a posledního bodu (sekundy dne, ošetření půlnoci)
    long long t0 = SekundyDneZIsoCasu(body.front().time);
    long long t1 = SekundyDneZIsoCasu(body.back().time);
    if (t1 < t0) t1 += 24LL * 3600LL; // přechod přes půlnoc

    akt.setTotalTimeSeconds((double)(t1 - t0));
}

static vector<Activity> NactiAktivityZeSlozkyData(const fs::path& slozkaData) {
    vector<Activity> aktivity;

    if (!fs::exists(slozkaData)) {
        cerr << "Slozka neexistuje: " << slozkaData.string() << "\n";
        return aktivity;
    }

    for (const auto& entry : fs::directory_iterator(slozkaData)) {
        if (!entry.is_regular_file()) continue;

        fs::path p = entry.path();
        if (p.extension() != ".gpx") continue;

        string nazev = p.stem().string();
        Activity akt = GpxParser::NactiAktivituZeSouboru(p.string(), nazev);
        SpocitejStatistikyAktivity(akt);
        aktivity.push_back(std::move(akt));
    }

    return aktivity;
}

static void SeradAktivityPodleData(vector<Activity>& aktivity) {
    auto casZacatku = [](const Activity& a) -> string {
        const auto& pts = a.getPoints();
        if (pts.empty()) return "";
        return pts.front().time;
        };

    sort(aktivity.begin(), aktivity.end(),
        [&](const Activity& a, const Activity& b) {
            return casZacatku(a) < casZacatku(b);
        });
}

static void VypisSeznamAktivit(const vector<Activity>& aktivity) {
    if (aktivity.empty()) {
        cout << "Zadne aktivity nejsou nactene.\n";
        return;
    }

    cout << "\n--- Seznam aktivit ---\n";
    for (size_t i = 0; i < aktivity.size(); ++i) {
        const auto& a = aktivity[i];
        const auto& pts = a.getPoints();

        string start = pts.empty() ? "neznamy cas" : DatumCasZIso(pts.front().time);

        cout << "[" << i << "] " << a.getName()
            << " | " << fixed << setprecision(2) << a.getTotalDistanceKm() << " km"
            << " | cas: " << FormatCasHhMmSs((long long)a.getTotalTimeSeconds())
            << " | prumerne tempo: " << Stats::FormatTempoMmSsNaKm(a.getAveragePaceMinPerKm())
            << "\n";
    }
}


static void VypisDetailAktivity(const Activity& akt) {
    cout << "\n--- Detail aktivity: " << akt.getName() << " ---\n";

    const auto& pts = akt.getPoints();
    if (!pts.empty()) {
        cout << "Start: " << DatumCasZIso(pts.front().time) << "\n";
    }

    cout << "Vzdalenost: " << fixed << setprecision(2) << akt.getTotalDistanceKm() << " km\n";
    cout << "Cas: " << FormatCasHhMmSs((long long)akt.getTotalTimeSeconds()) << "\n";
    cout << "Prumerne tempo: " << Stats::FormatTempoMmSsNaKm(akt.getAveragePaceMinPerKm()) << "\n";

    
    vector<Stats::KmSplit> splity = Stats::ZiskejVsechnyKmSplity(akt);

    if (splity.empty()) {
        cout << "Splity: nelze spocitat (malo bodu / kratka aktivita)\n";
    }
    else {
        cout << "\n--- Km splity ---\n";
        for (const auto& s : splity) {
            cout << s.kmPoradi << ". km split: "
                << Stats::FormatTempoMmSsNaKm(s.tempoMinNaKm)
                << "\n";
        }

        
        auto bestIt = min_element(splity.begin(), splity.end(),
            [](const Stats::KmSplit& a, const Stats::KmSplit& b) {
                return a.tempoMinNaKm < b.tempoMinNaKm;
            });

        cout << "Nejlepsi split (km " << bestIt->kmPoradi << "): "
            << Stats::FormatTempoMmSsNaKm(bestIt->tempoMinNaKm)
            << "\n";
    }

}

static void VypisTydenniTrend(const vector<Activity>& aktivity) {
    auto trend = Stats::SpocitejTydenniTrend(aktivity);
	//osetreni prazdneho trendu
    if (trend.empty()) {
        cout << "Trend je prazdny (zadna data / chyba v casech).\n";
        return;
    }

    cout << "\n--- Tydenni trend ---\n";
    for (const auto& [klic, t] : trend) {
        cout << klic.isoRok << " Tyden c." << (klic.isoTyden < 10 ? "0" : "") << klic.isoTyden
            << ": " << t.pocetBehu << " behu, "
            << fixed << setprecision(2) << t.celkemKm << " km, tempo "
            << Stats::FormatTempoMmSsNaKm(t.PrumerneTempoMinNaKm())
            << "\n";
    }
}

int main() {
    ios::sync_with_stdio(false);
    cin.tie(nullptr);

    fs::path slozkaData = fs::current_path() / "data";

    vector<Activity> aktivity = NactiAktivityZeSlozkyData(slozkaData);
    SeradAktivityPodleData(aktivity);

    while (true) {
        cout << "\n=============================\n";
        cout << "        Strava Projekt";
        cout << "\n=============================\n";
        cout << "1) Vypsat seznam aktivit\n";
        cout << "2) Detail aktivity\n";
        cout << "3) Tydenni trend\n";
        cout << "4) Znovu nacist aktivity\n";
        cout << "5) Export aktivity pro Mapy.cz (GPX)\n";
        cout << "0) Konec\n";
        cout << "Volba: ";

        int volba = -1;
        if (!(cin >> volba)) {
            cout << "Neplatny vstup.\n";
            return 0;
        }

        if (volba == 0) {
            cout << "Konec.\n";
            break;
        }
        else if (volba == 1) {
            VypisSeznamAktivit(aktivity);
        }
        else if (volba == 2) {
            if (aktivity.empty()) {
                cout << "Zadne aktivity.\n";
                continue;
            }
            cout << "Zadej index aktivity: ";
            int idx;
            cin >> idx;
            if (idx < 0 || idx >= (int)aktivity.size()) {
                cout << "Spatny index.\n";
                continue;
            }
            VypisDetailAktivity(aktivity[idx]);
        }
        else if (volba == 3) {
            VypisTydenniTrend(aktivity);
        }
        else if (volba == 4) {
            aktivity = NactiAktivityZeSlozkyData(slozkaData);
            SeradAktivityPodleData(aktivity);
            cout << "Nacteno: " << aktivity.size() << " aktivit.\n";
        }
        else if (volba == 5) {
            if (aktivity.empty()) {
                cout << "Zadne aktivity.\n";
                continue;
            }
            cout << "Zadej index aktivity pro export: ";
            int idx;
            cin >> idx;
            if (idx < 0 || idx >= (int)aktivity.size()) {
                cout << "Spatny index.\n";
                continue;
            }
            fs::path outPath = fs::current_path() / (aktivity[idx].getName() + "_export.gpx");
            try {
                GpxExporter::ExportToGpx(aktivity[idx], outPath.string());
                cout << "Aktivita exportovana do: " << outPath.string() << "\n";
            }
            catch (const exception& ex) {
                cout << "Chyba pri exportu: " << ex.what() << "\n";
            }
		}
        else {
            cout << "Neznama volba.\n";
        }
    }

    return 0;
}
