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
/* This is the main entry point for VDrift.                             */
/*                                                                      */
/************************************************************************/

#ifndef _TIMER_H
#define _TIMER_H

#include "texture.h"
#include "vertexarray.h"
#include "config.h"
#include "font.h"
#include <ostream>
#include <string>
#include <vector>
#include <map>

class TIMER
{
public:
	TIMER();

	~TIMER();

	bool Load(const std::string & trackrecordspath, float stagingtime);

	/// Add a car of the given type and return the integer identifier that the track system will use.
	int AddCar(const std::string & cartype);

	void SetPlayerCarID(int newid);

	void Unload();

	bool Staging() const;

	void Tick(float dt);

	void Lap(const unsigned int carid, const int nextsector, const bool countit);

	void UpdateDistance(const unsigned int carid, const double newdistance);

	void DebugPrint(std::ostream & out) const;

	float GetPlayerTime();

	float GetLastLap();

	float GetBestLap();

	int GetPlayerCurrentLap();

	int GetCurrentLap(unsigned int index);

	float GetStagingTimeLeft() const;

	/// Eeturn the place (first element) out of total (second element).
	std::pair <int, int> GetCarPlace(int index);

	std::pair <int, int> GetPlayerPlace();

	float GetDriftScore(unsigned int index) const;

	float GetThisDriftScore(unsigned int index) const;

	bool GetIsDrifting(unsigned int index) const;

	void SetIsDrifting(unsigned int index, bool newdrift, bool countthedrift);

	void IncrementThisDriftScore(unsigned int index, float incrementamount);

	void UpdateMaxDriftAngleSpeed(unsigned int index, float angle, float speed);

private:
	class PLACE
	{
	public:
		PLACE(int newindex, int newlaps, double newdistance);

		int GetIndex() const;

		bool operator< (const PLACE & other) const;

	private:
		int index;
		int laps;
		double distance;
	};

	class LAPTIME
	{
	public:
		LAPTIME();

		void Reset();

		bool HaveTime() const;

		double GetTimeInSeconds() const;

		/// Convert time in seconds into output min and secs.
		void GetTimeInMinutesSeconds(float & secs, int & min) const;

		void Set(double newtime);

		/// Only set the time if we don't have a time or if the new time is faster than the current time.
		void SetIfFaster(double newtime);

	private:
		bool havetime;
		double time;
	};

	class DRIFTSCORE
	{
	public:
		DRIFTSCORE();

		void Reset();

		void SetScore(float value);

		float GetScore() const;

		void SetDrifting(bool value, bool countit);

		bool GetDrifting() const;

		void SetThisDriftScore(float value);

		float GetThisDriftScore() const;

		void SetMaxAngle(float value);

		void SetMaxSpeed(float value);

		float GetMaxAngle() const;

		float GetMaxSpeed() const;

		float GetBonusScore() const;

	private:
		float score;
		float thisdriftscore;
		bool drifting;
		float max_angle;
		float max_speed;
	};

	class LAPINFO
	{
	public:
		LAPINFO(const std::string & newcartype);

		void Reset();

		void Tick(float dt);

		void Lap(bool countit);

		const std::string & GetCarType() const;

		void DebugPrint(std::ostream & out) const;

		double GetTime() const;

		double GetLastLap() const;

		double GetBestLap() const;

		int GetCurrentLap() const;

		void UpdateLapDistance(double newdistance);

		double GetLapDistance() const;

		const DRIFTSCORE & GetDriftScore() const;

		DRIFTSCORE & GetDriftScore();

	private:
		/// Running time for this lap.
		double time;
		/// Last lap time for player & opponents.
		LAPTIME lastlap;
		/// Best lap time for player & opponents.
		LAPTIME bestlap;
		/// Total time of a race for player & opponents.
		double totaltime;
		/// Current lap.
		int num_laps;
		std::string cartype;
		/// Total track distance driven this lap in meters.
		double lapdistance;
		DRIFTSCORE driftscore;
	};

	std::vector <LAPINFO> car;
	/// The track records configfile.
	CONFIG trackrecords;
	/// The filename for the track records.
	std::string trackrecordsfile;
	/// Amount of time left in staging.
	float pretime;
	/// The index for the player's car. Defaults to zero.
	unsigned int playercarindex;
	bool loaded;
};

#endif
