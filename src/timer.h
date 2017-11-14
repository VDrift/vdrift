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

#ifndef _TIMER_H
#define _TIMER_H

#include "cfg/config.h"

#include <ostream>
#include <string>
#include <vector>

class Timer
{
public:
	Timer() : pretime(0.0), playercarindex(0), loaded(false) {}

	~Timer() {Unload();}

	bool Load(const std::string & trackrecordspath, float stagingtime);

	///add a car of the given type and return the integer identifier that the track system will use
	int AddCar(const std::string & cartype);

	void SetPlayerCarId(int newid) {playercarindex = newid;}

	void Unload();

	bool Staging() const {return (pretime > 0);}

	void Tick(float dt);

	void Lap(const unsigned int carid, const int nextsector);

	void UpdateDistance(const unsigned int carid, const double newdistance);

	float GetTime(unsigned int index) {assert(index<car.size());return car[index].GetTime();}

	float GetLastLap(unsigned int index) {assert(index<car.size());return car[index].GetLastLap();}

	float GetBestLap(unsigned int index) {assert(index<car.size());return car[index].GetBestLap();}

	int GetCurrentLap(unsigned int index) const {assert(index<car.size()); return car[index].GetCurrentLap();}

	int GetLastSector(unsigned int index) const {assert(index<car.size()); return car[index].GetSector();}

	float GetStagingTimeLeft() const {return pretime;}

	///return the place (first element) out of total (second element)
	std::pair <int, int> GetCarPlace(int index);

	float GetDriftScore(unsigned int index) const
	{
		assert(index<car.size());
		return car[index].GetDriftScore().GetScore();
	}

	float GetThisDriftScore(unsigned int index) const
	{
		assert(index<car.size());
		return car[index].GetDriftScore().GetThisDriftScore() + car[index].GetDriftScore().GetBonusScore();
	}

	bool GetIsDrifting(unsigned int index) const
	{
		assert(index<car.size());
		return car[index].GetDriftScore().GetDrifting();
	}

	void SetIsDrifting(unsigned int index, bool newdrift, bool countthedrift)
	{
		assert(index<car.size());
		car[index].GetDriftScore().SetDrifting(newdrift, countthedrift);
	}

	void IncrementThisDriftScore(unsigned int index, float incrementamount)
	{
		assert(index<car.size());
		car[index].GetDriftScore().SetThisDriftScore(car[index].GetDriftScore().GetThisDriftScore()+incrementamount);
	}

	void UpdateMaxDriftAngleSpeed(unsigned int index, float angle, float speed)
	{
		assert(index<car.size());
		if (angle > car[index].GetDriftScore().GetMaxAngle())
			car[index].GetDriftScore().SetMaxAngle(angle);
		if (speed > car[index].GetDriftScore().GetMaxSpeed())
			car[index].GetDriftScore().SetMaxSpeed(speed);
	}

	template <class Stream>
	void DebugPrint(Stream & out) const
	{
		for (unsigned int i = 0; i < car.size(); ++i)
		{
			out << i << ". ";
			car[i].DebugPrint(out);
		}
	}

private:
	class LapInfo;
	std::vector <LapInfo> car;

	Config trackrecords; //the track records configfile
	std::string trackrecordsfile; //the filename for the track records
	float pretime; //amount of time left in staging
	unsigned int playercarindex; //the index for the player's car; defaults to zero
	bool loaded;

	class DriftScore
	{
	private:
		float score;
		float thisdriftscore;
		bool drifting;
		float max_angle;
		float max_speed;

	public:
		DriftScore() : score(0), thisdriftscore(0), drifting(false), max_angle(0), max_speed(0) {}

		void Reset()
		{
			score = 0;
			SetDrifting(false, false);
		}

		void SetScore ( float value )
		{
			score = value;
		}

		float GetScore() const
		{
			return score;
		}

		void SetDrifting ( bool value, bool countit )
		{
			if (!value && drifting && countit && thisdriftscore + GetBonusScore() > 5)
			{
				score += thisdriftscore + GetBonusScore();
				//std::cout << "Incrementing score: " << score << std::endl;
			}
			//else if (!value && drifting) std::cout << "Not scoring: " << countit << ", " << thisdriftscore << std::endl;

			if (!value)
			{
				thisdriftscore = 0;
				max_angle = 0;
				max_speed = 0;
			}

			drifting = value;
		}

		bool GetDrifting() const
		{
			return drifting;
		}

		void SetThisDriftScore ( float value )
		{
			thisdriftscore = value;
		}

		float GetThisDriftScore() const
		{
			return thisdriftscore;
		}

		void SetMaxAngle ( float value )
		{
			max_angle = value;
		}

		void SetMaxSpeed ( float value )
		{
			max_speed = value;
		}

		float GetMaxAngle() const
		{
			return max_angle;
		}

		float GetMaxSpeed() const
		{
			return max_speed;
		}

		float GetBonusScore() const
		{
			return max_speed * 0.5f + max_angle * 40 / 3.141593f + thisdriftscore; //including thisdriftscore here is redundant on purpose to give more points to long drifts
		}
	};

	class LapInfo
	{
	private:
		std::string cartype;
		double bestlap; //best lap time for player & opponents
		double lastlap; //last lap time for player & opponents
		double time; //running time for this lap
		double totaltime; //total time of a race for player & opponents
		double lapdistance; //total track distance driven this lap in meters
		int num_laps; //current lap
		int sector;
		DriftScore driftscore;

		///only set the time if we don't have a time or if the new time is faster than the current time
		static inline void SetIfFaster(double & time, double newtime)
		{
			if (time == 0 || time > newtime)
				time = newtime;
		}

	public:
		LapInfo(const std::string & newcartype, double newbestlap=0) :
			cartype(newcartype),
			bestlap(newbestlap),
			lastlap(0),
			time(0),
			totaltime(0),
			lapdistance(0),
			num_laps(0),
			sector(-1)
		{
			// ctor
		}

		void Tick(float dt)
		{
			time += double(dt);
		}

		void Lap(bool countit)
		{
			if (countit)
			{
				lastlap = time;
				SetIfFaster(bestlap, time);
			}

			totaltime += time;
			time = 0;
			num_laps++;
		}

		const std::string & GetCarType() const
		{
			return cartype;
		}

		template <class Stream>
		void DebugPrint(Stream & out) const
		{
			out << "car=" << cartype << ", t=" << totaltime << ", tlap=" << time
				<< ", last=" << lastlap << ", best=" << bestlap
				<< ", lap=" << num_laps << "\n";
		}

		double GetTime() const
		{
			return time;
		}

		double GetLastLap() const
		{
			return lastlap;
		}

		double GetBestLap() const
		{
			return bestlap;
		}

		int GetCurrentLap() const
		{
			return num_laps;
		}

		void SetSector(int nextsector)
		{
			sector = nextsector;
		}

		int GetSector() const
		{
			return sector;
		}

		void UpdateLapDistance(double newdistance)
		{
			lapdistance = newdistance;
		}

		double GetLapDistance() const
		{
			return lapdistance;
		}

		const DriftScore & GetDriftScore() const
		{
			return driftscore;
		}

		DriftScore & GetDriftScore()
		{
			return driftscore;
		}
	};
};

#endif
