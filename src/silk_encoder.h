#ifndef _SILK_ENCODE_H
#define _SILK_ENCODE_H

#include <Windows.h>
#include <vector>
#include "silk/interface/SKP_Silk_SDK_API.h"

class CSilkEncoder
{
public:
	CSilkEncoder();
	~CSilkEncoder();
	void Encode(int sampleRate, int duration_ms, const std::vector<BYTE>& pcm_in, std::vector<BYTE>& silk_out, int bit_rate = 10000);
private:
	void* m_pSilkEncoder = nullptr;
};

#endif
