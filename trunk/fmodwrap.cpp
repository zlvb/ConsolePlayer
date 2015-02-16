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
#include "fmodwrap.h"
#include "fmod/fmod.h"
#ifdef _DEBUG
#include "fmod/fmod_errors.h"
#endif
#include "filefinder.h"

#include <map>
#include <windows.h>
#include <math.h>
#define BSTRUE 1
#define BSFALSE 0
#define DISTANCEFACTOR 1.0f
#ifndef SAFE_DELETE
#define SAFE_DELETE(p) {delete (p);(p) = NULL;}
#endif

namespace fmodwrap{
typedef std::map<FMOD_CHANNEL*,Player*> ChannelPlayers;
FMOD_SYSTEM *kFmod_system = NULL;
char        kFmod_devicename[256] = {0};
ChannelPlayers channel_players;

static FMOD_RESULT _C(FMOD_RESULT result)
{
	if (result != FMOD_OK)
	{
#if defined(_DEBUG)
		MessageBoxA(0, FMOD_ErrorString(result), "Error", MB_OK | MB_ICONERROR);
#endif
	}
    return result;
}
// -----------------------------------------------------------------------------
static void set_user_speak_mode()
{
	FMOD_SPEAKERMODE speakermode;
	FMOD_CAPS        caps;
	_C(FMOD_System_GetDriverCaps(kFmod_system, 0, &caps, 0, 0, &speakermode));
	_C(FMOD_System_SetSpeakerMode(kFmod_system, speakermode));
	if (caps & FMOD_CAPS_HARDWARE_EMULATED)      
	{                               
		// 音频播放的硬件加速被关闭，可能会影响播放性能
		_C(FMOD_System_SetDSPBufferSize(kFmod_system, 1024, 10));
	}

	_C(FMOD_System_GetDriverInfo(kFmod_system, 0, kFmod_devicename, 256, 0));

	if (strstr(kFmod_devicename, "SigmaTel"))   
	{// 据说是防止Sigmatel声音设备在播放PCM 16bit时发出破裂声，所以把采样率设置为48000
	 // 总之FMOD的例子是这么做的……
		_C(FMOD_System_SetSoftwareFormat(kFmod_system, 48000, FMOD_SOUND_FORMAT_PCMFLOAT, 0,0, FMOD_DSP_RESAMPLER_LINEAR));
	}
}
// -----------------------------------------------------------------------------
static void set_stero_speak_mode()
{
	_C(FMOD_System_SetSpeakerMode(kFmod_system, FMOD_SPEAKERMODE_STEREO));
}
// -----------------------------------------------------------------------------
// 状态回调
static FMOD_RESULT F_CALLBACK fmod_callback(FMOD_CHANNEL *channel, FMOD_CHANNEL_CALLBACKTYPE type, void* /*commanddata1*/, void* /*commanddata2*/)
{
	if (type != FMOD_CHANNEL_CALLBACKTYPE_END)
		return FMOD_OK;
	ChannelPlayers::iterator it = channel_players.find(channel);
	if (it == channel_players.end())
		return FMOD_OK;
	Player *player = it->second;
	if (!player->looped())
		player->Reload();
	if (!player->get_endcallback())
		return FMOD_OK;
	if (!player->sound())
		return FMOD_OK;
	(*player->get_endcallback())(player);
	return FMOD_OK;
}
// -----------------------------------------------------------------------------
void Open()
{
	_C(FMOD_System_Create(&kFmod_system));  // 创建fmod系统
	
	int numdrivers = 0;
	_C(FMOD_System_GetNumDrivers(kFmod_system, &numdrivers));   // 得到声卡数

	if (numdrivers == 0)    // 没有声卡
    {
		_C(FMOD_System_SetOutput(kFmod_system, FMOD_OUTPUTTYPE_NOSOUND));
    }
	else
    {
		set_user_speak_mode();
    }

	FMOD_RESULT result = FMOD_System_Init(kFmod_system, 4093, FMOD_INIT_NORMAL, NULL);

	if (result == FMOD_ERR_OUTPUT_CREATEBUFFER)         
	{// 扬声器模式不被声卡支持，切回立体声模式
		set_stero_speak_mode();
		_C(FMOD_System_Init(kFmod_system, 4093, FMOD_INIT_NORMAL, NULL));
	}

	_C(FMOD_System_Set3DSettings(kFmod_system, 1.0f, DISTANCEFACTOR, 1.0f));
	_C(FMOD_System_SetGeometrySettings(kFmod_system, 2000.0f));
}
// -----------------------------------------------------------------------------
void Close()
{
	_C(FMOD_System_Close(kFmod_system));
	_C(FMOD_System_Release(kFmod_system));
}
// -----------------------------------------------------------------------------
Sound *CreateSound(const char *name)
{
	int mode = FMOD_2D|FMOD_CREATESTREAM|FMOD_HARDWARE;
	char pcPath[MAX_PATH];
	ff_standardize_path(name, pcPath);

	Sound *new_sound = new Sound();
	if(!new_sound->LoadSoundFile(pcPath, mode))
    {
        SAFE_DELETE(new_sound);            
        return NULL;
    }

	return new_sound;
}
// -----------------------------------------------------------------------------
Player *CreatePlayer()
{
	return new Player();
}
// -----------------------------------------------------------------------------
void DestroySound( Sound *sound )
{
	if (!sound)
		return;
    SAFE_DELETE(sound);
}
// -----------------------------------------------------------------------------
void DestroyPlayer( Player *player )
{
    SAFE_DELETE(player);
}
// -----------------------------------------------------------------------------
void Update()
{
	_C(FMOD_System_Update(kFmod_system));
}
// -----------------------------------------------------------------------------
float master_volume()
{
	FMOD_CHANNELGROUP *master_group;
	_C(FMOD_System_GetMasterChannelGroup(kFmod_system, &master_group));
	float vol;
	_C(FMOD_ChannelGroup_GetVolume(master_group, &vol));
	return vol;
}
// -----------------------------------------------------------------------------
void master_volume( float vol )
{
	if (vol > 1.0f)
		vol = 1.0f;
	FMOD_CHANNELGROUP *master_group;
	_C(FMOD_System_GetMasterChannelGroup(kFmod_system, &master_group));
	_C(FMOD_ChannelGroup_SetVolume(master_group,vol));
}
//////////////////////////////////////////////////////////////////////////
bool Sound::LoadSoundFile( const char *name, int mode )
{
	FMOD_RESULT result = FMOD_System_CreateSound(kFmod_system, name, mode, NULL, &fmod_sound_);
	if (result != FMOD_OK)
	{
		_C(result);
		return false;
	}
	return true;
}
// -----------------------------------------------------------------------------
Sound::~Sound()
{
	_C(FMOD_Sound_Release(fmod_sound_));
}
// -----------------------------------------------------------------------------
unsigned int Sound::length() const
{
	unsigned int len;
	_C(FMOD_Sound_GetLength(fmod_sound_,&len,FMOD_TIMEUNIT_MS));
	return len;
}
// -----------------------------------------------------------------------------
int Sound::channels() const
{
	int num_channels = 0;
	_C(FMOD_Sound_GetFormat(fmod_sound_,NULL,NULL,&num_channels,NULL));
	return num_channels;
}
//////////////////////////////////////////////////////////////////////////
void Player::Load(Sound *sound)
{
	Unload();
    if (!sound)
    {
        return;
    }
	sound_ = sound;
	_C(FMOD_System_PlaySound(kFmod_system, FMOD_CHANNEL_FREE, sound_->get_fmod_sound(), BSTRUE, &fmod_channel_));
	_C(FMOD_Channel_SetCallback(fmod_channel_, fmod_callback));
	looped(looped());
	channel_players[fmod_channel_] = this;
	if (volume_ > -0xFFFF)
		volume(volume_);
}
// -----------------------------------------------------------------------------
void Player::Unload()
{
	if (!fmod_channel_)
		return;
	sound_ = NULL;
	channel_players.erase(fmod_channel_);
	_C(FMOD_Channel_Stop(fmod_channel_));    
	fmod_channel_ = NULL;
}
// -----------------------------------------------------------------------------
Player::Player()
	:sound_(NULL),looped_(false),fmod_channel_(NULL),volume_(-0xFFFF),endcallback_(NULL)
{

}
// -----------------------------------------------------------------------------
Player::~Player()
{
	Unload();
}
// -----------------------------------------------------------------------------
void Player::Play( bool belooped )
{
	looped(belooped);
	paused(false);
}
// -----------------------------------------------------------------------------
void Player::paused( bool paused )
{
	_C(FMOD_Channel_SetPaused(fmod_channel_, (FMOD_BOOL)paused));
}
// -----------------------------------------------------------------------------
bool Player::paused() const
{
	FMOD_BOOL paused = BSFALSE;
	_C(FMOD_Channel_GetPaused(fmod_channel_, &paused));
	return paused==BSTRUE;
}
// -----------------------------------------------------------------------------
void Player::looped( bool belooped )
{
	looped_ = belooped;
	if (!fmod_channel_)
		return;
	_C(FMOD_Channel_SetMode(fmod_channel_, looped_?FMOD_LOOP_NORMAL:FMOD_LOOP_OFF));
}
// -----------------------------------------------------------------------------
void Player::Stop()
{
	if (!playing())
		return;
	paused(true);
	elapse(0);
}
// -----------------------------------------------------------------------------
bool Player::playing()
{
	FMOD_BOOL is_playing = BSFALSE;
	if (fmod_channel_ && sound_)                                // 当channel关闭时，会返回错误信息
		FMOD_Channel_IsPlaying(fmod_channel_, &is_playing);     // 此时当做正常情况，所以不用_C检测错误
	return is_playing == BSTRUE;
}
// -----------------------------------------------------------------------------
void Player::volume( float vol )
{
	if (vol > 1.0f)
		vol = 1.0f;
	volume_ = vol;
	if (fmod_channel_)
		_C(FMOD_Channel_SetVolume(fmod_channel_, volume_));
}
// -----------------------------------------------------------------------------
float Player::volume() const
{
	if (fmod_channel_ && volume_ < -0xFFF0)
	{
		_C(FMOD_Channel_GetVolume(fmod_channel_, &volume_));
	}
	return volume_;
}
// -----------------------------------------------------------------------------
unsigned int Player::elapse()
{
	if (!fmod_channel_)
		return 0;
	unsigned int elap;
	_C(FMOD_Channel_GetPosition(fmod_channel_, &elap, FMOD_TIMEUNIT_MS));
	return elap;
}
// -----------------------------------------------------------------------------
void Player::elapse( unsigned int elap )
{
	if (!fmod_channel_)
		return;
	_C(FMOD_Channel_SetPosition(fmod_channel_, elap, FMOD_TIMEUNIT_MS));
}
// -----------------------------------------------------------------------------
float *Player::GetSpectrum( int channel_offset )
{
	FMOD_Channel_GetSpectrum(fmod_channel_,rate_[channel_offset],512,channel_offset,FMOD_DSP_FFT_WINDOW_BLACKMANHARRIS);
	return rate_[channel_offset];
}
// -----------------------------------------------------------------------------
void Player::Reload()
{
	if (sound_)
	{
		Load(sound_);
	}
}
}


