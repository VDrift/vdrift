#include "timer.h"
#include "unittest.h"
#include <string>
#include <sstream>

using std::string;
using std::endl;
using std::vector;
using std::stringstream;

TIMER::TIMER() : pretime(0.0), playercarindex(0), loaded(false)
{
	// Constructor.
}

TIMER::~TIMER()
{
	Unload();
}

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

void TIMER::SetPlayerCarID(int newid)
{
	playercarindex = newid;
}

void TIMER::Unload()
{
	if (loaded)
	{
		trackrecords.Write(true, trackrecordsfile);
		//std::cout << "Writing records to: " << trackrecordsfile << endl;
	}
	trackrecords.Clear();
	loaded = false;
}

bool TIMER::Staging() const
{
	return (pretime > 0);
}

void TIMER::Tick(float dt)
{
	float elapsed_time = dt;

	pretime -= dt;

	if (pretime > 0)
	{
		elapsed_time -= dt;
	}

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

float TIMER::GetPlayerTime()
{
	assert(playercarindex<car.size());
	return car[playercarindex].GetTime();
}

float TIMER::GetLastLap()
{
	assert(playercarindex<car.size());
	return car[playercarindex].GetLastLap();
}

float TIMER::GetBestLap()
{
	assert(playercarindex<car.size());
	float curbestlap = car[playercarindex].GetBestLap();
	float prevbest(0);
	bool haveprevbest = trackrecords.GetParam(car[playercarindex].GetCarType(), "sector 0", prevbest);
	if (haveprevbest && (curbestlap == 0 || prevbest < curbestlap))
		return prevbest;
	else
		return curbestlap;
}

int TIMER::GetPlayerCurrentLap()
{
	return GetCurrentLap(playercarindex);
}

int TIMER::GetCurrentLap(unsigned int index)
{
	assert(index<car.size());
	return car[index].GetCurrentLap();
}

float TIMER::GetStagingTimeLeft() const
{
	return pretime;
}

std::pair <int, int> TIMER::GetCarPlace(int index)
{
	assert(index<(int)car.size() || index >= 0);

	int place = 1;
	int total = car.size();

	std::list <PLACE> distances;

	for (int i = 0; i < (int)car.size(); i++)
		distances.push_back(PLACE(i, car[i].GetCurrentLap(), car[i].GetLapDistance()));

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

std::pair <int, int> TIMER::GetPlayerPlace()
{
	return GetCarPlace(playercarindex);
}

float TIMER::GetDriftScore(unsigned int index) const
{
	assert(index<car.size());
	return car[index].GetDriftScore().GetScore();
}

float TIMER::GetThisDriftScore(unsigned int index) const
{
	assert(index<car.size());
	return car[index].GetDriftScore().GetThisDriftScore() + car[index].GetDriftScore().GetBonusScore();
}

bool TIMER::GetIsDrifting(unsigned int index) const
{
	assert(index<car.size());
	return car[index].GetDriftScore().GetDrifting();
}

void TIMER::SetIsDrifting(unsigned int index, bool newdrift, bool countthedrift)
{
	assert(index<car.size());
	car[index].GetDriftScore().SetDrifting(newdrift, countthedrift);
}

void TIMER::IncrementThisDriftScore(unsigned int index, float incrementamount)
{
	assert(index<car.size());
	car[index].GetDriftScore().SetThisDriftScore(car[index].GetDriftScore().GetThisDriftScore()+incrementamount);
}

void TIMER::UpdateMaxDriftAngleSpeed(unsigned int index, float angle, float speed)
{
	assert(index<car.size());
	if (angle > car[index].GetDriftScore().GetMaxAngle())
		car[index].GetDriftScore().SetMaxAngle(angle);
	if (speed > car[index].GetDriftScore().GetMaxSpeed())
		car[index].GetDriftScore().SetMaxSpeed(speed);
}

TIMER::PLACE::PLACE(int newindex, int newlaps, double newdistance) : index(newindex), laps(newlaps), distance(newdistance)
{
	// Constructor.
}

int TIMER::PLACE::GetIndex() const
{
	return index;
}

bool TIMER::PLACE::operator< (const TIMER::PLACE & other) const
{
	if (laps == other.laps)
		return distance > other.distance;
	else
		return laps > other.laps;
}

TIMER::LAPTIME::LAPTIME()
{
	Reset();
}

void TIMER::LAPTIME::Reset()
{
	havetime = false;
	time = 0;
}

bool TIMER::LAPTIME::HaveTime() const
{
	return havetime;
}

double TIMER::LAPTIME::GetTimeInSeconds() const
{
	return time;
}

void TIMER::LAPTIME::GetTimeInMinutesSeconds(float & secs, int & min) const
{
	min = (int) time / 60;
	secs = time - min*60;
}
void TIMER::LAPTIME::Set(double newtime)
{
	time = newtime;
	havetime = true;
}

void TIMER::LAPTIME::SetIfFaster(double newtime)
{
	if (!havetime || newtime < time)
		time = newtime;
	havetime = true;
}

TIMER::DRIFTSCORE::DRIFTSCORE() : score(0), thisdriftscore(0), drifting(false), max_angle(0), max_speed(0)
{
	// Constructor.
}

void TIMER::DRIFTSCORE::Reset()
{
	score = 0;
	SetDrifting(false, false);
}

void TIMER::DRIFTSCORE::SetScore ( float value )
{
	score = value;
}

float TIMER::DRIFTSCORE::GetScore() const
{
	return score;
}

void TIMER::DRIFTSCORE::SetDrifting ( bool value, bool countit )
{
	if (!value && drifting && countit && thisdriftscore + GetBonusScore() > 5.0)
		score += thisdriftscore + GetBonusScore();

	if (!value)
	{
		thisdriftscore = 0;
		max_angle = 0;
		max_speed = 0;
	}

	drifting = value;
}

bool TIMER::DRIFTSCORE::GetDrifting() const
{
	return drifting;
}

void TIMER::DRIFTSCORE::SetThisDriftScore ( float value )
{
	thisdriftscore = value;
}

float TIMER::DRIFTSCORE::GetThisDriftScore() const
{
	return thisdriftscore;
}

void TIMER::DRIFTSCORE::SetMaxAngle ( float value )
{
	max_angle = value;
}

void TIMER::DRIFTSCORE::SetMaxSpeed ( float value )
{
	max_speed = value;
}

float TIMER::DRIFTSCORE::GetMaxAngle() const
{
	return max_angle;
}

float TIMER::DRIFTSCORE::GetMaxSpeed() const
{
	return max_speed;
}

float TIMER::DRIFTSCORE::GetBonusScore() const
{
	// Including thisdriftscore here is redundant on purpose to give more points to long drifts.
	return max_speed / 2.0 + max_angle * 40.0 / 3.141593 + thisdriftscore;
}

TIMER::LAPINFO::LAPINFO(const std::string & newcartype) : cartype(newcartype)
{
	Reset();
}

void TIMER::LAPINFO::Reset()
{
	time = totaltime = 0.0;
	lastlap.Reset();
	bestlap.Reset();
	num_laps = 0;
}

void TIMER::LAPINFO::Tick(float dt)
{
	time += dt;
}

void TIMER::LAPINFO::Lap(bool countit)
{
	if (countit)
	{
		lastlap.Set(time);
		bestlap.SetIfFaster(time);
	}

	totaltime += time;
	time = 0.0;
	num_laps++;
}

const std::string & TIMER::LAPINFO::GetCarType() const
{
	return cartype;
}

void TIMER::LAPINFO::DebugPrint(std::ostream & out) const
{
	out << "car=" << cartype << ", t=" << totaltime << ", tlap=" << time << ", last=" << lastlap.GetTimeInSeconds() << ", best=" << bestlap.GetTimeInSeconds() << ", lap=" << num_laps << std::endl;
}

double TIMER::LAPINFO::GetTime() const
{
	return time;
}

double TIMER::LAPINFO::GetLastLap() const
{
	return lastlap.GetTimeInSeconds();
}

double TIMER::LAPINFO::GetBestLap() const
{
	return bestlap.GetTimeInSeconds();
}

int TIMER::LAPINFO::GetCurrentLap() const
{
	return num_laps;
}

void TIMER::LAPINFO::UpdateLapDistance(double newdistance)
{
	lapdistance = newdistance;
}

double TIMER::LAPINFO::GetLapDistance() const
{
	return lapdistance;
}

const TIMER::DRIFTSCORE & TIMER::LAPINFO::GetDriftScore() const
{
	return driftscore;
}

TIMER::DRIFTSCORE & TIMER::LAPINFO::GetDriftScore()
{
	return driftscore;
}
