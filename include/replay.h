#ifndef _REPLAY_H
#define _REPLAY_H

#include "car.h"
#include "joeserialize.h"
#include "macros.h"

#include <iostream>
#include <string>

class REPLAY
{
friend class joeserialize::Serializer;
private:
	//sub-classes
	class REPLAYVERSIONING
	{
		public:
			std::string format_version;
			int inputs_supported;
			float framerate;
			
			REPLAYVERSIONING() : format_version("VDRIFTREPLAYV??"), inputs_supported(0), framerate(0) {}
			REPLAYVERSIONING(const std::string & ver, unsigned int ins, float newfr) : format_version(ver),
				     inputs_supported(ins), framerate(newfr) {}
				     
			bool Serialize(joeserialize::Serializer & s)
			{
				_SERIALIZE_(s,inputs_supported);
				_SERIALIZE_(s,framerate);
				return true;
			}

			void Save(std::ostream & outstream)
			{
				//write the file format version data manually.  if the serialization functions were used, a variable length string would be written instead, which isn't exactly what we want
				outstream.write(format_version.data(), format_version.length());
				
				//write the rest of the versioning info
				joeserialize::BinaryOutputSerializer serialize_output(outstream);
				Serialize(serialize_output);
			}
			
			void Load(std::istream & instream)
			{
				//read the file format version data manually
				const unsigned int bufsize = format_version.length()+1;
				char * version_buf = new char[bufsize+1];
				instream.get(version_buf, bufsize);
				version_buf[bufsize] = '\0';
				format_version = version_buf;
				delete [] version_buf;
				
				//read the rest of the versioning info
				joeserialize::BinaryInputSerializer serialize_input(instream);
				Serialize(serialize_input);
			}
			
			bool operator==(const REPLAYVERSIONING & other) const
			{
				return (format_version == other.format_version && 
					inputs_supported == other.inputs_supported && 
				        framerate == other.framerate);
			}
	};
	
	class INPUTFRAME
	{
	private:
		friend class joeserialize::Serializer;
		class INPUTPAIR
		{
		public:
			INPUTPAIR() : index(0), value(0) {}
			INPUTPAIR(int newindex, float newvalue) : index(newindex), value(newvalue) {}
			
			int index;
			float value;
			
			bool Serialize(joeserialize::Serializer & s)
			{
				_SERIALIZE_(s,index);
				_SERIALIZE_(s,value);
				return true;
			}
		};
		
		int frame;
		std::vector <INPUTPAIR> inputs;
		
	public:
		INPUTFRAME() : frame(0) {}
		INPUTFRAME(int newframe) : frame(newframe) {}
		
		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,frame);
			_SERIALIZE_(s,inputs);
			return true;
		}
		
		void AddInput(int index, float value)
		{
			inputs.push_back(INPUTPAIR(index,value));
		}
		
		unsigned int GetNumInputs() const {return inputs.size();}

		int GetFrame() const
		{
			return frame;
		}
		
		///returns a pair for the <control id, value> of the indexed input
		std::pair <int, float> GetInput(unsigned int index) const {assert(index < inputs.size());return std::pair <int, float> (inputs[index].index, inputs[index].value);}
	};
	
	class STATEFRAME
	{
	private:
		friend class joeserialize::Serializer;
		
		int frame;
		std::string binary_state_data;
		std::vector <float> input_snapshot;
		
	public:
		STATEFRAME() : frame(0) {}
		STATEFRAME(int newframe) : frame(newframe) {}
		
		bool Serialize(joeserialize::Serializer & s)
		{
			_SERIALIZE_(s,frame);
			_SERIALIZE_(s,binary_state_data);
			_SERIALIZE_(s,input_snapshot);
			//std::cout << binary_state_data.length() << std::endl;
			return true;
		}

		void SetBinaryStateData (const std::string & value)
		{
			binary_state_data = value;
			//std::cout << binary_state_data.length() << std::endl;
		}

		int GetFrame() const
		{
			return frame;
		}

		std::string GetBinaryStateData() const
		{
			//std::cout << binary_state_data.length() << std::endl;
			return binary_state_data;
		}

		const std::vector < float > & GetInputSnapshot() const
		{
			return input_snapshot;
		}

		void SetInputSnapshot ( const std::vector< float >& value )
		{
			input_snapshot = value;
		}
	};
	
	//version information
	REPLAYVERSIONING version_info;
	
	//serialized
	std::string track;
	std::string cartype; //car type, used for loading graphics and sound
	std::string carpaint; //car paint id string
	std::string carfile; //entire contents of the car file (e.g. XS.car)
	float carcolor_r;
	float carcolor_g;
	float carcolor_b;
	std::vector <INPUTFRAME> inputframes;
	std::vector <STATEFRAME> stateframes;
	
	//not stored in the replay file
	int frame;
	enum
	{
		IDLE,
		RECORDING,
		PLAYING
	} replaymode;
	std::vector <float> inputbuffer;
	unsigned int cur_inputframe;
	unsigned int cur_stateframe;
	
	//functions
	void ProcessPlayInputFrame(const INPUTFRAME & frame);
	void ProcessPlayStateFrame(const STATEFRAME & frame, CAR & car);
	bool Load(std::istream & instream); ///< load one input and state frame chunk from the stream. returns true on success, returns false for EOF
	bool LoadHeader(std::istream & instream, std::ostream & error_output); ///< returns true on success.
	void Save(std::ostream & outstream); ///< save all input and state frames to the stream and then clear them
	void SaveHeader(std::ostream & outstream); ///< write only the header information to the stream
	void GetReadyToPlay();
	void GetReadyToRecord();
	
public:
	REPLAY(float framerate);
	
	///< returns true on success
	bool StartPlaying(
		const std::string & replayfilename,
		std::ostream & error_output);
	
	void StopPlaying();
	
	///< returns true if the replay system is currently playing
	bool GetPlaying() const {return (replaymode == PLAYING);} 
	
	const std::vector <float> & PlayFrame(CAR & car);
	
	void StartRecording(
		const std::string & newcartype,
		const std::string & newcarpaint,
		float r, float g, float b,
		const PTree & carconfig,
		const std::string & trackname,
		std::ostream & error_log);
	
	///< if replayfilename is empty, do not save the data
	void StopRecording(const std::string & replayfilename);
	
	///< returns true if the replay system is currently recording
	bool GetRecording() const {return (replaymode == RECORDING);}
	
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
	
	void GetCarColor(float & r, float & g, float & b) const
	{
		r = carcolor_r;
		g = carcolor_g;
		b = carcolor_b;
	}
};

#endif
