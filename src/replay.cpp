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

#include "replay.h"
#include "unittest.h"
#include "cfg/ptree.h"
#include "carinput.h"
#include "car.h"

#include <sstream>
#include <fstream>

REPLAY::REPLAY(float framerate) :
	version_info("VDRIFTREPLAYV16", CARINPUT::GAME_ONLY_INPUTS_START_HERE, framerate),
	replaymode(IDLE)
{
	// ctor
}

bool REPLAY::StartPlaying(const std::string & replayfilename, std::ostream & error_output)
{
	Reset();

	std::ifstream replaystream(replayfilename.c_str(), std::ios::binary);
	if (!replaystream)
	{
		error_output << "Error loading replay file: " << replayfilename << std::endl;
		return false;
	}

	if (!Load(replaystream, error_output))
		return false;

	for (size_t i = 0; i < carstate.size(); ++i)
	{
		carstate[i].Reset();
	}

	replaymode = PLAYING;

	return true;
}

void REPLAY::Reset()
{
	replaymode = IDLE;
	track.clear();
	carinfo.clear();
	carstate.clear();
}

void REPLAY::StartRecording(
	const std::vector<CARINFO> & ncarinfo,
	const std::string & trackname,
	std::ostream & error_log)
{
	Reset();

	replaymode = RECORDING;
	carinfo = ncarinfo;
	track = trackname;

	carstate.resize(carinfo.size());
	for (size_t i = 0; i < carstate.size(); ++i)
	{
		carstate[i].Reset();
	}
}

void REPLAY::StopRecording(const std::string & replayfilename)
{
	replaymode = IDLE;
	if (!replayfilename.empty())
	{
		std::ofstream f(replayfilename.c_str(), std::ios::binary);
		if (f)
		{
			Save(f);
		}
	}
}

const std::vector<float> & REPLAY::PlayFrame(unsigned carid, CAR & car)
{
	assert(carid < carstate.size());
	assert(unsigned(version_info.inputs_supported) == CARINPUT::GAME_ONLY_INPUTS_START_HERE);

	if (GetPlaying() && !carstate[carid].PlayFrame(car))
	{
		replaymode = IDLE;
	}
	return carstate[carid].inputbuffer;
}

void REPLAY::RecordFrame(unsigned carid, const std::vector <float> & inputs, CAR & car)
{
	assert(carid < carstate.size());
	assert(unsigned(version_info.inputs_supported)== CARINPUT::GAME_ONLY_INPUTS_START_HERE);

	if (GetRecording())
	{
		// enforce a maximum recording time of about 92 days
		if (carstate[carid].frame > 2000000000)
			replaymode = IDLE;

		carstate[carid].RecordFrame(inputs, car);
	}
}

void REPLAY::CARSTATE::RecordFrame(const std::vector <float> & inputs, CAR & car)
{
	assert(inputbuffer.size() == CARINPUT::GAME_ONLY_INPUTS_START_HERE);

	// record inputs, delta encoding
	INPUTFRAME newinputframe(frame);
	for (unsigned i = 0; i < CARINPUT::GAME_ONLY_INPUTS_START_HERE; i++)
	{
		if (inputs[i] != inputbuffer[i])
		{
			inputbuffer[i] = inputs[i];
			newinputframe.AddInput(i, inputs[i]);
		}
	}
	if (newinputframe.GetNumInputs() > 0)
		inputframes.push_back(newinputframe);

	// record every 30th state, input frame
	if (frame % 30 == 0)
	{
		std::stringstream statestream;
		joeserialize::BinaryOutputSerializer serialize_output(statestream);
		car.Serialize(serialize_output);
		stateframes.push_back(STATEFRAME(frame));
		stateframes.back().SetBinaryStateData(statestream.str());
		stateframes.back().SetInputSnapshot(inputs);
	}

	frame++;
}

bool REPLAY::CARSTATE::PlayFrame(CAR & car)
{
	frame++;

	assert(inputbuffer.size() == CARINPUT::GAME_ONLY_INPUTS_START_HERE);

	// fast forward through the inputframes until we're up to date
	while (cur_inputframe < inputframes.size() &&
		inputframes[cur_inputframe].GetFrame() <= frame)
	{
		ProcessPlayInputFrame(inputframes[cur_inputframe]);
		cur_inputframe++;
	}

	// fast forward through the stateframes until we're up to date
	while (cur_stateframe < stateframes.size() &&
			stateframes[cur_stateframe].GetFrame() <= frame)
	{
		if (stateframes[cur_stateframe].GetFrame() == frame)
			ProcessPlayStateFrame(stateframes[cur_stateframe], car);
		cur_stateframe++;
	}

	return (cur_stateframe != stateframes.size() || cur_inputframe != inputframes.size());
}

void REPLAY::CARSTATE::ProcessPlayInputFrame(const INPUTFRAME & frame)
{
	for (unsigned i = 0; i < frame.GetNumInputs(); i++)
	{
		std::pair <int, float> input_pair = frame.GetInput(i);
		inputbuffer[input_pair.first] = input_pair.second;
	}
}

void REPLAY::CARSTATE::ProcessPlayStateFrame(const STATEFRAME & frame, CAR & car)
{
	// process input snapshot
	for (unsigned i = 0; i < inputbuffer.size() && i < frame.GetInputSnapshot().size(); i++)
	{
		inputbuffer[i] = frame.GetInputSnapshot()[i];
	}

	// process binary car state
	std::stringstream statestream(frame.GetBinaryStateData());
	joeserialize::BinaryInputSerializer serialize_input(statestream);
	car.Serialize(serialize_input);
}

