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

#ifndef _REPLAY_H
#define _REPLAY_H

#include "mathvector.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>
#include <string>

class CAR;

class REPLAY
{
public:
	REPLAY(float framerate);

	/// return number of recorded car states
	unsigned GetCarsRecorded() const;

	/// true on success
	bool StartPlaying(
		const std::string & replayfilename,
		std::ostream & error_output);

	/// stops playing/recording, clears state
	void Reset();

	/// true if the replay system is currently playing
	bool GetPlaying() const;

	void StartRecording(
		const std::vector<std::string> & cartype,
		const std::vector<std::string> & carpaint,
		const std::vector<std::string> & carconfig,
		const std::vector<MATHVECTOR<float, 3> > & carcolor,
		const std::string & trackname,
		std::ostream & error_log);

	/// if replayfilename is empty, do not save the data
	void StopRecording(const std::string & replayfilename);

	/// true if the replay system is currently recording
	bool GetRecording() const;

	/// set car state, return car inputs
	const std::vector<float> & PlayFrame(unsigned carid, CAR & car);

	/// record car inputs and state
	void RecordFrame(unsigned carid, const std::vector <float> & inputs, CAR & car);

	bool Serialize(joeserialize::Serializer & s);

	const std::vector<std::string> & GetCarTypes() const;

	const std::vector<std::string> & GetCarFiles() const;

	const std::vector<std::string> & GetCarPaints() const;

	const std::vector<MATHVECTOR<float, 3> > & GetCarColors() const;

	const std::string & GetTrack() const;

private:
	friend class joeserialize::Serializer;

	class VERSION
	{
	public:
		std::string format_version;
		int inputs_supported;
		float framerate;

		VERSION();

		VERSION(const std::string & ver, unsigned ins, float newfr);

		bool Serialize(joeserialize::Serializer & s);

		void Save(std::ostream & outstream);

		void Load(std::istream & instream);

		bool operator==(const VERSION & other) const;
	};

	/// input delta frame (p-frame)
	class INPUTFRAME
	{
	public:
		INPUTFRAME();

		INPUTFRAME(unsigned newframe);

		bool Serialize(joeserialize::Serializer & s);

		void AddInput(int index, float value);

		unsigned GetNumInputs() const;

		unsigned GetFrame() const;

		const std::pair<int, float>& GetInput(unsigned index) const;

	private:
		friend class joeserialize::Serializer;
		unsigned frame;
		std::vector< std::pair<int, float> > inputs;
	};

	/// input and vehicle state frame (i-frame)
	class STATEFRAME
	{
	public:
		STATEFRAME();

		STATEFRAME(unsigned newframe);

		bool Serialize(joeserialize::Serializer & s);

		void SetBinaryStateData(const std::string & value);

		unsigned GetFrame() const;

		const std::string & GetBinaryStateData() const;

		const std::vector<float> & GetInputSnapshot() const;

		void SetInputSnapshot(const std::vector<float>& value);

	private:
		friend class joeserialize::Serializer;
		unsigned frame;
		std::string binary_state_data;
		std::vector<float> input_snapshot;
	};

	struct CARSTATE
	{
		/// serialized
		std::vector<INPUTFRAME> inputframes;
		std::vector<STATEFRAME> stateframes;

		/// not serialized
		std::vector<float> inputbuffer; // buffer for input delta frame decoding
		unsigned cur_inputframe;
		unsigned cur_stateframe;
		unsigned frame;

		/// true if we have zero recorded frames
		bool Empty() const;

		/// reset state
		void Reset();

		/// write state into outstream
		bool Serialize(joeserialize::Serializer & s);

		/// set car, update inputbuffer, false if we are out of frames
		bool PlayFrame(CAR & car);

		/// get car state, save input delta frame
		void RecordFrame(const std::vector<float> & inputs, CAR & car);

		void ProcessPlayInputFrame(const INPUTFRAME & frame);

		void ProcessPlayStateFrame(const STATEFRAME & frame, CAR & car);
	};

	/// serialized
	VERSION version_info;
	std::string track;
	std::vector<std::string> cartype;	//car type, used for loading graphics and sound
	std::vector<std::string> carpaint;	//car paint id string
	std::vector<std::string> carconfig;	//entire contents of the car file (e.g. XS.car)
	std::vector<MATHVECTOR<float, 3> > carcolor;
	std::vector<CARSTATE> carstate;

	/// not serialized
	enum {IDLE, RECORDING, PLAYING} replaymode;

	/// load all input and state frames to the stream
	bool Load(std::istream & instream, std::ostream & error_output);

	/// save all input and state frames to the stream and then clear them
	void Save(std::ostream & outstream);
};

// implementation

inline bool REPLAY::GetPlaying() const
{
	return (replaymode == PLAYING);
}

inline bool REPLAY::GetRecording() const
{
	return (replaymode == RECORDING);
}

inline const std::vector<std::string> & REPLAY::GetCarTypes() const
{
	return cartype;
}

inline const std::vector<std::string> & REPLAY::GetCarFiles() const
{
	return carconfig;
}

inline const std::vector<std::string> & REPLAY::GetCarPaints() const
{
	return carpaint;
}

inline const std::vector<MATHVECTOR<float, 3> > & REPLAY::GetCarColors() const
{
	return carcolor;
}

inline const std::string & REPLAY::GetTrack() const
{
	return track;
}

#endif
