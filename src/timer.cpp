#include "timer.h"
#include "unittest.h"

#include <string>
#include <sstream>

using std::string;
using std::endl;
using std::vector;
using std::stringstream;

bool TIMER::Load(const std::string & trackrecordspath, float stagingtime)
{
	Unload();

	car.clear();

	pretime = stagingtime;

	trackrecordsfile = trackrecordspath;

	trackrecords.Load(trackrecordsfile);

	loaded = true;

	return true;
}

int TIMER::AddCar(const std::string & cartype)
{
	car.push_back(LAPINFO(cartype));
	return car.size()-1;
}

void TIMER::Unload()
{
	if (loaded)
	{
		trackrecords.Write(trackrecordsfile);
	}
	trackrecords.Clear();
	loaded = false;
}

void TIMER::Tick(float dt)
{
	float elapsed_time = dt;

    pretime -= dt;

	if (pretime > 0)
	{
		elapsed_time -= dt;
	}

	/*if (pretime < 0)
	{
		elapsed_time += -pretime;
	}*/

	assert(elapsed_time >= 0);

	for (vector <LAPINFO>::iterator i = car.begin(); i != car.end(); ++i)
		i->Tick(elapsed_time);
}

void TIMER::Lap(const unsigned int carid, const int nextsector, const bool countit)
{
	assert(carid < car.size());

	if (countit && carid == playercarindex)
	{
		stringstream secstr;
		secstr << "sector " << nextsector;
		string lastcar;
		/*if (trackrecords.GetParam("last.car", lastcar))
		{
			if (lastcar != car[carid].GetCarType()) //clear last lap time
			trackrecords.SetParam("last.sector 0", (float)0.0);
		}*/
		trackrecords.SetParam("last", secstr.str(), (float) car[carid].GetTime());
		trackrecords.SetParam("last", "car", car[carid].GetCarType());

		float prevbest = 0;
		bool haveprevbest = trackrecords.GetParam(car[carid].GetCarType(), secstr.str(), prevbest);
		if (car[carid].GetTime() < prevbest || !haveprevbest)
			trackrecords.SetParam(car[carid].GetCarType(), secstr.str(), (float) car[carid].GetTime());
	}

	if (nextsector == 0)
		car[carid].Lap(countit);
}

void TIMER::UpdateDistance(const unsigned int carid, const double newdistance)
{
	assert(carid < car.size());
	car[carid].UpdateLapDistance(newdistance);
}

void TIMER::DebugPrint(std::ostream & out) const
{
	for (unsigned int i = 0; i < car.size(); ++i)
	{
		out << i << ". ";
		car[i].DebugPrint(out);
	}
}

class PLACE
{
    private:
        int index;
        int laps;
        double distance;

    public:
        PLACE(int newindex, int newlaps, double newdistance) : index(newindex), laps(newlaps), distance(newdistance) {}

        int GetIndex() const {return index;}

        bool operator< (const PLACE & other) const
        {
            if (laps == other.laps)
                return distance > other.distance;
            else
                return laps > other.laps;
        }
};

std::pair <int, int> TIMER::GetCarPlace(int index)
{
    assert(index<(int)car.size());
    assert(index >= 0);

    int place = 1;
    int total = car.size();

    std::list <PLACE> distances;

    for (int i = 0; i < (int)car.size(); i++)
    {
        distances.push_back(PLACE(i, car[i].GetCurrentLap(), car[i].GetLapDistance()));
    }

    distances.sort();

    int curplace = 1;
    for (std::list <PLACE>::iterator i = distances.begin(); i != distances.end(); ++i)
    {
        if (i->GetIndex() == index)
            place = curplace;

        curplace++;
    }

    return std::make_pair(place, total);
}