bool REPLAY::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, track);
	_SERIALIZE_(s, carinfo);
	_SERIALIZE_(s, carstate);
	return true;
}

void REPLAY::Save(std::ostream & outstream)
{
	// write the file format version data manually
	// if the serialization functions were used,
	// a variable length string would be written instead,
	// which isn't exactly what we want
	version_info.Save(outstream);

	joeserialize::BinaryOutputSerializer serialize_output(outstream);
	Serialize(serialize_output);

	Reset();
}

bool REPLAY::Load(std::istream & instream, std::ostream & error_output)
{
	VERSION stream_version;
	stream_version.Load(instream);

	if (!(stream_version == version_info))
	{
		error_output << "Stream version " <<
			stream_version.format_version << "/" <<
			stream_version.inputs_supported << "/" <<
			stream_version.framerate <<
			" does not match expected version " <<
			version_info.format_version << "/" <<
			version_info.inputs_supported << "/" <<
			version_info.framerate << std::endl;
		return false;
	}

	joeserialize::BinaryInputSerializer serialize_input(instream);
	if (!Serialize(serialize_input))
	{
		error_output << "Error loading replay." << std::endl;
		return false;
	}

	return true;
}

REPLAY::VERSION::VERSION() :
	format_version("VDRIFTREPLAYV??"),
	inputs_supported(0),
	framerate(0)
{
	// ctor
}

REPLAY::VERSION::VERSION(const std::string & ver,  unsigned ins, float newfr) :
	format_version(ver),
	inputs_supported(ins),
	framerate(newfr)
{
	// ctor
}

bool REPLAY::VERSION::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, inputs_supported);
	_SERIALIZE_(s, framerate);
	return true;
}

void REPLAY::VERSION::Save(std::ostream & outstream)
{
	// write the file format version data manually
	// if the serialization functions were used,
	// a variable length string would be written instead,
	// which isn't exactly what we want
	outstream.write(format_version.data(), format_version.length());

	// write the rest of the versioning info
	joeserialize::BinaryOutputSerializer serialize_output(outstream);
	Serialize(serialize_output);
}

void REPLAY::VERSION::Load(std::istream & instream)
{
	// read the file format version data manually
	const unsigned bufsize = format_version.length();
	char * version_buf = new char[bufsize+1];
	instream.read(version_buf, bufsize);
	version_buf[bufsize] = '\0';
	format_version = version_buf;
	delete [] version_buf;

	// read the rest of the versioning info
	joeserialize::BinaryInputSerializer serialize_input(instream);
	Serialize(serialize_input);
}

bool REPLAY::VERSION::operator==(const VERSION & other) const
{
	return (format_version == other.format_version &&
		inputs_supported == other.inputs_supported &&
			framerate == other.framerate);
}

REPLAY::INPUTFRAME::INPUTFRAME() :
	frame(0)
{
	// ctor
}

REPLAY::INPUTFRAME::INPUTFRAME(unsigned newframe) :
	frame(newframe)
{
	// ctor
}

bool REPLAY::INPUTFRAME::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, frame);
	_SERIALIZE_(s, inputs);
	return true;
}

void REPLAY::INPUTFRAME::AddInput(int index, float value)
{
	inputs.push_back(std::make_pair(index, value));
}

unsigned REPLAY::INPUTFRAME::GetNumInputs() const
{
	return inputs.size();
}

unsigned REPLAY::INPUTFRAME::GetFrame() const
{
	return frame;
}

const std::pair<int, float> & REPLAY::INPUTFRAME::GetInput(unsigned index) const
{
	assert(index < inputs.size());
	return inputs[index];
}

REPLAY::STATEFRAME::STATEFRAME() :
	frame(0)
{
	// ctor
}

REPLAY::STATEFRAME::STATEFRAME(unsigned newframe) :
	frame(newframe)
{
	// ctor
}

bool REPLAY::STATEFRAME::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, frame);
	_SERIALIZE_(s, binary_state_data);
	_SERIALIZE_(s, input_snapshot);
	return true;
}

void REPLAY::STATEFRAME::SetBinaryStateData(const std::string & value)
{
	binary_state_data = value;
}

unsigned REPLAY::STATEFRAME::GetFrame() const
{
	return frame;
}

const std::string & REPLAY::STATEFRAME::GetBinaryStateData() const
{
	return binary_state_data;
}

const std::vector<float> & REPLAY::STATEFRAME::GetInputSnapshot() const
{
	return input_snapshot;
}

void REPLAY::STATEFRAME::SetInputSnapshot(const std::vector<float>& value)
{
	input_snapshot = value;
}

bool REPLAY::CARSTATE::Empty() const
{
	return stateframes.empty() && inputframes.empty();
}

void REPLAY::CARSTATE::Reset()
{
	inputbuffer.clear();
	inputbuffer.resize(CARINPUT::GAME_ONLY_INPUTS_START_HERE, 0);
	cur_inputframe = 0;
	cur_stateframe = 0;
	frame = 0;
}

bool REPLAY::CARSTATE::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, inputframes);
	_SERIALIZE_(s, stateframes);
	return true;
}

QT_TEST(replay_test)
{
	/*//basic version validity check
	{
		REPLAY replay(0.004);
		stringstream teststream;
		replay.Save(teststream);
		QT_CHECK(replay.Load(teststream, std::cerr));
	}*/
}
