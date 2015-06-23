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

#include "carsound.h"
#include "tobullet.h"
#include "content/contentmanager.h"
#include "physics/cardynamics.h"
#include "sound/sound.h"
#include "cfg/ptree.h"

template <typename T>
static inline T clamp(T val, T min, T max)
{
	return (val < max) ? (val > min) ? val : min : max;
}

CarSound::CarSound() :
	psound(0),
	gearsound_check(0),
	brakesound_check(false),
	handbrakesound_check(false),
	interior(false)
{
	// ctor
}

CarSound::CarSound(const CarSound & other) :
	psound(0),
	gearsound_check(0),
	brakesound_check(false),
	handbrakesound_check(false),
	interior(false)
{
	// we don't really support copying of these suckers
	assert(!other.psound);
}

CarSound & CarSound::operator= (const CarSound & other)
{
	// we don't really support copying of these suckers
	assert(!other.psound && !psound);
	return *this;
}

CarSound::~CarSound()
{
	Clear();
}

bool CarSound::Load(
	const std::string & carpath,
	const std::string & carname,
	Sound & sound,
	ContentManager & content,
	std::ostream & error_output)
{
	assert(!psound);

	// check for sound specification file
	std::string path_aud = carpath + "/" + carname + ".aud";
	std::ifstream file_aud(path_aud.c_str());
	if (file_aud.good())
	{
		PTree aud;
		read_ini(file_aud, aud);
		enginesounds.reserve(aud.size());
		for (PTree::const_iterator i = aud.begin(); i != aud.end(); ++i)
		{
			const PTree & audi = i->second;

			std::string filename;
			std::shared_ptr<SoundBuffer> soundptr;
			if (!audi.get("filename", filename, error_output)) return false;

			enginesounds.push_back(EngineSoundInfo());
			EngineSoundInfo & info = enginesounds.back();

			if (!audi.get("MinimumRPM", info.minrpm, error_output)) return false;
			if (!audi.get("MaximumRPM", info.maxrpm, error_output)) return false;
			if (!audi.get("NaturalRPM", info.naturalrpm, error_output)) return false;

			bool powersetting;
			if (!audi.get("power", powersetting, error_output)) return false;
			if (powersetting)
				info.power = EngineSoundInfo::POWERON;
			else if (!powersetting)
				info.power = EngineSoundInfo::POWEROFF;
			else
				info.power = EngineSoundInfo::BOTH;

			info.sound_source = sound.AddSource(soundptr, 0, true, true);
			sound.SetSourceGain(info.sound_source, 0);
		}

		// set blend start and end locations -- requires multiple passes
		std::map <EngineSoundInfo *, EngineSoundInfo *> temporary_to_actual_map;
		std::list <EngineSoundInfo> poweron_sounds, poweroff_sounds;
		for (std::vector <EngineSoundInfo>::iterator i = enginesounds.begin(); i != enginesounds.end(); ++i)
		{
			if (i->power == EngineSoundInfo::POWERON)
			{
				poweron_sounds.push_back(*i);
				temporary_to_actual_map[&poweron_sounds.back()] = &*i;
			}
			else if (i->power == EngineSoundInfo::POWEROFF)
			{
				poweroff_sounds.push_back(*i);
				temporary_to_actual_map[&poweroff_sounds.back()] = &*i;
			}
		}

		poweron_sounds.sort();
		poweroff_sounds.sort();

		// we only support 2 overlapping sounds at once each for poweron and poweroff; this
		// algorithm fails for other cases (undefined behavior)
		std::list <EngineSoundInfo> * cursounds = &poweron_sounds;
		for (int n = 0; n < 2; n++)
		{
			if (n == 1)
				cursounds = &poweroff_sounds;

			for (std::list <EngineSoundInfo>::iterator i = (*cursounds).begin(); i != (*cursounds).end(); ++i)
			{
				// set start blend
				if (i == (*cursounds).begin())
					i->fullgainrpmstart = i->minrpm;

				// set end blend
				std::list <EngineSoundInfo>::iterator inext = i;
				inext++;
				if (inext == (*cursounds).end())
					i->fullgainrpmend = i->maxrpm;
				else
				{
					i->fullgainrpmend = inext->minrpm;
					inext->fullgainrpmstart = i->maxrpm;
				}
			}

			// now assign back to the actual infos
			for (std::list <EngineSoundInfo>::iterator i = (*cursounds).begin(); i != (*cursounds).end(); ++i)
			{
				assert(temporary_to_actual_map.find(&(*i)) != temporary_to_actual_map.end());
				*temporary_to_actual_map[&(*i)] = *i;
			}
		}
	}
	else
	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "engine");
		enginesounds.push_back(EngineSoundInfo());
		enginesounds.back().sound_source = sound.AddSource(soundptr, 0, true, true);
	}

	//set up tire squeal sounds
	for (int i = 0; i < 4; ++i)
	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "tire_squeal");
		tiresqueal[i] = sound.AddSource(soundptr, i * 0.25, true, true);
	}

	//set up tire gravel sounds
	for (int i = 0; i < 4; ++i)
	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "gravel");
		gravelsound[i] = sound.AddSource(soundptr, i * 0.25, true, true);
	}

	//set up tire grass sounds
	for (int i = 0; i < 4; ++i)
	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "grass");
		grasssound[i] = sound.AddSource(soundptr, i * 0.25, true, true);
	}

	//set up bump sounds
	for (int i = 0; i < 4; ++i)
	{
		std::shared_ptr<SoundBuffer> soundptr;
		if (i >= 2)
		{
			content.load(soundptr, carpath, "bump_rear");
		}
		else
		{
			content.load(soundptr, carpath, "bump_front");
		}
		tirebump[i] = sound.AddSource(soundptr, 0, true, false);
	}

	//set up crash sound
	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "crash");
		crashsound = sound.AddSource(soundptr, 0, true, false);
	}

	//set up gear sound
	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "gear");
		gearsound = sound.AddSource(soundptr, 0, true, false);
	}

	//set up brake sound
	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "brake");
		brakesound = sound.AddSource(soundptr, 0, true, false);
	}

	//set up handbrake sound
	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "handbrake");
		handbrakesound = sound.AddSource(soundptr, 0, true, false);
	}

	{
		std::shared_ptr<SoundBuffer> soundptr;
		content.load(soundptr, carpath, "wind");
		roadnoise = sound.AddSource(soundptr, 0, true, true);
	}

	psound = &sound;

	return true;
}

