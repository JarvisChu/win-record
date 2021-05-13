#include "audio_processor.h"
#include <Windows.h>

AudioProcessor::AudioProcessor(){
	m_offset = 0;

	m_org_sample_rate = 48000;
	m_org_sample_bits = 16;
	m_org_channel = 2;

	m_tgt_sample_rate = 8000;
	m_tgt_sample_bits = 1;
	m_tgt_channel = 16;

	m_seg_instan = m_org_sample_rate * m_org_channel * m_org_sample_bits / 8 / m_tgt_sample_rate; // 除数放后，增加精度，用于降采样

	uv_mutex_init(&m_lock_pcm);
	uv_mutex_init(&m_lock_silk);
}

AudioProcessor::~AudioProcessor(){}

void AudioProcessor::OnAudioData(BYTE *pData, size_t size){

	// 缩小采样率	
	UINT offset = m_offset;
	uv_mutex_lock(&m_lock_pcm);
	
	if (pData) {
		while (offset < size){
			m_pcm.insert(m_pcm.end(), pData + offset, pData + offset + m_tgt_channel * m_tgt_sample_bits / 8);
			offset += m_seg_instan;
		}
		m_offset = offset - size;
		//m_buffer.insert(m_buffer.end(), pData, pData + size);
	}else {
		size_t insterCount = size / m_seg_instan;
		m_pcm.insert(m_pcm.end(), insterCount * m_tgt_channel * m_tgt_sample_bits / 8, 0);
	}
	
	size_t pcmBufferCount = m_pcm.size();

	//encode silk
	/*if (m_silkEncoder){
		// silk 编码每次20ms，所以循环从m_buffer中取20ms的音频进行编码
		int nBytesPer20ms = targetSample * targetChannel * targetBit / 8 / 50; // 20ms的数据量
		while ((int)m_buffer.size() > nBytesPer20ms) {
			m_silkEncoder->Encode(m_targetAudioFormatConfig.sampleRate, 20, m_buffer, m_silkBuffer, m_targetAudioFormatConfig.bitRate);
			if (m_bNeedSaveFile) m_waveFile.Write(m_buffer, nBytesPer20ms); // save pcm
			m_buffer.erase(m_buffer.begin(), m_buffer.begin() + nBytesPer20ms);
		}
	}*/

	uv_mutex_unlock(&m_lock_pcm);

	return;
}