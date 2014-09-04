/************************************************************************/
/*                                                                      */
/* This file is part of VDrift.                                         */
/*                                                                      */
/* VDrift is free software: you can redistribute it and/or modify       */
/* it under the terms of the GNU General Public License as published by */
/* the Free Software Foundation, either version 3 of the License, or    */
/* (at your option) any later version.                                  */
/*                                                                      */
/* VDrift is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY; without even the implied warranty of       */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the        */
/* GNU General Public License for more details.                         */
/*                                                                      */
/* You should have received a copy of the GNU General Public License    */
/* along with VDrift.  If not, see <http://www.gnu.org/licenses/>.      */
/*                                                                      */
/************************************************************************/

#include "timer.h"
#include "unittest.h"

#include <string>
#include <sstream>

using std::string;
using std::endl;
using std::vector;
using std::ostringstream;

bool Timer::Load(const std::string & trackrecordspath, float stagingtime)
{
	Unload();

	car.clear();

	pretime = stagingtime;

	trackrecordsfile = trackrecordspath;

	trackrecords.load(trackrecordsfile);

	loaded = true;

	return true;
}

int Timer::AddCar(const std::string & cartype)
{
	car.push_back(LapInfo(cartype));
	return car.size()-1;
}

void Timer::Unload()
{
	if (loaded)
	{
		trackrecords.write(trackrecordsfile);
	}
	trackrecords.clear();
	loaded = false;
}

void Timer::Tick(float dt)
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

	for (vector <LapInfo>::iterator i = car.begin(); i != car.end(); ++i)
		i->Tick(elapsed_time);
}

void Timer::Lap(const unsigned int carid, const int nextsector)
{
	assert(carid < car.size());

	bool countlap = (car[carid].GetSector() >= 0);
	if (countlap && carid == playercarindex)
	{
		ostringstream secstr;
		secstr << "sector " << nextsector;
		string lastcar;
		/*if (trackrecords.get("last.car", lastcar))
		{
			if (lastcar != car[carid].GetCarType()) //clear last lap time
			trackrecords.set("last.sector 0", (float)0.0);
		}*/
		trackrecords.set("last", secstr.str(), (float) car[carid].GetTime());
		trackrecords.set("last", "car", car[carid].GetCarType());

		float prevbest = 0;
		bool haveprevbest = trackrecords.get(car[carid].GetCarType(), secstr.str(), prevbest);
		if (car[carid].GetTime() < prevbest || !haveprevbest)
			trackrecords.set(car[carid].GetCarType(), secstr.str(), (float) car[carid].GetTime());
	}

	car[carid].SetSector(nextsector);
	if (nextsector == 0)
		car[carid].Lap(countlap);
}

void Timer::UpdateDistance(const unsigned int carid, const double newdistance)
{
	assert(carid < car.size());
	car[carid].UpdateLapDistance(newdistance);
}

void Timer::DebugPrint(std::ostream & out) const
{
	for (unsigned int i = 0; i < car.size(); ++i)
	{
		out << i << ". ";
		car[i].DebugPrint(out);
	}
}

class Place
{
private:
	int index;
	int laps;
	double distance;

public:
	Place(int newindex, int newlaps, double newdistance) : index(newindex), laps(newlaps), distance(newdistance) {}

	int GetIndex() const {return index;}

	bool operator< (const Place & other) const
	{
		if (laps == other.laps)
			return distance > other.distance;
		else
			return laps > other.laps;
	}
};

std::pair <int, int> Timer::GetCarPlace(int index)
{
    assert(index<(int)car.size());
    assert(index >= 0);

    int place = 1;
    int total = car.size();

	std::list <Place> distances;

    for (int i = 0; i < (int)car.size(); i++)
    {
		distances.push_back(Place(i, car[i].GetCurrentLap(), car[i].GetLapDistance()));
    }

    distances.sort();

    int curplace = 1;
	for (std::list <Place>::iterator i = distances.begin(); i != distances.end(); ++i)
    {
        if (i->GetIndex() == index)
            place = curplace;

        curplace++;
    }

    return std::make_pair(place, total);
}
