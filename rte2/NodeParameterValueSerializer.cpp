#include "NodeParameterValueSerializer.h"
#include "NodeParameter.h"
#include "memutil.h"
#include <fstream>

namespace {
	// buffer -> ptr
	uint8_t* writeFile(uint8_t* ptr, void* buffer)
	{
		rte::File* pFile = reinterpret_cast<rte::File*>(buffer);
		assert(pFile != nullptr);
		const std::string absolutePath = rte::path::normalize(pFile->path);

		// ファイル内容の読み込み
		char* fileData = nullptr;
		pFile->length = 0;
		if (rte::path::fileExists(absolutePath))
		{
			std::ifstream ifs(absolutePath, std::ifstream::binary);

			ifs.seekg(0, ifs.end);
			pFile->length = static_cast<uint32_t>(ifs.tellg());
			ifs.seekg(0, ifs.beg);

			fileData = new char[pFile->length];
			ifs.read(fileData, pFile->length);

			ifs.close();
		}

		// rte::File のシリアライズ
		const std::string& rootPath = rte::NodeParameter::getFileParameterRoot();
		const std::string relativePath = rte::string::trim1(
			rte::string::replace(absolutePath, rootPath, ""), '/');
		ptr = writeString(relativePath, ptr);
		ptr = writeUInt32(pFile->length, ptr);
		if (fileData != nullptr)
		{
			ptr = writeValue(fileData, pFile->length, ptr);
		}
		rte::mem::safeDeleteArray(&fileData);

		return ptr;
	}

	// ptr -> buffer
	uint8_t* readFile(uint8_t* ptr, void* buffer)
	{
		rte::File* pFile = reinterpret_cast<rte::File*>(buffer);
		assert(pFile != nullptr);

		// rte::File のデシリアライズ
		const std::string& rootPath = rte::NodeParameter::getFileParameterRoot();
		std::string relativePath;
		ptr = readString(&relativePath, ptr);
		strncpy_s(pFile->path, _countof(pFile->path), rte::path::combine(rootPath, relativePath).c_str(), _TRUNCATE);
		ptr = readUInt32(&pFile->length, ptr);

		// ファイル内容の書き込み
		if (pFile->length > 0)
		{
			std::ofstream ofs(pFile->path, std::ofstream::binary);

			uint8_t* data = getValuePtr(ptr);
			ofs.write(reinterpret_cast<char*>(data), pFile->length);
			ptr += pFile->length;

			ofs.close();
		}

		return ptr;
	}
}

namespace rte{

	uint8_t* NodeParameterValueSerializer::serialize(uint8_t* ptr, NodeParameter* pParam)
	{
		switch (pParam->getType())
		{
		case ParameterType::File:
			return writeFile(ptr, pParam->mpValue);

		default:
			return writeValue(pParam->mpValue, pParam->mValueSize, ptr, 'x');
		}
	}

	uint8_t* NodeParameterValueSerializer::deserialize(uint8_t* ptr, NodeParameter* pParam)
	{
		switch (pParam->getType())
		{
		case ParameterType::File:
			return readFile(ptr, pParam->mpValue);

		default:
			return readValue(pParam->mpValue, pParam->mValueSize, ptr, 'x');
		}
	}


}// namespace rte
