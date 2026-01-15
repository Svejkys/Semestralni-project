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
{   //vytvoreni aktivity
    Activity aktivita(nazevAktivity);
    //otevreni gpx souboru jako textoveho souboru
    ifstream soubor(cestaKSouboru);
    if (!soubor) {
        cerr << "Nepodarilo se otevrit soubor: " << cestaKSouboru << endl;
        return aktivita; 
    }

    string radek;
    int pocitadloBodu = 0;
	//cteni souboru radek po radku
    while (getline(soubor, radek)) {
        //hledani zacatku bodu trasy
        if (radek.find("<trkpt") != string::npos) {
            //vytvoreni trackpointu pro ten bod
            TrackPoint bod{};
            bod.elevation = 0.0;
            bod.time = "";

			//nacteni atributu lat a lon
            bool latOk = NactiAtributDouble(radek, "lat", bod.latitude);
            bool lonOk = NactiAtributDouble(radek, "lon", bod.longitude);
            
            if (!latOk || !lonOk) {
                continue;
            }

            
            streampos pozicePredDalsimRadkem = soubor.tellg();
            //vyska
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
                //cas
                else if (radek.find("<time>") != string::npos) {
                    size_t start = radek.find("<time>");
                    size_t end = radek.find("</time>");
                    if (start != string::npos && end != string::npos && end > start) {
                        start += 6; 
                        bod.time = radek.substr(start, end - start);
                    }
                }
				//konec bodu
                else if (radek.find("</trkpt>") != string::npos) {
                    // konec jednoho bodu
                    break;
                }

                
                pozicePredDalsimRadkem = soubor.tellg();
            }
            //ulozit bod do aktivity
            aktivita.addPoint(bod);
            pocitadloBodu++;
        }
    }

    return aktivita;
}
