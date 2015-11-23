#pragma once
#include "common.h"

namespace rte
{
	class NodeContentData : public Serializable, private noncopyable, private nonmovable
	{
	public:
		enum class Type : uint8_t
		{
			Invalid,
			Int32,
		};

	public:
		NodeContentData() = delete;
		~NodeContentData() = default;

#ifdef _SWIG_PY
		bool operator==(const NodeContentData& other)
		{
			return (this == &other);
		}
#endif

		virtual int calcSize() = 0;
		virtual uint8_t* serialize(uint8_t* buffer) = 0;
		virtual uint8_t* deserialize(uint8_t* buffer) = 0;

		Type getType() { return mType; }
		Type getType() const { return mType; }

	protected:
		NodeContentData(Type type)
			: mType(type)
		{ }

	private:
		Type mType;
	};

	class Int32Data RTE_FINAL : public NodeContentData
	{
	public:
		int32_t Value;

		Int32Data() : NodeContentData(NodeContentData::Type::Int32) { }

		int calcSize();
		uint8_t* serialize(uint8_t* buffer);
		uint8_t* deserialize(uint8_t* buffer);
	};

}// namespace rte
