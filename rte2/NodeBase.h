#pragma once
#include "common.h"
#include "NodeDictionary.h"
#include <string>
#include <map>
#include <cassert>

namespace rte
	{

	class Context;

	class NodeBase : noncopyable, nonmovable
	{
	public:
		NodeBase(const std::string& name, NodeBase* pParent);
		virtual ~NodeBase() = default;

		const std::string& getName() { return mName; }
		const std::string& getName() const { return mName; }

		const std::string& getPath() { return mPath; }
		const std::string& getPath() const { return mPath; }

		NodeBase* getParent() { return mpParent; }
		const NodeBase* getParent() const { return mpParent; }

		void setParent(NodeBase* pParent);

	protected:
		void setPath(const std::string& path, Context* pContext);

	private:
		void updatePath();

	private:
		std::string mName;
		std::string mPath;
		NodeBase* mpParent;
	};

}// namespace rte
