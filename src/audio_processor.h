#ifndef _AUDIO_PROCESS_H
#define _AUDIO_PROCESS_H

#include <vector>
#include <nan.h>
#include <Windows.h>

enum WaveSource
{
	Wave_In = 1, // input audio, microphone
	Wave_Out = 2 // output audio, speeker loopback
};

class AudioProcessor{
public:
	AudioProcessor();
	~AudioProcessor();
	void OnAudioData(BYTE *pData, size_t size);

private:
	uv_mutex_t m_lock_pcm;
	uv_mutex_t m_lock_silk;

	std::vector<BYTE> m_pcm;
	std::vector<BYTE> m_silk;
	unsigned int m_offset;	

	UINT m_org_sample_rate;
	UINT m_org_sample_bits;
	UINT m_org_channel;

	UINT m_tgt_sample_rate;
	UINT m_tgt_sample_bits;
	UINT m_tgt_channel;

	UINT m_seg_instan;
};

#endif
