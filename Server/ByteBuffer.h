#pragma once
#include "stdheader.h"

class ByteBuffer
{
private:
	byte* buffer{ nullptr };

	int offset{ 0 };
	int capacity{ 0 };
public:
	ByteBuffer();
	~ByteBuffer();

	void Append(const byte* data, int size);
	void Resize(int n);
	void Clear();

	byte* Buffer() { return buffer; }
	int GetSize() { return offset; }
};
