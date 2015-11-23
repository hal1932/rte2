#pragma once
#include "common.h"
#include <string>
#include <memory>
#include <cstdint>
#include <cassert>

inline uint8_t* writeValue(const void* value, size_t valueSize, uint8_t* buffer, char prefix = 'b')
{
	uint8_t* ptr = buffer;

	*ptr = prefix;
	++ptr;

	memcpy(ptr, value, valueSize);
	ptr += valueSize;

	return ptr;
}

inline uint8_t* readValue(void* pOut, size_t valueSize, uint8_t* buffer, char prefix = 'b')
{
	uint8_t* ptr = buffer;

	logAssert(*ptr == prefix, "invalid prefix", (char)*ptr, prefix);
	++ptr;

	memcpy(pOut, ptr, valueSize);
	ptr += valueSize;

	return ptr;
}

/*
	'i'    : 1byte
	content: 4byte
*/
inline int calcSizeInt32(int32_t)
{
	return 1 + sizeof(int32_t);
}

inline uint8_t* writeInt32(int32_t value, uint8_t* buffer)
{
	return writeValue(&value, sizeof(int32_t), buffer, 'i');
}

inline uint8_t* readInt32(int32_t* pOut, uint8_t* buffer)
{
	return readValue(pOut, sizeof(int32_t), buffer, 'i');
}

/*
	's'   : 1byte
	length: 4byte
	content: length byte
*/
inline int calcSizeString(const std::string& str)
{
	return 1 + sizeof(int32_t) + str.length();
}

inline uint8_t* writeString(const std::string& str, uint8_t* buffer)
{
	uint32_t length = static_cast<uint32_t>(str.length());

	uint8_t* ptr = writeValue(&length, sizeof(uint32_t), buffer, 's');
	if (length > 0)
	{
		memcpy(ptr, str.c_str(), length);
		ptr += str.length();
	}

	return ptr;
}

inline uint8_t* readString(std::string* pOut, uint8_t* buffer)
{
	uint32_t length;
	uint8_t* ptr = readValue(&length, sizeof(uint32_t), buffer, 's');

	if (length > 0)
	{
		std::unique_ptr<char> pStr(new char[length + 1]);
		memcpy(pStr.get(), ptr, length);
		ptr += length;
		pStr.get()[length] = '\0';
		*pOut = std::string(pStr.get());
	}
	else
	{
		pOut->clear();
	}

	return ptr;
}
