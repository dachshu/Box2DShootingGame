#include "ByteBuffer.h"



ByteBuffer::ByteBuffer()
{
}


ByteBuffer::~ByteBuffer()
{
	if (buffer) delete[] buffer;
}

void ByteBuffer::Append(const byte* data, int size)
{
	if (capacity < offset + size)
		Resize(offset + size);
	if (data) {
		memcpy_s(buffer + offset, capacity - offset, data, size);
		offset += size;
	}
}

void ByteBuffer::Resize(int n)
{
	if (n < offset) return;

	byte* newBuf = new byte[n];

	if (buffer) {
		memcpy_s(newBuf, n, buffer, offset);
		delete[] buffer;
	}
	buffer = newBuf;
	capacity = n;
}

void ByteBuffer::Clear()
{
	offset = 0;
}
