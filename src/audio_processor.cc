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

AudioProcessor::~AudioProcessor(){}


void AudioProcessor::SetAudioParam(AudioFormat audio_format, int sample_rate, int sample_bits, int channel)
{
	m_tgt_audio_format = audio_format;
	m_tgt_sample_rate = sample_rate;
	m_tgt_sample_bits = sample_bits;
	m_tgt_channel = channel;

	// update m_seg_instan
	m_seg_instan = m_org_sample_rate * m_org_channel * m_org_sample_bits / 8 / m_tgt_sample_rate; // 除数放后，增加精度，用于降采样
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
	
	size_t pcmBufferCount = m_pcm.size();

	//encode silk
	if ( m_tgt_audio_format == AF_SILK){
		// silk encode 20ms pcm  audio data each time; looping to get 20ms from m_pcm
		int nBytesPer20ms = m_tgt_sample_rate * m_tgt_sample_bits * m_tgt_channel / 8 / 50; // bytes count of 20ms pcm audio data 
		while ((int)m_pcm.size() > nBytesPer20ms) {
			m_silk_encoder.Encode(m_tgt_sample_rate, 20, m_pcm, m_silk);
			//if (m_bNeedSaveFile) m_waveFile.Write(m_buffer, nBytesPer20ms); // save pcm
			m_pcm.erase(m_pcm.begin(), m_pcm.begin() + nBytesPer20ms);
		}
	}

	uv_mutex_unlock(&m_lock_pcm);

	return;
}

void AudioProcessor::GetAudioData(std::vector<BYTE> &bufferOut){
	uv_mutex_lock(&m_lock_pcm);
	if (m_tgt_audio_format == AF_SILK) {
		//	if (m_bNeedSaveFile) m_silkFile.Write(m_silkBuffer, m_silkBuffer.size());
		bufferOut.swap(m_silk);
		m_silk.clear();
	}else {
		//if (m_bNeedSaveFile) m_waveFile.Write(m_buffer, m_buffer.size());
		bufferOut.swap(m_pcm);
		m_pcm.clear();
	}
	uv_mutex_unlock(&m_lock_pcm);
}