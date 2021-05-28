#ifndef _AUDIO_PROCESS_H
#define _AUDIO_PROCESS_H

#include <vector>
#include <napi.h>
#include <uv.h>
#include <Windows.h>
#include "silk_encoder.h"

enum WaveSource
{
	Wave_In = 1, // input audio, microphone
	Wave_Out = 2 // output audio, speeker loopback
};

enum AudioFormat{
	AF_PCM = 0,
	AF_SILK = 1
};

class AudioProcessor{
public:
	AudioProcessor();
	~AudioProcessor();
	void SetOrgAudioParam(AudioFormat audio_format, int sample_rate, int sample_bits, int channel); //audio_format: only support pcm
	void SetTgtAudioParam(AudioFormat audio_format, int sample_rate, int sample_bits, int channel);
	void OnAudioData(BYTE *pData, size_t size);
	void GetAudioData(std::vector<BYTE> &bufferOut);

private:
	CSilkEncoder m_silk_encoder;

	uv_mutex_t m_lock_pcm;
	uv_mutex_t m_lock_silk;

	std::vector<BYTE> m_pcm;
	std::vector<BYTE> m_silk;
	unsigned int m_offset;

	UINT m_org_sample_rate;
	UINT m_org_sample_bits;
	UINT m_org_channel;

	AudioFormat m_tgt_audio_format;
	UINT m_tgt_sample_rate;
	UINT m_tgt_sample_bits;
	UINT m_tgt_channel;

	UINT m_seg_instan;
};

#endif
