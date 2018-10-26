/*
	Copyright (c) 2014 Zhang li

	Permission is hereby granted, free of charge, to any person obtaining a copy
	of this software and associated documentation files (the "Software"), to deal
	in the Software without restriction, including without limitation the rights
	to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
	copies of the Software, and to permit persons to whom the Software is
	furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
	OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
	THE SOFTWARE.

	MIT License: http://www.opensource.org/licenses/mit-license.php
*/

/*
	Author zhang li
	Email zlvbvbzl@gmail.com
*/

#ifndef FMODWRAP
#define FMODWRAP

#include "fmod/fmod.h"

namespace fmodwrap{
class Player;
class Sound;

void Open();
void Close();
Sound *CreateSound(const char *name);
Player *CreatePlayer();
void DestroySound(Sound *sound);
void DestroySound(const char *name);
void DestroyPlayer(Player *player);
void Update();
float master_volume();
void master_volume(float vol);
typedef void (*PLAYER_END_CALLBACK) (Player *player);
class Player
{
public:
	void Play(bool looped);
	void Stop();
	void paused(bool paused);
	bool paused() const;
	void Load(Sound *sound);
	void Unload();
	void set_endcallback(const PLAYER_END_CALLBACK endcallback) { endcallback_ = endcallback; }
	PLAYER_END_CALLBACK get_endcallback() const {return endcallback_;}
	unsigned int elapse();
	void elapse(unsigned int elap);
	void volume(float vol);
	float volume() const;
	void looped(bool belooped);
	bool looped() const {return looped_;}
	bool playing();
	Sound *sound() const {return sound_;}
	void GetSpectrum();
	void Reload();
    FMOD_DSP_PARAMETER_FFT *Fftparameter() const { return fftparameter_; }
private:
	Player();
	~Player();
	friend Player *CreatePlayer();
	friend void DestroyPlayer(Player *player);
	FMOD_CHANNEL    *fmod_channel_;
	Sound			*sound_;
	bool			looped_;
	mutable float	volume_;
	PLAYER_END_CALLBACK endcallback_;
    FMOD_DSP_PARAMETER_FFT *fftparameter_ = nullptr;
    FMOD_CHANNELGROUP *channerlgroup_ = nullptr;
    FMOD_DSP *dsp_ = nullptr;
};

class Sound
{
public:    
	bool streamed() const;
	unsigned int length() const;
	int channels() const;
	void maxdist(float r);
	FMOD_SOUND *get_fmod_sound() const {return fmod_sound_;}
private:	
	bool LoadSoundFile(const char *name, int mode);	
	friend Sound *CreateSound();
	friend void DestroySound(Sound *sound);
	friend void DestroySound(const char *name);	
    friend void ClearSoundCache();
    Sound():fmod_sound_(0){}
    ~Sound();
	friend class Player;
	friend Sound *CreateSound(const char *name);
	FMOD_SOUND *fmod_sound_;
};
}

#endif 
