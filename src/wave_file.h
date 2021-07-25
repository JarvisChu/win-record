#ifndef _WAVE_FILE_H
#define _WAVE_FILE_H

#include <vector>
#include <string>

typedef unsigned char BYTE;

#define MAKE_FOURCC(a,b,c,d) \
( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )


// format 子块
struct SubChunkFormat
{
	uint32_t chunk;           // 固定为fmt ，大端存储
	uint32_t subchunk_size;   // 子块的大小，不包含chunk+subchunk_size字段
	uint16_t audio_format;    // 编码格式(Audio Format)，1代表PCM无损格式
	uint16_t channels;        // 声道数(Channels)，1或2
	uint32_t sample_rate;     // 采样率(Sample Rate)
	uint32_t byte_rate;       // 传输速率(Byte Rate)，每秒数据字节数，SampleRate * Channels * BitsPerSample / 8
	uint16_t block_align;     // 每个采样所需的字节数BlockAlign，BitsPerSample*Channels / 8
	uint16_t bits_per_sample; // 单个采样位深(Bits Per Sample)，可选8、16或32
	uint16_t ex_size;         // 扩展块的大小，附加块的大小
};

// data 子块
struct SubChunkData
{
	uint32_t chunk;         // 固定为data，大端存储
	uint32_t subchunk_size; // 子块的大小，不包含chunk+subchunk_size字段
};

struct WaveHeader 
{
	uint32_t fourcc;     // 固定为RIFF，大端存储，所以需要使用MAKE_FOURCC宏处理
	uint32_t chunk_size; // 文件长度，不包含fourcc和chunk_size，即总文件长度-8字节
	uint32_t form_type;  // 固定为WAVE，大端存储，类型码(Form Type)，WAV文件格式标记，即"WAVE"四个字母
	
	SubChunkFormat subchunk_format; // fmt  子块
	SubChunkData subchunk_data;     // data 子块
};

class CWaveFile
{
public:
	CWaveFile();
	~CWaveFile();
	void Open(std::string path, uint32_t sample_rate, uint16_t sample_bits, uint16_t channels);
	void Write(const std::vector<BYTE>& data, size_t count);
	void Close();
private:
	void InitHeader(uint32_t sample_rate, uint16_t sample_bits, uint16_t channels);

private:
	WaveHeader m_header;
	std::string m_path;
	FILE* m_fp = nullptr;
	uint32_t m_len = 0;
};

#endif