#include <stdio.h>
#include "wave_file.h"
#include "base.h"

CWaveFile::CWaveFile()
{
	m_fp = nullptr;
}

CWaveFile::~CWaveFile()
{
	Close();
}

void CWaveFile::Open(std::string path, uint32_t sample_rate, uint16_t sample_bits, uint16_t channels)
{
	if (path.size() == 0) return;
	if (m_fp) return;
	
	std::wstring wpath = UTF82Wide(path);
	errno_t err = _wfopen_s(&m_fp, wpath.c_str(), L"wb");
	if (err != 0){
		printf("open wave file failed, %s\n", path.c_str());
		return;
	}

	InitHeader(sample_rate, sample_bits, channels);
}

void CWaveFile::Write(const std::vector<BYTE>& data, size_t count)
{
	if (m_fp && count > 0 && data.size() >= count) {
		fwrite(&data[0], sizeof(BYTE), count, m_fp);
		m_len += count;
	}
}

void CWaveFile::Close()
{
	if (m_fp) {
		// 回填长度字段
		m_header.chunk_size = 38 + m_len; // 38 = 46 (header size) - 8(sizeof chunk+ sizeof chunk_size) 
		fseek(m_fp, 4, SEEK_SET);
		fwrite(&m_header.chunk_size, sizeof(char), sizeof(m_header.chunk_size), m_fp);

		m_header.subchunk_data.subchunk_size = m_len;
		fseek(m_fp, 46 - 4, SEEK_SET);
		fwrite(&m_header.subchunk_data.subchunk_size, sizeof(char), sizeof(m_header.subchunk_data.subchunk_size), m_fp);
		
		fclose(m_fp);
		m_fp = nullptr;
	}
}

void CWaveFile::InitHeader(uint32_t sample_rate, uint16_t sample_bits, uint16_t channels)
{
	if (!m_fp) return;

	m_header.fourcc = MAKE_FOURCC('R', 'I', 'F', 'F'); 
	fwrite(&m_header.fourcc, sizeof(char), sizeof(m_header.fourcc), m_fp);
	m_header.chunk_size = 0; // 等文件写完之后，再回填该字段
	fwrite(&m_header.chunk_size, sizeof(char), sizeof(m_header.chunk_size), m_fp);
	m_header.form_type = MAKE_FOURCC('W', 'A', 'V', 'E');
	fwrite(&m_header.form_type, sizeof(char), sizeof(m_header.form_type), m_fp);

	m_header.subchunk_format.chunk = MAKE_FOURCC('f', 'm', 't', ' ');
	fwrite(&m_header.subchunk_format.chunk, sizeof(char), sizeof(m_header.subchunk_format.chunk), m_fp);
	m_header.subchunk_format.subchunk_size = 18;
	fwrite(&m_header.subchunk_format.subchunk_size, sizeof(char), sizeof(m_header.subchunk_format.subchunk_size), m_fp);
	m_header.subchunk_format.audio_format = 1;
	fwrite(&m_header.subchunk_format.audio_format, sizeof(char), sizeof(m_header.subchunk_format.audio_format), m_fp);
	m_header.subchunk_format.channels = channels;
	fwrite(&m_header.subchunk_format.channels, sizeof(char), sizeof(m_header.subchunk_format.channels), m_fp);
	m_header.subchunk_format.sample_rate = sample_rate;
	fwrite(&m_header.subchunk_format.sample_rate, sizeof(char), sizeof(m_header.subchunk_format.sample_rate), m_fp);
	m_header.subchunk_format.byte_rate = sample_rate*channels*sample_bits / 8;
	fwrite(&m_header.subchunk_format.byte_rate, sizeof(char), sizeof(m_header.subchunk_format.byte_rate), m_fp);
	m_header.subchunk_format.block_align = channels*sample_bits / 8;
	fwrite(&m_header.subchunk_format.block_align, sizeof(char), sizeof(m_header.subchunk_format.block_align), m_fp);
	m_header.subchunk_format.bits_per_sample = sample_bits;
	fwrite(&m_header.subchunk_format.bits_per_sample, sizeof(char), sizeof(m_header.subchunk_format.bits_per_sample), m_fp);
	m_header.subchunk_format.ex_size = 0;
	fwrite(&m_header.subchunk_format.ex_size, sizeof(char), sizeof(m_header.subchunk_format.ex_size), m_fp);

	m_header.subchunk_data.chunk = MAKE_FOURCC('d', 'a', 't', 'a');
	fwrite(&m_header.subchunk_data.chunk, sizeof(char), sizeof(m_header.subchunk_data.chunk), m_fp);
	m_header.subchunk_data.subchunk_size = 0;  // 等文件写完之后，再回填该字段
	fwrite(&m_header.subchunk_data.subchunk_size, sizeof(char), sizeof(m_header.subchunk_data.subchunk_size), m_fp);
}