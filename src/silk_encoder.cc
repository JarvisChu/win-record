#include "silk_encoder.h"

//#pragma comment(lib, "silk.lib")

CSilkEncoder::CSilkEncoder()
{
	SKP_int32 encSizeBytes = 0;
	int ret = SKP_Silk_SDK_Get_Encoder_Size(&encSizeBytes);
	if (ret != 0) {
		printf("SKP_Silk_SDK_Get_Encoder_Size failed, ret-%d\n", ret);
		return;
	}

	m_pSilkEncoder = malloc(encSizeBytes);
	SKP_SILK_SDK_EncControlStruct encStatus;
	ret = SKP_Silk_SDK_InitEncoder(m_pSilkEncoder, &encStatus);
	if (ret != 0) {
		printf("SKP_Silk_SDK_InitEncoder failed, ret-%d\n", ret);
		return;
	}
}

CSilkEncoder::~CSilkEncoder()
{
	if (m_pSilkEncoder) {
		free(m_pSilkEncoder);
		m_pSilkEncoder = nullptr;
	}
}

union Short2Bytes
{
	short s;
	BYTE bytes[2];
};

void CSilkEncoder::Encode(int sampleRate, int duration_ms, const std::vector<BYTE>& pcm_in, std::vector<BYTE>& silk_out, int bit_rate/* = 10000*/)
{
	if (nullptr == m_pSilkEncoder) return;

	SKP_SILK_SDK_EncControlStruct encControl;
	encControl.API_sampleRate = sampleRate;
	encControl.maxInternalSampleRate = sampleRate;
	encControl.packetSize = (duration_ms * sampleRate) / 1000;
	encControl.complexity = 0;
	encControl.packetLossPercentage = 0;
	encControl.useInBandFEC = 0;
	encControl.useDTX = 0;
	// 25000 -> 6倍压缩  
	// 20000 -> 7倍压缩
	// 10000 -> 13倍压缩
	// 8000  -> 16倍压缩
	encControl.bitRate = bit_rate;

	// encode
	short nBytes = 2048;
	SKP_uint8 payload[2048];
	int ret = SKP_Silk_SDK_Encode(m_pSilkEncoder, &encControl, (const short*)&pcm_in[0], encControl.packetSize, payload, &nBytes);
	if (ret) {
		printf("SKP_Silk_Encode failed, ret-%d\n", ret);
		return;
	}

	Short2Bytes s2b;
	s2b.s = nBytes;
	silk_out.insert(silk_out.end(), &s2b.bytes[0], &s2b.bytes[2]);
	silk_out.insert(silk_out.end(), &payload[0], &payload[nBytes]);
}