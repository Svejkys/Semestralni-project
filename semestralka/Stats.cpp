#include "Stats.h"
#include <cmath>

namespace {
    constexpr double DEG_TO_RAD = 3.14159265358979323846 / 180.0;
    constexpr double EARTH_RADIUS_KM = 6371.0;
}

namespace Stats {

    // Haversinova formule pro výpoèet vzdálenosti mezi dvìma GPS body
    double VzdalenostMeziBody(const TrackPoint& a, const TrackPoint& b)
    {
        double lat1 = a.latitude * DEG_TO_RAD;
        double lon1 = a.longitude * DEG_TO_RAD;
        double lat2 = b.latitude * DEG_TO_RAD;
        double lon2 = b.longitude * DEG_TO_RAD;

        double dLat = lat2 - lat1;
        double dLon = lon2 - lon1;

        double sinLat = std::sin(dLat / 2.0);
        double sinLon = std::sin(dLon / 2.0);

        double hav = sinLat * sinLat +
            std::cos(lat1) * std::cos(lat2) * sinLon * sinLon;

        double c = 2.0 * std::atan2(std::sqrt(hav), std::sqrt(1.0 - hav));

        return EARTH_RADIUS_KM * c;
    }

    // Spoèítá celkovou délku trasy v kilometrech
    double SpocitejCelkovoVzdalenostKm(const std::vector<TrackPoint>& body)
    {
        if (body.size() < 2)
            return 0.0;

        double soucet = 0.0;

        for (size_t i = 1; i < body.size(); ++i) {
            soucet += VzdalenostMeziBody(body[i - 1], body[i]);
        }

        return soucet;
    }

    int PrevodCasuNaSekundy(const std::string& cas)
    {
        // Oèekávaný formát: 2023-01-01T12:34:56Z

        if (cas.size() < 19)
            return 0;

        int hodiny = std::stoi(cas.substr(11, 2));  // znaky 11–12
        int minuty = std::stoi(cas.substr(14, 2));  // znaky 14–15
        int sekundy = std::stoi(cas.substr(17, 2)); // znaky 17–18

        return hodiny * 3600 + minuty * 60 + sekundy;
    }

    std::vector<KmSplit> SpocitejKilometroveUseky(const Activity& akt)
    {
        std::vector<KmSplit> splity;
        const auto& body = akt.getPoints();
        if (body.size() < 2) return splity;

        double segmentDist = 0.0;
        double segmentTime = 0.0;
        int kmPoradi = 1;

        int lastSec = PrevodCasuNaSekundy(body.front().time);

        for (size_t i = 1; i < body.size(); ++i) {
            double d = VzdalenostMeziBody(body[i - 1], body[i]);
            int curSec = PrevodCasuNaSekundy(body[i].time);
            int dt = curSec - lastSec;
            if (dt < 0) dt = 0;
            lastSec = curSec;

            segmentDist += d;
            segmentTime += dt;

            if (segmentDist >= 1.0) { // máme cca 1 km
                KmSplit split;
                split.kmPoradi = kmPoradi++;
                split.vzdalenostKm = segmentDist;
                split.casSekundy = segmentTime;
                if (segmentDist > 0.0) {
                    double minuty = segmentTime / 60.0;
                    split.tempoMinNaKm = minuty / segmentDist;
                }
                else {
                    split.tempoMinNaKm = 0.0;
                }

                splity.push_back(split);

                // zaèneme další úsek
                segmentDist = 0.0;
                segmentTime = 0.0;
            }
        }

        // poslední "nedokonèený" úsek, pokud není úplnì malý
        if (segmentDist > 0.2) {
            KmSplit split;
            split.kmPoradi = kmPoradi;
            split.vzdalenostKm = segmentDist;
            split.casSekundy = segmentTime;
            if (segmentDist > 0.0) {
                double minuty = segmentTime / 60.0;
                split.tempoMinNaKm = minuty / segmentDist;
            }
            else {
                split.tempoMinNaKm = 0.0;
            }
            splity.push_back(split);
        }

        return splity;
    }

} // namespace Stats
