//#include "stdafx.h"
#include <stdio.h>
#include "silk_file.h"
#include "base.h"

CSilkFile::CSilkFile()
{
	m_fp = nullptr;
}

CSilkFile::~CSilkFile()
{
	Close();
}

void CSilkFile::Open(std::string path)
{
	if (path.size() == 0) return;
	if (m_fp) return;

	std::wstring wpath = UTF82Wide(path);
	errno_t err = _wfopen_s(&m_fp, wpath.c_str(), L"wb");
	if (err != 0) {
		printf("open silk file failed, %s\n", path.c_str());
		return;
	}

	InitHeader();
}

// format of data:  [len+data][len+data]
void CSilkFile::Write(const std::vector<BYTE>& data, size_t count)
{
	if (m_fp && count > 0 && data.size() >= count) {
		fwrite(&data[0], sizeof(BYTE), count, m_fp);
	}
}

void CSilkFile::Close()
{
	if (m_fp) {
		int nBytes = -1;
		fwrite(&nBytes, sizeof(short), 1, m_fp);
		fclose(m_fp);
		m_fp = nullptr;
	}
}

void CSilkFile::InitHeader()
{
	if (!m_fp) return;

	static const char Silk_header[] = "#!SILK_V3";
	fwrite(Silk_header, sizeof(char), strlen(Silk_header), m_fp);
}