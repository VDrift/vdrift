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

	void PlayFrame(std::list<CAR> & cars);
	const std::vector<float> & PlayFrameCar(CAR & car);

	void StartRecording(
		const std::string & trackname,
		std::ostream & error_log,
		const std::list <CAR> & cars,
 		int num_laps);

	///< if replayfilename is empty, do not save the data
	void StopRecording(const std::string & replayfilename);

	///< returns true if the replay system is currently recording
	bool GetRecording() const { return (replaymode == RECORDING); }

	void RecordFrame(const std::list<CAR> & cars);
	void RecordFrameCar(const std::vector <float> & inputs, CAR & car);

	bool Serialize(joeserialize::Serializer & s);

	std::string GetCarType(CARID car_id) const
	{
		return cartypes.at(car_id);
	}

	std::string GetCarFile(CARID car_id) const
	{
		return carfiles.at(car_id);
	}

	std::string GetTrack() const
	{
		return track;
	}

	std::string GetCarPaint(CARID car_id) const
	{
		return carpaints.at(car_id);
	}

	MATHVECTOR<float, 3> GetCarColorHSV(CARID car_id) const
	{
		return carcolors.at(car_id);
	}

	int GetNumberLaps() const {
		return number_laps;
	}

	unsigned int GetNumberCars() const
	{
		return cartypes.size();
	}

	const std::vector<CARID>& GetCarIds() const {
		return carids;
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

		void AddInput(CARID car_id, int index, float value)
		{
			inputs[car_id].push_back(std::make_pair(index, value));
		}

		unsigned GetNumInputs(CARID car_id) const
		{
			if (inputs.find(car_id) != inputs.end())
				return inputs.at(car_id).size();
			else
				return 0;
		}

		int GetFrame() const
		{
			return frame;
		}

		///returns a pair for the <control id, value> of the indexed input
		const std::pair<int, float>& GetInput(CARID car_id, unsigned index) const
		{
			assert(index < inputs.at(car_id).size());
			return inputs.at(car_id)[index];
		}

	private:
		friend class joeserialize::Serializer;
		int frame;
		std::map<CARID, std::vector<std::pair<int, float> > > inputs;
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

		void SetBinaryStateData(CARID car_id, const std::string & value)
		{
			binary_state_data[car_id] = value;
		}

		int GetFrame() const
		{
			return frame;
		}

		std::string GetBinaryStateData(CARID car_id) const
		{
			return binary_state_data.at(car_id);
		}

		const std::vector<float> & GetInputSnapshot(CARID car_id) const
		{
			return input_snapshot.at(car_id);
		}

		void SetInputSnapshot(CARID car_id, const std::vector<float>& value)
		{
			input_snapshot[car_id] = value;
		}

	private:
		friend class joeserialize::Serializer;
		int frame;
		std::map<CARID, std::string> binary_state_data;
		std::map<CARID, std::vector<float> > input_snapshot;
	};

	// serialized data
	VERSION version_info;
	std::string track;

	std::vector<CARID>			 carids; // a list of the cars IDs
	std::map<CARID, std::string> cartypes; //car type, used for loading graphics and sound
	std::map<CARID, std::string> carpaints; //car paint id string
	std::map<CARID, std::string> carfiles; //entire contents of the car file (e.g. XS.car)
	std::map<CARID, MATHVECTOR <float, 3> > carcolors;

	int number_laps;

	std::vector<INPUTFRAME> inputframes;
	std::vector<STATEFRAME> stateframes;

	// not stored in the replay file
	int frame;
	enum {
		IDLE,
		RECORDING,
		PLAYING
	} replaymode;
	
	std::map<CARID, std::vector<float> > inputbuffers;

	unsigned cur_inputframe;
	unsigned cur_stateframe;

	bool should_record_state;

	// functions
	void ProcessPlayInputFrame(const INPUTFRAME & frame, std::list<CAR> & cars);
	void ProcessPlayStateFrame(const STATEFRAME & frame, std::list<CAR> & cars);
	bool Load(std::istream & instream); ///< load one input and state frame chunk from the stream. returns true on success, returns false for EOF
	bool LoadHeader(std::istream & instream, std::ostream & error_output); ///< returns true on success.
	void Save(std::ostream & outstream); ///< save all input and state frames to the stream and then clear them
	void SaveHeader(std::ostream & outstream); ///< write only the header information to the stream
	void GetReadyToPlay();
	void GetReadyToRecord();
};

#endif
