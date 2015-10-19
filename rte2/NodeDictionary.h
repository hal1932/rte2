#pragma once
#include "common.h"
#include <string>
#include <map>
#include <memory>
#include <cstring>

namespace rte {

	class NodeBase;

	class NodeDictionary RTE_FINAL : noncopyable, nonmovable
	{
	public:
		NodeDictionary() = default;
		~NodeDictionary();

		void add(NodeBase* pNode);
		void remove(NodeBase* pNode);
		NodeBase* find(const std::string& path);

		bool empty();
		void clear(void (*deleter)(NodeBase**));

	private:
		struct StrCmp : public std::binary_function < const char*, const char*, bool >
		{
			bool operator()(const char* lhs, const char* rhs) const
			{
				return strcmp(lhs, rhs) < 0;
			}
		};
		std::map<const char*, NodeBase*, StrCmp> mDic;
	};

}// namespace rte
