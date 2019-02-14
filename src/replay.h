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

#include "carinfo.h"
#include "macros.h"

#include <iosfwd>
#include <string>
#include <vector>

class CarDynamics;

class Replay
{
public:
	Replay(float framerate);

	/// true on success
	bool StartPlaying(
		const std::string & replayfilename,
		std::ostream & error_output);

	/// stops playing/recording, clears state
	void Reset();

	/// true if the replay system is currently playing
	bool GetPlaying() const;

	void StartRecording(
		const std::vector<CarInfo> & carinfo,
		const std::string & trackname,
		std::ostream & error_log);

	/// if replayfilename is empty, do not save the data
	void StopRecording(const std::string & replayfilename);

	/// true if the replay system is currently recording
	bool GetRecording() const;

	/// set car state, return car inputs
	const std::vector<float> & PlayFrame(unsigned carid, CarDynamics & car);

	/// record car inputs and state
	void RecordFrame(unsigned carid, const std::vector <float> & inputs, CarDynamics & car);

	template <class Serializer>
	bool Serialize(Serializer & s);

	const std::vector<CarInfo> & GetCarInfo() const;

	const std::string & GetTrack() const;

private:
	class Version
	{
	public:
		std::string format_version;
		int inputs_supported;
		float framerate;

		Version();

		Version(const std::string & ver, unsigned ins, float newfr);

		template <class Serializer>
		bool Serialize(Serializer & s);

		void Save(std::ostream & outstream);

		void Load(std::istream & instream);

		bool operator==(const Version & other) const;
	};

	/// input delta frame (p-frame)
	class InputFrame
	{
	public:
		InputFrame();

		InputFrame(unsigned newframe);

		template <class Serializer>
		bool Serialize(Serializer & s);

		void AddInput(int index, float value);

		unsigned GetNumInputs() const;

		unsigned GetFrame() const;

		const std::pair<int, float>& GetInput(unsigned index) const;

	private:
		unsigned frame;
		std::vector< std::pair<int, float> > inputs;
	};

	/// input and vehicle state frame (i-frame)
	class StateFrame
	{
	public:
		StateFrame();

		StateFrame(unsigned newframe);

		template <class Serializer>
		bool Serialize(Serializer & s);

		void SetBinaryStateData(const std::string & value);

		unsigned GetFrame() const;

		const std::string & GetBinaryStateData() const;

		const std::vector<float> & GetInputSnapshot() const;

		void SetInputSnapshot(const std::vector<float>& value);

	private:
		unsigned frame;
		std::string binary_state_data;
		std::vector<float> input_snapshot;
	};

	struct CarState
	{
		/// serialized
		std::vector<InputFrame> inputframes;
		std::vector<StateFrame> stateframes;

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
		template <class Serializer>
		bool Serialize(Serializer & s);

		/// set car, update inputbuffer, false if we are out of frames
		bool PlayFrame(CarDynamics & car);

		/// get car state, save input delta frame
		void RecordFrame(const std::vector<float> & inputs, CarDynamics & car);

		void ProcessPlayInputFrame(const InputFrame & frame);

		void ProcessPlayStateFrame(const StateFrame & frame, CarDynamics & car);
	};

	/// serialized
	Version version_info;
	std::string track;
	std::vector<CarInfo> carinfo;
	std::vector<CarState> carstate;

	/// not serialized
	enum {IDLE, RECORDING, PLAYING} replaymode;

	/// load all input and state frames to the stream
	bool Load(std::istream & instream, std::ostream & error_output);

	/// save all input and state frames to the stream and then clear them
	void Save(std::ostream & outstream);
};

// implementation

inline bool Replay::GetPlaying() const
{
	return (replaymode == PLAYING);
}

inline bool Replay::GetRecording() const
{
	return (replaymode == RECORDING);
}

inline const std::vector<CarInfo> & Replay::GetCarInfo() const
{
	return carinfo;
}

inline const std::string & Replay::GetTrack() const
{
	return track;
}

template <class Serializer>
inline bool Replay::CarState::Serialize(Serializer & s)
{
	_SERIALIZE_(s, inputframes);
	_SERIALIZE_(s, stateframes);
	return true;
}

template <class Serializer>
inline bool Replay::StateFrame::Serialize(Serializer & s)
{
	_SERIALIZE_(s, frame);
	_SERIALIZE_(s, binary_state_data);
	_SERIALIZE_(s, input_snapshot);
	return true;
}

template <class Serializer>
inline bool Replay::InputFrame::Serialize(Serializer & s)
{
	_SERIALIZE_(s, frame);
	_SERIALIZE_(s, inputs);
	return true;
}

template <class Serializer>
inline bool Replay::Version::Serialize(Serializer & s)
{
	_SERIALIZE_(s, inputs_supported);
	_SERIALIZE_(s, framerate);
	return true;
}

template <class Serializer>
inline bool Replay::Serialize(Serializer & s)
{
	_SERIALIZE_(s, track);
	_SERIALIZE_(s, carinfo);
	_SERIALIZE_(s, carstate);
	return true;
}

#endif
