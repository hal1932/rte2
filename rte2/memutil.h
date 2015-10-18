#pragma once
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

inline uint8_t* getValuePtr(uint8_t* buffer)
{
	// prefix‚Ì•ª‚ð“Ç‚Ý”ò‚Î‚µ‚Ä•Ô‚·
	return buffer + 1;
}

inline uint8_t* readValue(void* pOut, size_t valueSize, uint8_t* buffer, char prefix = 'b')
{
	uint8_t* ptr = buffer;

	assert(*ptr == prefix);
	++ptr;

	memcpy(pOut, ptr, valueSize);
	ptr += valueSize;

	return ptr;
}

inline uint8_t* writeUInt16(uint16_t value, uint8_t* buffer)
{
	return writeValue(&value, sizeof(uint16_t), buffer, 'u');
}

inline uint8_t* readUInt16(uint16_t* pOut, uint8_t* buffer)
{
	return readValue(pOut, sizeof(uint16_t), buffer, 'u');
}

inline uint8_t* writeUInt32(uint32_t value, uint8_t* buffer)
{
	return writeValue(&value, sizeof(uint32_t), buffer, 'i');
}

inline uint8_t* readUInt32(uint32_t* pOut, uint8_t* buffer)
{
	return readValue(pOut, sizeof(uint32_t), buffer, 'i');
}

inline uint8_t* writeString(const std::string& str, uint8_t* buffer)
{
	uint32_t length = static_cast<uint32_t>(str.length());
	uint8_t* ptr = writeValue(&length, sizeof(uint32_t), buffer, 's');

	memcpy(ptr, str.c_str(), length);
	ptr += str.length();

	return ptr;
}

inline uint8_t* readString(std::string* pOut, uint8_t* buffer)
{
	uint32_t length;
	uint8_t* ptr = readValue(&length, sizeof(uint32_t), buffer, 's');

	std::unique_ptr<char> pStr(new char[length + 1]);
	memcpy(pStr.get(), ptr, length);
	ptr += length;
	pStr.get()[length] = '\0';
	*pOut = std::string(pStr.get());

	return ptr;
}
