#pragma once

#include "common.h"
#include <vector>
#include <string>
#include <memory>

namespace rte
{

	enum class CommandType : uint16_t
	{
		Invalid = 0,

		Ping,
		NodeNameList,
		Node,
		ParamUpdate,
	};

	class Context;

	/*------------------------------------------------------------------*/

	class Command
	{
	public:
		Command() = delete;
		Command(Command&& other) { moveFrom(std::move(other)); }
		virtual ~Command() = default;
		virtual CommandType getType() { return mType; }

		virtual uint8_t* serialize(uint8_t* buffer, Context* pContext) = 0;
		virtual uint8_t* deserialize(uint8_t* buffer, Context* pContext) = 0;

	protected:
		explicit Command(CommandType type)
			: mType(type)
		{ }
		virtual void moveFrom(Command&& other) = 0;
		CommandType mType;
	};

	/*------------------------------------------------------------------*/

	class PingCommand RTE_FINAL : public Command
	{
	public:
		PingCommand();
		uint8_t* serialize(uint8_t* buffer, Context* pContext);
		uint8_t* deserialize(uint8_t* buffer, Context* pContext);
	};

	/*------------------------------------------------------------------*/

	class Node;

	class NodeNameListCommand RTE_FINAL : public Command
	{
	public:
		/*! @breaf デシリアライズされたパスリストからノードツリーを構成する
		 *         (*pOut)[0] がルートで、そこから getChildren() で子供をたどる
		 */ 
#ifdef _SWIG_PY
		// SWIG は unique_ptr をサポートしてない
		static std::vector<Node*> createNodeTree(const std::vector<std::string>& pathList);
#else
		static std::vector<std::unique_ptr<Node>> createNodeTree(const std::vector<std::string>& pathList);
#endif

	public:
		NodeNameListCommand();
		uint8_t* serialize(uint8_t* buffer, Context* pContext);
		uint8_t* deserialize(uint8_t* buffer, Context* pContext);

		void add(const Node* pNode);
		const std::vector<std::string>& getNodePathList() const { return mPathList; }

	private:
		std::vector<std::string> mPathList;
	};

	/*------------------------------------------------------------------*/

	class Node;

	class NodeCommand RTE_FINAL : public Command
	{
	public:
		NodeCommand();
		~NodeCommand();
		uint8_t* serialize(uint8_t* buffer, Context* pContext);
		uint8_t* deserialize(uint8_t* buffer, Context* pContext);

		Node* get();
		void set(Node* pNode);

	private:
		Node* mpNode;
		bool mOwnNode;
	};

	/*------------------------------------------------------------------*/

	class NodeParameter;

	class ParamUpdateCommand RTE_FINAL : public Command
	{
	public:
		ParamUpdateCommand();
		~ParamUpdateCommand();
		uint8_t* serialize(uint8_t* buffer, Context* pContext);
		uint8_t* deserialize(uint8_t* buffer, Context* pContext);

		void add(NodeParameter* pNodeParam);
		const std::vector<NodeParameter*>& getParameterList();

	private:
		std::vector<NodeParameter*> mParamPtrList;
		bool mOwnParam;
	};

}// namespace rte
