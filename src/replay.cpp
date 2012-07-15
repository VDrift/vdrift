#include "replay.h"
#include "unittest.h"
#include "cfg/ptree.h"
#include "carinput.h"

#include <sstream>
#include <fstream>

REPLAY::REPLAY(float framerate) :
	version_info("VDRIFTREPLAYV14", CARINPUT::GAME_ONLY_INPUTS_START_HERE, framerate),
	frame(0),
	replaymode(IDLE)
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
}

void REPLAY::StartRecording(
	const std::string & trackname,
	std::ostream & error_log,
	const std::list <CAR> & cars,
 	int num_laps)
{
	track = trackname;

	carids.clear();
	cartypes.clear();
	carpaints.clear();
	carcolors.clear();
	carfiles.clear();
	
	for (std::list <CAR>::const_iterator i = cars.begin(); i != cars.end(); ++i)
	{
		carids.push_back(i->GetCarId());
		cartypes[i->GetCarId()] = i->GetCarType();
		carpaints[i->GetCarId()] = i->GetCarPaint();
		carcolors[i->GetCarId()] = i->GetCarColor();
	
		std::stringstream carstream;
		i->GetCarConfig().write(carstream);
		carfiles[i->GetCarId()] = carstream.str();
	}
	
	number_laps = num_laps;

	inputbuffers.clear();
	for (std::list<CAR>::const_iterator i = cars.begin(); i != cars.end(); ++i)
		inputbuffers[i->GetCarId()].resize(CARINPUT::GAME_ONLY_INPUTS_START_HERE, 0);
	
	GetReadyToRecord();
}

void REPLAY::GetReadyToPlay()
{
	frame = 0;
	replaymode = PLAYING;

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

	carids.clear();
	inputbuffers.clear();
	for (std::map<CARID, std::string>::const_iterator iter = cartypes.begin(); iter != cartypes.end(); ++iter) 
	{
		carids.push_back(iter->first);
		inputbuffers[iter->first].resize(CARINPUT::GAME_ONLY_INPUTS_START_HERE, 0);
	}

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

void REPLAY::RecordFrame(const std::list<CAR> & cars)
{
	if (!GetRecording())
		return;

	if (frame > 2000000000) //enforce a maximum recording time of about 92 days
	{
		StopRecording("");
	}

	//create input frame
	inputframes.push_back(INPUTFRAME(frame));
	
	//create state frame
	int framespersecond = 1.0/version_info.framerate;
	should_record_state = (frame % framespersecond == 0); //once per second
	
	if (should_record_state)
	{
		stateframes.push_back(STATEFRAME(frame));
	}
	
	frame++;
}
	
void REPLAY::RecordFrameCar(const std::vector <float> & inputs, CAR & car) 
{
	if (!GetRecording())
		return;
	
	std::vector<float>& inputbuffer = inputbuffers[car.GetCarId()];
	

	assert(inputbuffer.size() == CARINPUT::GAME_ONLY_INPUTS_START_HERE);
	assert((unsigned int) version_info.inputs_supported == CARINPUT::GAME_ONLY_INPUTS_START_HERE);

	//record the car inputs in the current input frame
	for (unsigned i = 0; i < CARINPUT::GAME_ONLY_INPUTS_START_HERE; i++)
	{
		if (inputs[i] != inputbuffer[i])
		{
			inputbuffer[i] = inputs[i];
			inputframes.back().AddInput(car.GetCarId(), i, inputs[i]);
		}
	}


	//record the car state in the current state frame
	if (should_record_state) //once per second
	{
		std::stringstream statestream;
		joeserialize::BinaryOutputSerializer serialize_output(statestream);
		car.Serialize(serialize_output);
		stateframes.back().SetBinaryStateData(car.GetCarId(), statestream.str());
		stateframes.back().SetInputSnapshot(car.GetCarId(), inputs);
	}
}

void REPLAY::PlayFrame(std::list<CAR> & cars)
{
	if (!GetPlaying())
	{
		return;
	}

	assert((unsigned int) version_info.inputs_supported == CARINPUT::GAME_ONLY_INPUTS_START_HERE);

	//fast forward through the inputframes until we're up to date
	while (cur_inputframe < inputframes.size() && inputframes[cur_inputframe].GetFrame() <= frame)
	{
		ProcessPlayInputFrame(inputframes[cur_inputframe], cars);
		cur_inputframe++;
	}

	//fast forward through the stateframes until we're up to date
	while (cur_stateframe < stateframes.size() && stateframes[cur_stateframe].GetFrame() <= frame)
	{
		if (stateframes[cur_stateframe].GetFrame() == frame)
			ProcessPlayStateFrame(stateframes[cur_stateframe], cars);
		cur_stateframe++;
	}

	//detect end of input
	if (cur_stateframe >= stateframes.size() || cur_inputframe >= inputframes.size())
	{
		StopPlaying();
	}

	frame++;
}

const std::vector<float> & REPLAY::PlayFrameCar(CAR & car)
{
	return inputbuffers[car.GetCarId()];
} 

void REPLAY::ProcessPlayInputFrame(const INPUTFRAME & frame, std::list<CAR> & cars)
{
	for (std::list<CAR>::iterator car = cars.begin(); car != cars.end(); ++car)
	{
		std::vector<float>& inputbuffer = inputbuffers[car->GetCarId()];
		
		for (unsigned i = 0; i < frame.GetNumInputs(car->GetCarId()); i++)
		{
			std::pair <int, float> input_pair = frame.GetInput(car->GetCarId(), i);
			inputbuffer[input_pair.first] = input_pair.second;
		}
	}
}

void REPLAY::ProcessPlayStateFrame(const STATEFRAME & frame, std::list<CAR> & cars)
{
	//process input snapshot
	for (std::list<CAR>::iterator car = cars.begin(); car != cars.end(); ++car)
	{
		std::vector<float>& inputbuffer = inputbuffers[car->GetCarId()];
		
		//process input snapshot
		for (unsigned i = 0; i < inputbuffer.size() && i < frame.GetInputSnapshot(car->GetCarId()).size(); i++)
		{
			inputbuffer[i] = frame.GetInputSnapshot(car->GetCarId())[i];
		}

		//process binary car state
		std::stringstream statestream(frame.GetBinaryStateData(car->GetCarId()));
		joeserialize::BinaryInputSerializer serialize_input(statestream);
		car->Serialize(serialize_input);
	}
}

bool REPLAY::Serialize(joeserialize::Serializer & s)
{
	_SERIALIZE_(s, track);
	_SERIALIZE_(s, cartypes);
	_SERIALIZE_(s, carpaints);
	_SERIALIZE_(s, carfiles);
	_SERIALIZE_(s, carcolors);
	
	_SERIALIZE_(s, inputframes);
	_SERIALIZE_(s, stateframes);
	
	_SERIALIZE_(s, number_laps);

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
