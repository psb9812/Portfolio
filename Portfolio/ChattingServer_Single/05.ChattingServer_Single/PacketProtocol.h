#pragma once

enum HeaderType
{
	Net,
	Lan
};

#pragma pack(push, 1)
struct NetHeader
{
	unsigned char code;
	unsigned short len;
	unsigned char randKey;
	unsigned char checkSum;
};

struct LanHeader
{
	unsigned short len;
};
#pragma pack(pop)