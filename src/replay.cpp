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

#include <sstream>
#include <fstream>

REPLAY::REPLAY(float framerate) :
	version_info("VDRIFTREPLAYV14", CARINPUT::GAME_ONLY_INPUTS_START_HERE, framerate),
	frame(0),
	replaymode(IDLE),
	inputbuffer(CARINPUT::GAME_ONLY_INPUTS_START_HERE, 0)
{
	// ctor
}

void REPLAY::Save(std::ostream & outstream)
{
	joeserialize::BinaryOutputSerializer serialize_output(outstream);

	//the convention is that input frames come first, then state frames.
	joeserialize::Serializer & s = serialize_output;
	s.Serialize("inputframes", inputframes);
	s.Serialize("stateframes", stateframes);

	inputframes.clear();
	stateframes.clear();
}

void REPLAY::SaveHeader(std::ostream & outstream)
{
	//write the file format version data manually.  if the serialization functions were used, a variable length string would be written instead, which isn't exactly what we want
	version_info.Save(outstream);

	joeserialize::BinaryOutputSerializer serialize_output(outstream);
	Serialize(serialize_output);
}

bool REPLAY::Load(std::istream & instream)
{
	//peek to ensure we're not at the EOF
	instream.peek();
	if (instream.eof()) return false;

	std::vector <INPUTFRAME> newinputframes;
	std::vector <STATEFRAME> newstateframes;

	joeserialize::BinaryInputSerializer serialize_input(instream);

	//the convention is that input frames come first, then state frames.
	joeserialize::Serializer & s = serialize_input;
	s.Serialize("inputframes", newinputframes);
	s.Serialize("stateframes", newstateframes);

	//append the frames to the list
	inputframes.insert(inputframes.end(), newinputframes.begin(), newinputframes.end());
	stateframes.insert(stateframes.end(), newstateframes.begin(), newstateframes.end());

	return true;
}

bool REPLAY::LoadHeader(std::istream & instream, std::ostream & error_output)
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
		error_output << "Error loading replay header " << std::endl;
		return false;
	}

	return true;
}

void REPLAY::GetReadyToRecord()
{
	frame = 0;
	replaymode = RECORDING;
	inputframes.clear();
	stateframes.clear();
	inputbuffer.clear();
	inputbuffer.resize(CARINPUT::GAME_ONLY_INPUTS_START_HERE, 0);
}

void REPLAY::StartRecording(
	const std::string & newcartype,
	const std::string & newcarpaint,
	const MATHVECTOR<float, 3> & newcarcolor,
	const PTree & carconfig,
	const std::string & trackname,
	std::ostream & error_log)
{
	track = trackname;
	cartype = newcartype;
	carpaint = newcarpaint;
	carcolor = newcarcolor;

	GetReadyToRecord();

	std::stringstream carstream;
	write_ini(carconfig, carstream);
	carfile = carstream.str();
}

void REPLAY::GetReadyToPlay()
{
	frame = 0;
	replaymode = PLAYING;
	inputbuffer.clear();
	inputbuffer.resize(CARINPUT::GAME_ONLY_INPUTS_START_HERE, 0);

	//set the playback position at the beginning
	cur_inputframe = 0;
	cur_stateframe = 0;
}

bool REPLAY::StartPlaying(const std::string & replayfilename, std::ostream & error_output)
{
	GetReadyToPlay();

	//open the file
	std::ifstream replaystream(replayfilename.c_str(), std::ios::binary);
	if (!replaystream)
	{
		error_output << "Error loading replay file: " << replayfilename << std::endl;
		return false;
	}

	//load the header info from the file
	if (!LoadHeader(replaystream, error_output)) return false;

	//load all of the input/state frame chunks from the file until we hit the EOF
	while (Load(replaystream));

	return true;
}

void REPLAY::StopRecording(const std::string & replayfilename)
{
	replaymode = IDLE;
	if (!replayfilename.empty())
	{
		std::ofstream f(replayfilename.c_str(), std::ios::binary);
		if (f)
		{
			SaveHeader(f);
			Save(f);
		}
	}
}

void REPLAY::StopPlaying()
{
	replaymode = IDLE;
}

