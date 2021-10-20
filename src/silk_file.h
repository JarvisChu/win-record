#ifndef _SILK_FILE_H
#define _SILK_FILE_H

#include <vector>
#include <string>

typedef unsigned char BYTE;

// silk文件，存储格式参考了silk官方的示例(https://github.com/ploverlake/silk)，可直接使用官方的decode程序解码
// 具体格式为：
// header (固定为：#!SILK_V3) + 数据帧(N个，每个帧包括：2字节长度+数据) + 尾帧(2字节，值为-1)

class CSilkFile
{
public:
	CSilkFile();
	~CSilkFile();
	void Open(std::string path);
	void Write(const std::vector<BYTE>& data, size_t count);
	void Close();
private:
	void InitHeader();

private:
	FILE* m_fp = nullptr;
};

#endif