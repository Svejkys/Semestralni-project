#include "GpxParser.h"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;


static bool NactiAtributDouble(const string& radek,
    const string& klic,
    double& vystup)
{
    string hledat = klic + "=\""; 
    size_t start = radek.find(hledat);
    if (start == string::npos)
        return false;

    start += hledat.size();
    size_t end = radek.find("\"", start);
    if (end == string::npos)
        return false;

    string cisloText = radek.substr(start, end - start);
    try {
        vystup = stod(cisloText);
    }
    catch (...) {
        return false;
    }

    return true;
}

Activity GpxParser::NactiAktivituZeSouboru(const string& cestaKSouboru,
    const string& nazevAktivity)
{
    Activity aktivita(nazevAktivity);

    ifstream soubor(cestaKSouboru);
    if (!soubor) {
        cerr << "Nepodarilo se otevrit soubor: " << cestaKSouboru << endl;
        return aktivita; // vrátíme prázdnou aktivitu
    }

    string radek;
    int pocitadloBodu = 0;

    while (getline(soubor, radek)) {
        // Hledáme øádky s <trkpt lat="..." lon="...">
        if (radek.find("<trkpt") != string::npos) {
            TrackPoint bod{};
            bod.elevation = 0.0;
            bod.time = "";

            // Zkusit naèíst lat a lon
            bool latOk = NactiAtributDouble(radek, "lat", bod.latitude);
            bool lonOk = NactiAtributDouble(radek, "lon", bod.longitude);

            if (!latOk || !lonOk) {
                // Když se nepodaøí naèíst, tento bod pøeskoèíme
                continue;
            }

            // Teï naèteme následující øádky až po </trkpt>
            // a zkusíme z nich vytáhnout <ele> a <time>
            streampos pozicePredDalsimRadkem = soubor.tellg();

            while (getline(soubor, radek)) {
                if (radek.find("<ele>") != string::npos) {
                    size_t start = radek.find("<ele>");
                    size_t end = radek.find("</ele>");
                    if (start != string::npos && end != string::npos && end > start) {
                        start += 5; // délka "<ele>"
                        string eleText = radek.substr(start, end - start);
                        try {
                            bod.elevation = stod(eleText);
                        }
                        catch (...) {
                            bod.elevation = 0.0;
                        }
                    }
                }
                else if (radek.find("<time>") != string::npos) {
                    size_t start = radek.find("<time>");
                    size_t end = radek.find("</time>");
                    if (start != string::npos && end != string::npos && end > start) {
                        start += 6; // délka "<time>"
                        bod.time = radek.substr(start, end - start);
                    }
                }
                else if (radek.find("</trkpt>") != string::npos) {
                    // konec jednoho bodu
                    break;
                }

                // Uložíme pozici pro pøípadné další ètení
                pozicePredDalsimRadkem = soubor.tellg();
            }

            aktivita.addPoint(bod);
            pocitadloBodu++;
        }
    }

    return aktivita;
}
