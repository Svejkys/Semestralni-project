#pragma once
#include <string>
#include "Activity.h"
using namespace std;

class GpxParser {
public:
    // Naète GPX soubor a vytvoøí novou aktivitu s body
    static Activity NactiAktivituZeSouboru(const string& cestaKSouboru, const string& nazevAktivity);
};