void REPLAY::RecordFrame(const std::vector <float> & inputs, CAR & car)
{
	if (!GetRecording())
		return;

	if (frame > 2000000000) //enforce a maximum recording time of about 92 days
	{
		StopRecording("");
	}

	assert(inputbuffer.size() == CARINPUT::GAME_ONLY_INPUTS_START_HERE);
	assert((unsigned int) version_info.inputs_supported == CARINPUT::GAME_ONLY_INPUTS_START_HERE);

	//record inputs
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


	//record state
	int framespersecond = 1.0/version_info.framerate;
	if (frame % framespersecond == 0) //once per second
	{
		std::stringstream statestream;
		joeserialize::BinaryOutputSerializer serialize_output(statestream);
		car.Serialize(serialize_output);
		stateframes.push_back(STATEFRAME(frame));
		//stateframes.push_back(STATEFRAME(11189196)); //for debugging; in hex, 11189196 is AABBCC
		//cout << "Recording state size: " << statestream.str().length() << endl; //should be ~984
		stateframes.back().SetBinaryStateData(statestream.str());
		stateframes.back().SetInputSnapshot(inputs);
	}

	frame++;
}

const std::vector <float> & REPLAY::PlayFrame(CAR & car)
{
	if (!GetPlaying())
	{
		return inputbuffer;
	}

	frame++;

	assert(inputbuffer.size() == CARINPUT::GAME_ONLY_INPUTS_START_HERE);
	assert((unsigned int) version_info.inputs_supported == CARINPUT::GAME_ONLY_INPUTS_START_HERE);

	//fast forward through the inputframes until we're up to date
	while (cur_inputframe < inputframes.size() && inputframes[cur_inputframe].GetFrame() <= frame)
	{
		ProcessPlayInputFrame(inputframes[cur_inputframe]);
		cur_inputframe++;
	}

	//fast forward through the stateframes until we're up to date
	while (cur_stateframe < stateframes.size() && stateframes[cur_stateframe].GetFrame() <= frame)
	{
		if (stateframes[cur_stateframe].GetFrame() == frame) ProcessPlayStateFrame(stateframes[cur_stateframe], car);
		cur_stateframe++;
	}

	//detect end of input
	if (cur_stateframe == stateframes.size() && cur_inputframe == inputframes.size())
	{
		StopPlaying();
	}

	return inputbuffer;
}

void REPLAY::ProcessPlayInputFrame(const INPUTFRAME & frame)
{
	for (unsigned i = 0; i < frame.GetNumInputs(); i++)
	{
		std::pair <int, float> input_pair = frame.GetInput(i);
		inputbuffer[input_pair.first] = input_pair.second;
	}
}

void REPLAY::ProcessPlayStateFrame(const STATEFRAME & frame, CAR & car)
{
	//process input snapshot
	for (unsigned i = 0; i < inputbuffer.size() && i < frame.GetInputSnapshot().size(); i++)
	{
		inputbuffer[i] = frame.GetInputSnapshot()[i];
	}

	//process binary car state
	std::stringstream statestream(frame.GetBinaryStateData());
	joeserialize::BinaryInputSerializer serialize_input(statestream);
	car.Serialize(serialize_input);
}

bool REPLAY::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, track);
	_SERIALIZE_(s, cartype);
	_SERIALIZE_(s, carpaint);
	_SERIALIZE_(s, carfile);
	_SERIALIZE_(s, carcolor);
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
	//write the file format version data manually.  if the serialization functions were used, a variable length string would be written instead, which isn't exactly what we want
	outstream.write(format_version.data(), format_version.length());

	//write the rest of the versioning info
	joeserialize::BinaryOutputSerializer serialize_output(outstream);
	Serialize(serialize_output);
}

void REPLAY::VERSION::Load(std::istream & instream)
{
	//read the file format version data manually
	const unsigned bufsize = format_version.length();
	char * version_buf = new char[bufsize+1];
	instream.read(version_buf, bufsize);
	version_buf[bufsize] = '\0';
	format_version = version_buf;
	delete [] version_buf;

	//read the rest of the versioning info
	joeserialize::BinaryInputSerializer serialize_input(instream);
	Serialize(serialize_input);
}

bool REPLAY::VERSION::operator==(const VERSION & other) const
{
	return (format_version == other.format_version &&
		inputs_supported == other.inputs_supported &&
			framerate == other.framerate);
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
