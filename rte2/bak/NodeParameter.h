#pragma once
#include "common.h"
#include "NodeBase.h"
#include <string>
#include <memory>
#include <cassert>

namespace rte {

	enum class ParameterType : uint16_t
	{
		Invalid = 0,

		Int32,
		Float32,
		Vector3,
		String,
		File,
	};

#pragma pack(1)
	struct Vector3 RTE_FINAL
	{
		float x, y, z;
	};

	struct File RTE_FINAL
	{
		char path[_MAX_PATH];
		uint32_t length;
	};
#pragma pack()

	class Node;
	class Context;

	class NodeParameter : public NodeBase
	{
	public:
		static void setFileParameterRoot(const std::string& rootPath);
		static const std::string& getFileParameterRoot();

	public:
		NodeParameter();
		NodeParameter(const std::string& name, Node* pParent, ParameterType type);
		~NodeParameter();

		ParameterType getType() { return mType; }

		uint8_t* serialize(uint8_t* buffer, Context* pContext);
		uint8_t* deserialize(uint8_t* buffer, Context* pContext);

		void setValue(uint8_t* pNewValue, size_t newValueSize);
		void getValue(uint8_t* pOut, size_t valueSize);

		template<class T>
		void setValue(const T& newValue)
		{
			assert(mType != ParameterType::String);
			T* ptr = const_cast<T*>(&newValue);
			setValue(reinterpret_cast<uint8_t*>(ptr), sizeof(newValue));
		}

		template<>
		void setValue(const std::string& newValue)
		{
			mValueSize = newValue.length();

			mem::safeDeleteArray(reinterpret_cast<char**>(&mpValue));
			mpValue = new char[mValueSize];

			char* ptr = const_cast<char*>(newValue.c_str());
			setValue(reinterpret_cast<uint8_t*>(ptr), mValueSize);
		}

		// .i Ç≈ÇÃä÷êîçƒíËã`Ç™ï°éGÇ…Ç»ÇÈÇÃÇ≈Ç–Ç∆Ç‹Ç∏égÇÌÇ»Ç¢ÇÊÇ§Ç…ÇµÇƒÇ®Ç≠
#ifndef _SWIG_PY
		template<class T>
		void getValue(T* pOut)
		{
			assert(mType != ParameterType::String);
			getValue(reinterpret_cast<uint8_t*>(pOut), sizeof(T));
		}

		template<>
		void getValue(std::string* pOut)
		{
			uint8_t* ptr = new uint8_t[mValueSize + 1];
			getValue(ptr, mValueSize);
			ptr[mValueSize] = '\0';
			pOut->assign(reinterpret_cast<char*>(ptr));
		}
#endif

		template<class T>
		T getValue()
		{
			assert(mType != ParameterType::String);
			return *static_cast<T*>(mpValue);
		}

	private:
		friend class NodeParameterValueSerializer;

		ParameterType mType;
		void* mpValue;
		size_t mValueSize;

		static std::string mFileParameterRoot;
	};

	template<>
	inline std::string NodeParameter::getValue()
	{
		char* ptr = new char[mValueSize + 1];
		getValue(reinterpret_cast<uint8_t*>(ptr), mValueSize);
		ptr[mValueSize] = '\0';

		std::string value(ptr);
		mem::safeDeleteArray(&ptr);

		return value;
	}

}// namespace rte
