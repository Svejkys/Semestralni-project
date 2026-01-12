
#pragma once
#include <string>
#include <vector>
using namespace std;

// Jeden záznam GPS bodu
struct TrackPoint {
    double latitude;
    double longitude;
    double elevation;   
    string time;   
};

// Jedna aktivita (bìh)
class Activity {
public:
    Activity(const string& name);

    void addPoint(const TrackPoint& p);
    const vector<TrackPoint>& getPoints() const;

    void setTotalDistanceKm(double d);
    void setTotalTimeSeconds(double t);

    double getTotalDistanceKm() const;
    double getTotalTimeSeconds() const;
    double getAveragePaceMinPerKm() const;

    const string& getName() const;

private:
    string NazevAktivity;
    vector<TrackPoint> points_;
    double CelkovaVzdalenostKm;
    double CelkovyCasSekundy;
};
