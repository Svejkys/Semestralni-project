#include "GpxExporter.h"
#include <fstream>
#include <iomanip>
#include <stdexcept>
// pomocna funkce pro kontrolu, jestli souradnice vypadaji jako stupne
static bool LooksLikeDegrees(double lat, double lon) {
    return (lat >= -90.0 && lat <= 90.0 && lon >= -180.0 && lon <= 180.0);
}
// export aktivity do GPX souboru, ktery se ulozi na outPath
void GpxExporter::ExportToGpx(const Activity& activity, const std::string& outPath) {
    std::ofstream out(outPath);
    if (!out.is_open()) {
        throw std::runtime_error("Nelze otevrit soubor pro zapis: " + outPath);
    }

    const auto& pts = activity.getPoints();
    //osetreni
    if (pts.empty()) {
        throw std::runtime_error("Aktivita nema zadne body (TrackPoint).");
    }

	//kontrola souradnic
    if (!LooksLikeDegrees(pts.front().latitude, pts.front().longitude)) {
        throw std::runtime_error(
            "Souradnice nevypadaji jako stupne (lat/lon mimo rozsah). "
            "Mozna mas lat/lon prohozene nebo jsou v radianech."
        );
    }

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<gpx version=\"1.1\" creator=\"SemestralniProjekt\" "
        "xmlns=\"http://www.topografix.com/GPX/1/1\" "
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
        "xsi:schemaLocation=\"http://www.topografix.com/GPX/1/1 "
        "http://www.topografix.com/GPX/1/1/gpx.xsd\">\n";
    //urceni nazvu pro mapy.cz
    out << "  <trk>\n";
    out << "    <name>" << activity.getName() << "</name>\n";
    out << "    <trkseg>\n";
	//nastaveni formatu pro cisla
    out << std::fixed << std::setprecision(8);
	//prochazeni vsech bodu aktivity
    for (const auto& p : pts) {
        out << "      <trkpt lat=\"" << p.latitude << "\" lon=\"" << p.longitude << "\">";

        if (!p.time.empty()) {
            out << "<time>" << p.time << "</time>";
        }

        out << "</trkpt>\n";
	}

    out << "    </trkseg>\n";
    out << "  </trk>\n";
    out << "</gpx>\n";
}
