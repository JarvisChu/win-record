#include "audio_processor.h"
#include <Windows.h>

AudioProcessor::AudioProcessor(){
	m_offset = 0;

    // default settings
	m_org_sample_rate = 48000;
	m_org_sample_bits = 16;
	m_org_channel = 2;

	m_tgt_audio_format = AF_PCM;
	m_tgt_sample_rate = 8000;
	m_tgt_sample_bits = 16;
	m_tgt_channel = 1;

	m_seg_instan = m_org_sample_rate * m_org_channel * m_org_sample_bits / 8 / m_tgt_sample_rate; // 除数放后，增加精度，用于降采样

	uv_mutex_init(&m_lock_pcm);
	uv_mutex_init(&m_lock_silk);
}

AudioProcessor::~AudioProcessor(){
	Stop();
}

void AudioProcessor::Stop(){
	if(m_prefix.size() > 0){
		m_silk_file.Close();
		m_wave_file.Close();
	}
}

void AudioProcessor::SetOrgAudioParam(AudioFormat audio_format, int sample_rate, int sample_bits, int channel)
{
	// treat 44100 as 48000 to make downsampling work properly
	if(sample_rate == 44100){
		sample_rate = 48000;
	}

	// origin audio format: only support PCM
	m_org_sample_rate = sample_rate;
	m_org_sample_bits = sample_bits;
	m_org_channel = channel;

	// update m_seg_instan
	m_seg_instan = m_org_sample_rate * m_org_channel * m_org_sample_bits / 8 / m_tgt_sample_rate; // 除数放后，增加精度，用于降采样
}

void AudioProcessor::SetTgtAudioParam(AudioFormat audio_format, int sample_rate, int sample_bits, int channel, std::string prefix)
{
	m_tgt_audio_format = audio_format;
	m_tgt_sample_rate = sample_rate;
	m_tgt_sample_bits = sample_bits;
	m_tgt_channel = channel;

	// update m_seg_instan
	m_seg_instan = m_org_sample_rate * m_org_channel * m_org_sample_bits / 8 / m_tgt_sample_rate; // 除数放后，增加精度，用于降采样

	m_prefix = prefix;
	if(prefix.size() > 0){
		m_silk_file.Close();
		m_wave_file.Close();
		m_silk_file.Open(prefix + ".silk");
		m_wave_file.Open(prefix + ".wav", (uint32_t)sample_rate, (uint32_t)sample_bits, (uint16_t)channel);
	}
}

void AudioProcessor::OnAudioData(BYTE *pData, size_t size){

	// downsampling
	UINT offset = m_offset;
	uv_mutex_lock(&m_lock_pcm);
	
	if (pData) {
		while (offset < size){
			m_pcm.insert(m_pcm.end(), pData + offset, pData + offset + m_tgt_channel * m_tgt_sample_bits / 8);
			offset += m_seg_instan;
		}
		m_offset = offset - size;
	}else {
		size_t insterCount = size / m_seg_instan;
		m_pcm.insert(m_pcm.end(), insterCount * m_tgt_channel * m_tgt_sample_bits / 8, 0);
	}

	//encode silk
	if ( m_tgt_audio_format == AF_SILK){
		// silk encode 20ms pcm  audio data each time; looping to get 20ms from m_pcm
		int nBytesPer20ms = m_tgt_sample_rate * m_tgt_sample_bits * m_tgt_channel / 8 / 50; // bytes count of 20ms pcm audio data 
		while ((int)m_pcm.size() > nBytesPer20ms) {
			m_silk_encoder.Encode(m_tgt_sample_rate, 20, m_pcm, m_silk);
			if(m_prefix.size() > 0){
				m_pcm_cpy.insert(m_pcm_cpy.end(), m_pcm.begin(), m_pcm.begin() + nBytesPer20ms);
			}
			m_pcm.erase(m_pcm.begin(), m_pcm.begin() + nBytesPer20ms);	
		}
	}

	uv_mutex_unlock(&m_lock_pcm);

	return;
}

void AudioProcessor::GetAudioData(std::vector<BYTE> &bufferOut){
	uv_mutex_lock(&m_lock_pcm);
	if (m_tgt_audio_format == AF_SILK) {
		if(m_prefix.size() > 0){
			m_silk_file.Write(m_silk, m_silk.size());
			m_wave_file.Write(m_pcm_cpy, m_pcm_cpy.size());
			m_pcm_cpy.clear();
		}

		bufferOut.swap(m_silk);
		m_silk.clear();	
	}else {
		if(m_prefix.size() > 0){
			m_wave_file.Write(m_pcm, m_pcm.size());
		}
		bufferOut.swap(m_pcm);
		m_pcm.clear();
	}
	uv_mutex_unlock(&m_lock_pcm);
}