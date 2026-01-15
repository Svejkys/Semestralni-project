
#pragma once
#include <string>
#include <vector>
using namespace std;

//jeden zaznam GPS bodu
struct TrackPoint {
	double latitude; //zem.sirka
	double longitude;//zem.delka
	double elevation;//nadm.vyska   
	string time;     //cas
};

//aktivita (bìh)
class Activity {
public:
	//vytvori aktivitu s nazvem
    Activity(const string& name);
    const string& getName() const;
    void addPoint(const TrackPoint& p);
    const vector<TrackPoint>& getPoints() const;
	//nastavi celkovou vzdalenost a cas aktivity
    void setTotalDistanceKm(double d);
    void setTotalTimeSeconds(double t);
    //celkova vzdalenost
    double getTotalDistanceKm() const;
    //celkovy cas
    double getTotalTimeSeconds() const;
	//vypocitane prumerne tempo z tech hodnot
    double getAveragePaceMinPerKm() const;

private:
    string NazevAktivity;
    vector<TrackPoint> points_;
    double CelkovaVzdalenostKm;
    double CelkovyCasSekundy;
};
