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

#include "car.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>
#include <string>

class REPLAY
{
public:
	REPLAY(float framerate);

	///< returns true on success
	bool StartPlaying(
		const std::string & replayfilename,
		std::ostream & error_output);

	void StopPlaying();

	///< returns true if the replay system is currently playing
	bool GetPlaying() const { return (replaymode == PLAYING); }

	const std::vector<float> & PlayFrame(CAR & car);

	void StartRecording(
		const std::string & newcartype,
		const std::string & newcarpaint,
		const MATHVECTOR<float, 3> & newcarcolor,
		const PTree & carconfig,
		const std::string & trackname,
		std::ostream & error_log);

	///< if replayfilename is empty, do not save the data
	void StopRecording(const std::string & replayfilename);

	///< returns true if the replay system is currently recording
	bool GetRecording() const { return (replaymode == RECORDING); }

	void RecordFrame(const std::vector <float> & inputs, CAR & car);

	bool Serialize(joeserialize::Serializer & s);

	std::string GetCarType() const
	{
		return cartype;
	}

	std::string GetCarFile() const
	{
		return carfile;
	}

	std::string GetTrack() const
	{
		return track;
	}

	std::string GetCarPaint() const
	{
		return carpaint;
	}

	MATHVECTOR<float, 3> GetCarColorHSV() const
	{
		return carcolor;
	}

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

	class INPUTFRAME
	{
	public:
		INPUTFRAME() : frame(0) {}

		INPUTFRAME(int newframe) : frame(newframe) {}

		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s, frame);
			_SERIALIZE_(s, inputs);
			return true;
		}

		void AddInput(int index, float value)
		{
			inputs.push_back(std::make_pair(index, value));
		}

		unsigned GetNumInputs() const
		{
			return inputs.size();
		}

		int GetFrame() const
		{
			return frame;
		}

		///returns a pair for the <control id, value> of the indexed input
		const std::pair<int, float>& GetInput(unsigned index) const
		{
			assert(index < inputs.size());
			return inputs[index];
		}

	private:
		friend class joeserialize::Serializer;
		int frame;
		std::vector<std::pair<int, float> > inputs;
	};

	class STATEFRAME
	{
	public:
		STATEFRAME() : frame(0) {}

		STATEFRAME(int newframe) : frame(newframe) {}

		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s, frame);
			_SERIALIZE_(s, binary_state_data);
			_SERIALIZE_(s, input_snapshot);
			return true;
		}

		void SetBinaryStateData(const std::string & value)
		{
			binary_state_data = value;
		}

		int GetFrame() const
		{
			return frame;
		}

		std::string GetBinaryStateData() const
		{
			return binary_state_data;
		}

		const std::vector<float> & GetInputSnapshot() const
		{
			return input_snapshot;
		}

		void SetInputSnapshot(const std::vector<float>& value)
		{
			input_snapshot = value;
		}

	private:
		friend class joeserialize::Serializer;
		int frame;
		std::string binary_state_data;
		std::vector<float> input_snapshot;
	};

	// serialized data
	VERSION version_info;
	std::string track;
	std::string cartype; //car type, used for loading graphics and sound
	std::string carpaint; //car paint id string
	std::string carfile; //entire contents of the car file (e.g. XS.car)
	MATHVECTOR<float, 3> carcolor;
	std::vector<INPUTFRAME> inputframes;
	std::vector<STATEFRAME> stateframes;

	// not stored in the replay file
	int frame;
	enum {
		IDLE,
		RECORDING,
		PLAYING
	} replaymode;
	std::vector<float> inputbuffer;
	unsigned cur_inputframe;
	unsigned cur_stateframe;

	// functions
	void ProcessPlayInputFrame(const INPUTFRAME & frame);
	void ProcessPlayStateFrame(const STATEFRAME & frame, CAR & car);
	bool Load(std::istream & instream); ///< load one input and state frame chunk from the stream. returns true on success, returns false for EOF
	bool LoadHeader(std::istream & instream, std::ostream & error_output); ///< returns true on success.
	void Save(std::ostream & outstream); ///< save all input and state frames to the stream and then clear them
	void SaveHeader(std::ostream & outstream); ///< write only the header information to the stream
	void GetReadyToPlay();
	void GetReadyToRecord();
};

#endif
