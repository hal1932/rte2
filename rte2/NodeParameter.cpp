#include "NodeParameter.h"
#include "Node.h"
#include "Context.h"
#include "memutil.h"
#include "NodeParameterValueSerializer.h"

namespace {
	size_t allocValueBuffer(void** ppValue, rte::ParameterType type, size_t valueSize = 0)
	{
		assert(type != rte::ParameterType::Invalid);

		switch (type)
		{
		case rte::ParameterType::Int32:
			*ppValue = new int32_t();
			return sizeof(int32_t);

		case rte::ParameterType::Float32:
			*ppValue = new float();
			return sizeof(float);

		case rte::ParameterType::Vector3:
			*ppValue = new rte::Vector3();
			return sizeof(rte::Vector3);

		case rte::ParameterType::String:
			if (valueSize > 0)
			{
				*ppValue = new char[valueSize];
			}
			else
			{
				*ppValue = nullptr;
			}
			return valueSize;

		case rte::ParameterType::File:
			*ppValue = new rte::File();
			return sizeof(rte::File);
		}

		return 0;
	}

	void freeValueBuffer(void** ppValue, rte::ParameterType type)
	{
		switch (type)
		{
		case rte::ParameterType::String:
			rte::mem::safeDeleteArray(ppValue);
			break;

		default:
			rte::mem::safeDelete(ppValue);
			break;
		}
	}
}

namespace rte {

	std::string NodeParameter::mFileParameterRoot;

	void NodeParameter::setFileParameterRoot(const std::string& rootPath)
	{
		mFileParameterRoot = path::normalize(rootPath);
	}

	const std::string& NodeParameter::getFileParameterRoot()
	{
		return mFileParameterRoot;
	}

	NodeParameter::NodeParameter()
		: NodeBase("", nullptr)
	{ }

	NodeParameter::NodeParameter(const std::string& name, Node* pParent, ParameterType type)
		: NodeBase(name, pParent)
		, mType(ParameterType::Invalid)
		, mpValue(nullptr)
	{
		assert(pParent != nullptr);
		pParent->addParemeter(this);
		mType = type;
		mValueSize = allocValueBuffer(&mpValue, mType);
	}

	NodeParameter::~NodeParameter()
	{
		freeValueBuffer(&mpValue, mType);
	}

	void NodeParameter::setValue(uint8_t* pNewValue, size_t newValueSize)
	{
		assert(newValueSize == mValueSize);
		memcpy(mpValue, pNewValue, mValueSize);
	}

	void NodeParameter::getValue(uint8_t* pOut, size_t valueSize)
	{
		assert(valueSize == mValueSize);
		memcpy(pOut, mpValue, mValueSize);
	}

	uint8_t* NodeParameter::serialize(uint8_t* buffer, Context*)
	{
		uint8_t* ptr = static_cast<uint8_t*>(buffer);
		ptr = writeString(getPath(), ptr);
		ptr = writeUInt16(static_cast<uint16_t>(mType), ptr);
		ptr = writeUInt32(static_cast<uint32_t>(mValueSize), ptr);
		ptr = NodeParameterValueSerializer::serialize(ptr, this);
		return ptr;
	}

	uint8_t* NodeParameter::deserialize(uint8_t* buffer, Context* pContext)
	{
		uint8_t* ptr = static_cast<uint8_t*>(buffer);

		std::string path;
		ptr = readString(&path, ptr);
		setPath(path, pContext);

		ptr = readUInt16(reinterpret_cast<uint16_t*>(&mType), ptr);
		ptr = readUInt32(reinterpret_cast<uint32_t*>(&mValueSize), ptr);

		freeValueBuffer(&mpValue, mType);
		const size_t valueSize = allocValueBuffer(&mpValue, mType, mValueSize);
		assert(valueSize == mValueSize);

		ptr = NodeParameterValueSerializer::deserialize(ptr, this);

		return ptr;
	}

}// namespace rte