void CarSound::Update(const CarDynamics & dynamics, float dt)
{
	if (!psound) return;

	Vec3 pos_car = ToMathVector<float>(dynamics.GetPosition());
	Vec3 pos_eng = ToMathVector<float>(dynamics.GetEnginePosition());

	psound->SetSourcePosition(roadnoise, pos_car[0], pos_car[1], pos_car[2]);
	psound->SetSourcePosition(crashsound, pos_car[0], pos_car[1], pos_car[2]);
	psound->SetSourcePosition(gearsound, pos_car[0], pos_car[1], pos_car[2]);
	psound->SetSourcePosition(brakesound, pos_car[0], pos_car[1], pos_car[2]);
	psound->SetSourcePosition(handbrakesound, pos_car[0], pos_car[1], pos_car[2]);

	// update engine sounds
	const float rpm = dynamics.GetTachoRPM();
	const float throttle = dynamics.GetEngine().GetThrottle();
	float total_gain = 0.0;

	std::vector<std::pair<size_t, float> > gainlist;
	gainlist.reserve(enginesounds.size());
	for (std::vector<EngineSoundInfo>::iterator i = enginesounds.begin(); i != enginesounds.end(); ++i)
	{
		EngineSoundInfo & info = *i;
		float gain = 1.0;

		if (rpm < info.minrpm)
		{
			gain = 0;
		}
		else if (rpm < info.fullgainrpmstart && info.fullgainrpmstart > info.minrpm)
		{
			gain *= (rpm - info.minrpm) / (info.fullgainrpmstart - info.minrpm);
		}

		if (rpm > info.maxrpm)
		{
			gain = 0;
		}
		else if (rpm > info.fullgainrpmend && info.fullgainrpmend < info.maxrpm)
		{
			gain *= 1.0 - (rpm - info.fullgainrpmend) / (info.maxrpm - info.fullgainrpmend);
		}

		if (info.power == EngineSoundInfo::BOTH)
		{
			gain *= throttle * 0.5 + 0.5;
		}
		else if (info.power == EngineSoundInfo::POWERON)
		{
			gain *= throttle;
		}
		else if (info.power == EngineSoundInfo::POWEROFF)
		{
			gain *= (1.0-throttle);
		}

		total_gain += gain;
		gainlist.push_back(std::make_pair(info.sound_source, gain));

		float pitch = rpm / info.naturalrpm;

		psound->SetSourcePosition(info.sound_source, pos_eng[0], pos_eng[1], pos_eng[2]);
		psound->SetSourcePitch(info.sound_source, pitch);
	}

	// normalize gains
	assert(total_gain >= 0.0);
	for (std::vector<std::pair<size_t, float> >::iterator i = gainlist.begin(); i != gainlist.end(); ++i)
	{
		float gain;
		if (total_gain == 0.0)
		{
			gain = 0.0;
		}
		else if (enginesounds.size() == 1 && enginesounds.back().power == EngineSoundInfo::BOTH)
		{
			gain = i->second;
		}
		else
		{
			gain = i->second / total_gain;
		}
		psound->SetSourceGain(i->first, gain);
	}

	// update tire squeal sounds
	for (int i = 0; i < 4; i++)
	{
		// make sure we don't get overlap
		psound->SetSourceGain(gravelsound[i], 0.0);
		psound->SetSourceGain(grasssound[i], 0.0);
		psound->SetSourceGain(tiresqueal[i], 0.0);

		float squeal = dynamics.GetTireSquealAmount(WheelPosition(i));
		float maxgain = 0.3;
		float pitchvariation = 0.4;

		unsigned sound_active = 0;
		const TrackSurface & surface = dynamics.GetWheelContact(WheelPosition(i)).GetSurface();
		if (surface.type == TrackSurface::ASPHALT)
		{
			sound_active = tiresqueal[i];
		}
		else if (surface.type == TrackSurface::GRASS)
		{
			sound_active = grasssound[i];
			maxgain = 0.4; // up the grass sound volume a little
		}
		else if (surface.type == TrackSurface::GRAVEL)
		{
			sound_active = gravelsound[i];
			maxgain = 0.4;
		}
		else if (surface.type == TrackSurface::CONCRETE)
		{
			sound_active = tiresqueal[i];
			maxgain = 0.3;
			pitchvariation = 0.25;
		}
		else if (surface.type == TrackSurface::SAND)
		{
			sound_active = grasssound[i];
			maxgain = 0.25; // quieter for sand
			pitchvariation = 0.25;
		}
		else
		{
			sound_active = tiresqueal[i];
			maxgain = 0.0;
		}

		btVector3 pos_wheel = dynamics.GetWheelPosition(WheelPosition(i));
		btVector3 vel_wheel = dynamics.GetWheelVelocity(WheelPosition(i));
		float pitch = (vel_wheel.length() - 5.0) * 0.1;
		pitch = clamp(pitch, 0.0f, 1.0f);
		pitch = 1.0 - pitch;
		pitch *= pitchvariation;
		pitch = pitch + (1.0 - pitchvariation);
		pitch = clamp(pitch, 0.1f, 4.0f);

		psound->SetSourcePosition(sound_active, pos_wheel[0], pos_wheel[1], pos_wheel[2]);
		psound->SetSourcePitch(sound_active, pitch);
		psound->SetSourceGain(sound_active, squeal * maxgain);
	}

	// update road noise sound
	{
		float gain = dynamics.GetVelocity().length();
		gain *= 0.02;
		gain *= gain;
		if (gain > 1) gain = 1;
		psound->SetSourceGain(roadnoise, gain);
	}
/*	fixme
	// update bump noise sound
	{
		for (int i = 0; i < 4; i++)
		{
			suspensionbumpdetection[i].Update(
				dynamics.GetSuspension(WHEEL_POSITION(i)).GetVelocity(),
				dynamics.GetSuspension(WHEEL_POSITION(i)).GetDisplacementFraction(),
				dt);
			if (suspensionbumpdetection[i].JustSettled())
			{
				float bumpsize = suspensionbumpdetection[i].GetTotalBumpSize();

				const float breakevenms = 5.0;
				float gain = bumpsize * GetSpeed() / breakevenms;
				if (gain > 1)
					gain = 1;
				if (gain < 0)
					gain = 0;

				if (gain > 0 && !tirebump[i].Audible())
				{
					tirebump[i].SetGain(gain);
					tirebump[i].Stop();
					tirebump[i].Play();
				}
			}
		}
	}
*/
	// update crash sound
	crashdetection.Update(dynamics.GetSpeed(), dt);
	float crashdecel = crashdetection.GetMaxDecel();
	if (crashdecel > 0)
	{
		const float mingainat = 200;
		const float maxgainat = 2000;
		float gain = (crashdecel - mingainat) / (maxgainat - mingainat);
		gain = clamp(gain, 0.1f, 1.0f);

		if (!psound->GetSourcePlaying(crashsound))
		{
			psound->ResetSource(crashsound);
			psound->SetSourceGain(crashsound, gain);
		}
	}

	// update interior sounds
	if (!interior) return;

	// update gear sound
	if (gearsound_check != dynamics.GetTransmission().GetGear())
	{
		float gain = 0.0;
		if (rpm > 0.0)
			gain = dynamics.GetEngine().GetRPMLimit() / rpm;
		gain = clamp(gain, 0.25f, 0.50f);

		if (!psound->GetSourcePlaying(gearsound))
		{
			psound->ResetSource(gearsound);
			psound->SetSourceGain(gearsound, gain);
		}
		gearsound_check = dynamics.GetTransmission().GetGear();
	}
/*	fixme
	// brake sound
	if (inputs[CarInput::BRAKE] > 0 && !brakesound_check)
	{
		// disable brake sound, sounds wierd
		if (false)//!psound->GetSourcePlaying(brakesound))
		{
			psound->ResetSource(brakesound);
			psound->SetSourceGain(brakesound, 0.5);
		}
		brakesound_check = true;
	}
	if (inputs[CarInput::BRAKE] <= 0)
		brakesound_check = false;

	// handbrake sound
	if (inputs[CarInput::HANDBRAKE] > 0 && !handbrakesound_check)
	{
		if (!psound->GetSourcePlaying(handbrakesound))
		{
			psound->ResetSource(handbrakesound);
			psound->SetSourceGain(handbrakesound, 0.5);
		}
		handbrakesound_check = true;
	}
	if (inputs[CarInput::HANDBRAKE] <= 0)
		handbrakesound_check = false;
*/
}

void CarSound::EnableInteriorSound(bool value)
{
	interior = value;
}

void CarSound::Clear()
{
	if (!psound) return;

	// reverse order
	psound->RemoveSource(roadnoise);
	psound->RemoveSource(handbrakesound);
	psound->RemoveSource(brakesound);
	psound->RemoveSource(gearsound);
	psound->RemoveSource(crashsound);

	for (int i = WHEEL_POSITION_SIZE - 1; i >= 0; --i)
		psound->RemoveSource(tirebump[i]);

	for (int i = WHEEL_POSITION_SIZE - 1; i >= 0; --i)
		psound->RemoveSource(grasssound[i]);

	for (int i = WHEEL_POSITION_SIZE - 1; i >= 0; --i)
		psound->RemoveSource(gravelsound[i]);

	for (int i = WHEEL_POSITION_SIZE - 1; i >= 0; --i)
		psound->RemoveSource(tiresqueal[i]);

	for (int i = enginesounds.size() - 1; i >= 0; --i)
		psound->RemoveSource(enginesounds[i].sound_source);

	psound = 0;
}
