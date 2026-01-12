#include "Activity.h"
using namespace std;

Activity::Activity(const std::string& name)
    : NazevAktivity(name), CelkovaVzdalenostKm(0.0), CelkovyCasSekundy(0.0) {
}

void Activity::addPoint(const TrackPoint& p) {
    points_.push_back(p);
}

const std::vector<TrackPoint>& Activity::getPoints() const {
    return points_;
}

void Activity::setTotalDistanceKm(double d) {
    CelkovaVzdalenostKm = d;
}

void Activity::setTotalTimeSeconds(double t) {
    CelkovyCasSekundy = t;
}

double Activity::getTotalDistanceKm() const {
    return CelkovaVzdalenostKm;
}

double Activity::getTotalTimeSeconds() const {
    return CelkovyCasSekundy;
}

double Activity::getAveragePaceMinPerKm() const {
    if (CelkovaVzdalenostKm <= 0.0) return 0.0;
    double minutes = CelkovyCasSekundy / 60.0;
    return minutes / CelkovaVzdalenostKm;
}

const std::string& Activity::getName() const {
    return NazevAktivity;
}
