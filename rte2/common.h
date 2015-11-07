#pragma once
#include <string>
#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdint>
#include <cassert>

#include <WinSock2.h> // Windows.h より先に include しないとバグる
#include <Windows.h>

#include <io.h>
#include <sys/stat.h>

// swigだとfinal使えない。。
#ifdef _SWIG_PY
#	define RTE_FINAL
#else
#	define RTE_FINAL final
#endif

// http://www.swig.org/Doc3.0/CPlusPlus11.html#CPlusPlus11_rvalue_reference_and_move_semantics
#ifdef _SWIG_PY
#	define RTE_RVAL_DECL(type, x) type& x
#	define RTE_MOVE(x) x
#else
#	define RTE_RVAL_DECL(type, x) type&& x
#	define RTE_MOVE(x) std::move(x)
#endif

// swigはC++11対応が微妙。。
#if __cplusplus < 201103L
// C++11
#else
// C++98/03
#endif


#define logInfo(msg) rte::log::info_(__FUNCTION__, msg)
#define logWarn(msg) rte::log::warn_(__FUNCTION__, msg)
#define logError(msg) rte::log::error_(__FUNCTION__, msg)

namespace rte {

#ifdef _SWIG_PY
	inline uint8_t* makeArray(std::string buffer)
	{
		auto result = new uint8_t[buffer.length()];
		memcpy(result, buffer.c_str(), buffer.length());
		return result;
	}
#endif

	enum class TriBool : uint8_t
	{
		False = 0,
		True,
		Unknown,
	};

	class noncopyable
	{
	public:
		noncopyable() = default;
		noncopyable(const noncopyable&) = delete;
		noncopyable& operator=(const noncopyable&) = delete;
	};

	class nonmovable
	{
	public:
		nonmovable() = default;
		nonmovable(nonmovable&&) = delete;
	};

	namespace log {
		inline void info_(const char* function, const std::string& msg)
		{
			std::cout << "[" << function << "] " << msg << std::endl;
		}

		inline void warn_(const char* function, const std::string& msg)
		{
			std::cout << "[WARNING][" << function << "] " << msg << std::endl;
		}

		inline void error_(const char* function, const std::string& msg)
		{
			std::cerr << "[ERROR][" << function << "] " << msg << std::endl;
		}

		inline std::string getLastErrorString(int err)
		{
			LPVOID lpMsg;
			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				nullptr,// lpSource
				err,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
				(LPSTR)&lpMsg,
				0, nullptr);
			auto result = std::string(static_cast<char*>(lpMsg));
			LocalFree(lpMsg);
			return result;
		}
	}

	/*------------------------------------------------------------------*/
	// メモリ・オブジェクト関連
	namespace mem {
		template<class T>
		inline void safeDelete(T** ppObj)
		{
			if (*ppObj != nullptr)
			{
				delete *ppObj;
				*ppObj = nullptr;
			}
		}

		template<class T>
		inline void safeDeleteArray(T** ppObj)
		{
			if (*ppObj != nullptr)
			{
				delete[] *ppObj;
				*ppObj = nullptr;
			}
		}

		template<class T>
		class Array
		{
		public:
			Array()
				: mPtr(nullptr), mSize(0)
			{ }

			explicit Array(int size)
				: mPtr(nullptr), mSize(0)
			{
				allocate(size);
			}

			Array(Array&& other)
			{
				*this = std::move(other);
			}

			Array& operator=(Array&& other)
			{
				mPtr = other.mPtr;
				mSize = other.mSize;

				other.mPtr = nullptr;
				other.mSize = 0;

				return *this;
			}

			Array(Array&) = delete;
			Array& operator=(Array&) = delete;
			~Array() = default;

			T* get() { return mPtr; }
			int size() { return mSize; }

			void allocate(int size)
			{
				deallocate();
				if (size > 0)
				{
					mPtr = new T[size];
					printf("alloc %p\n", mPtr);
					mSize = size;
				}
			}

			void deallocate()
			{
				mem::safeDelete(&mPtr);
				printf("deallocate %p\n", mPtr);
				mSize = 0;
			}

			void invalidate()
			{
				deallocate();
				mSize = -1;
			}

			void resize(int size)
			{
				assert(size >= 0);
				if (size == 0)
				{
					deallocate();
					return;
				}

				auto ptr = new T[size];
				if (mPtr != nullptr)
				{
					memcpy(ptr, mPtr, size);
				}
				
				deallocate();
				mPtr = ptr;
				mSize = size;
			}

			void append(Array&& other)
			{
				if (other.mSize == 0)
				{
					return;
				}

				auto ptr = new T[mSize + other.mSize];
				memcpy(ptr, mPtr, mSize);
				memcpy(ptr + mSize, other.mPtr, other.mSize);

				mPtr = ptr;
				printf("append %p\n", mPtr);
				mSize += other.mSize;

				other.deallocate();
			}

		private:
			T* mPtr;
			int mSize;
		};

		template<class T>
		class SafeArray
		{
		public:
			SafeArray()
				: mPtr(nullptr), mSize(0), mOwnPtr(false)
			{ }

			explicit SafeArray(int size)
				: mPtr(nullptr), mSize(size)
			{
				if (size > 0)
				{
					mPtr = new T[size];
					mOwnPtr = true;
				}
			}

			SafeArray(SafeArray<T>&& other)
			{
				mPtr = other.mPtr;
				mSize = other.mSize;
				mOwnPtr = true;
				other.mPtr = nullptr;
				other.mSize = 0;
			}

			~SafeArray()
			{
				if (mOwnPtr)
				{
					mem::safeDeleteArray(&mPtr);
				}
				mSize = 0;
			}

			// http://stackoverflow.com/questions/8102244/swig-c-to-python-warning362-operator-ignored
			//SafeArray<T>& operator=(const SafeArray<T>& other)
			//{
			//	mem::safeDelete(&mPtr);
			//	mPtr = other.mPtr;
			//	mSize = other.mSize;
			//	return *this;
			//}
			SafeArray(const SafeArray<T>&) = delete;
			SafeArray<T>& operator=(const SafeArray<T>&) = delete;

			void moveFrom(SafeArray<T>&& other)
			{
				mPtr = other.mPtr;
				mSize = other.mSize;

				other.mPtr = nullptr;
				other.mSize = 0;
			}

			T* get() { return mPtr; }
			int size() { return mSize; }

			void swap(SafeArray<T>& other)
			{
				std::swap(mPtr, other.mPtr);
				std::swap(mSize, other.mSize);
				std::swap(mOwnPtr, other.mOwnPtr);
			}

			void resize(int size)
			{
				if (size == mSize)
				{
					return;
				}

				if (size < 0)
				{
					mem::safeDeleteArray(&mPtr);
					mSize = size;
					return;
				}

				auto ptr = new T[size];
				memcpy(ptr, mPtr, size);

				mem::safeDeleteArray(&mPtr);
				mPtr = ptr;
				mSize = size;
				mOwnPtr = true;
			}

			void append(T* ptr, int size)
			{
				auto combined = new T[mSize + size];
				memcpy(combined, mPtr, mSize);
				memcpy(combined + mSize, ptr, size);

				mem::safeDeleteArray(&mPtr);
				mPtr = combined;
				mSize += size;
			}

			void append(SafeArray&& other)
			{
				append(other.get(), other.size());
				other.~SafeArray();
			}

		private:
			T* mPtr;
			int mSize;
			bool mOwnPtr;
		};
	}

	/*------------------------------------------------------------------*/
	// 文字列関連
	namespace string {
		inline std::string replace(const std::string& str, const std::string& oldValue, const std::string& newValue)
		{
			if (oldValue.length() == 0)
			{
				return str;
			}

			std::string result = str;

			std::string::size_type current = 0;
			while (true)
			{
				std::string::size_type next = result.find_first_of(oldValue, current);
				if (next == std::string::npos)
				{
					break;
				}
				result.replace(next, oldValue.length(), newValue);
				current = next + oldValue.length() + 1;
			}

			return result;
		}

		inline int find(const char* str, char target)
		{
			const char* begin = str;
			const char* end = str + strlen(str);
			const char* found = std::find(begin, end, target);
			return (*found != *end) ? std::distance(begin, found) : -1;
		}

		inline std::string trim(const std::string& str, const char* trimChars = " \t\r\n")
		{
			const char* cstr = str.c_str();
			const int length = static_cast<int>(str.length());

			int trimBegin = 0;
			while (trimBegin < length)
			{
				if (find(trimChars, cstr[trimBegin]) < 0) break;
				++trimBegin;
			}

			int trimEnd = 0;
			while (trimEnd < length)
			{
				if (find(trimChars, cstr[length - trimEnd - 1]) < 0) break;
				++trimEnd;
			}

			return std::move(str.substr(trimBegin, str.length() - (trimBegin + trimEnd)));
		}

		inline std::string trim1(const std::string& str, char trimChar = ' ')
		{
			const int trimBegin = (*str.begin() == trimChar) ? 1 : 0;
			const int trimEnd = (str.length() > 1 && *str.rbegin() == trimChar) ? 1 : 0;
			return std::move(str.substr(trimBegin, str.length() - (trimBegin + trimEnd)));
		}

		inline void split(std::vector<std::string>* pOut, const std::string& str, char delimiter)
		{
			pOut->clear();

			std::string::size_type current = 0;
			while (true)
			{
				std::string::size_type next = str.find_first_of(delimiter, current);
				if (next == std::string::npos)
				{
					break;
				}
				pOut->emplace_back(str.substr(current, next));
				current = next + 1;
			}
			pOut->emplace_back(str.substr(current));
		}

		inline void join(std::string* pOut, const std::vector<std::string> strList, char delimiter)
		{
			pOut->clear();

			for (auto str : strList)
			{
				pOut->append(str + delimiter);
			}

			if (strList.size() > 0)
			{
				pOut->erase(pOut->begin() + pOut->length() - 1);
			}
		}
	}// namespace string

	/*------------------------------------------------------------------*/
	// パス関連
	namespace path {
		inline std::string normalize(const std::string& path)
		{
			if (path.length() == 0)
			{
				return path;
			}

			std::string result = string::replace(path, "\\", "/");
			assert(result.length() != 0);
			if (*result.rbegin() == '/')
			{
				result = std::move(result.substr(0, result.length() - 1));
			}
			return std::move(result);
		}

		inline bool exista(const std::string& path)
		{
			return _access_s(path.c_str(), 0) == 0;
		}

		inline bool fileExists(const std::string& path)
		{
			struct _stat st;
			if (_stat(path.c_str(), &st) != 0)
			{
				return false;
			}
			return (st.st_mode & _S_IFREG) == _S_IFREG;
		}

		inline bool dirExists(const std::string& path)
		{
			struct _stat st;
			if (_stat(path.c_str(), &st) != 0)
			{
				return false;
			}
			return (st.st_mode & _S_IFDIR) == _S_IFDIR;
		}

		inline std::string basename(const std::string& path)
		{
			std::string::size_type index = path.find_last_of('/');
			if (index == std::string::npos)
			{
				index = path.find_last_of('\\');
			}

			if (index != std::string::npos)
			{
				return std::move(path.substr(index + 1));
			}
			return path;
		}

		inline std::string dirname(const std::string& path)
		{
			std::string::size_type index = path.find_last_of('/');
			if (index == std::string::npos)
			{
				index = path.find_last_of('\\');
			}
			assert(index != std::string::npos);
			return std::move(path.substr(0, index));
		}

		inline std::string combine(const std::string& lhs, const std::string& rhs)
		{
			if (lhs.length() == 0)
			{
				return rhs;
			}
			if (rhs.length() == 0)
			{
				assert(lhs.length() > 0);
				return (*lhs.rbegin() == '/') ? std::move(lhs.substr(0, rhs.length() - 1)) : lhs;
			}

			assert(lhs.length() > 0);
			assert(rhs.length() > 0);
			std::string l = (*lhs.rbegin() != '/') ? std::move(lhs + '/') : lhs;
			std::string r = (*rhs.begin() == '/') ? std::move(rhs.substr(1, rhs.length() - 1)) : rhs;
			return std::move(l + r);
		}
	}// namespace path

	/*------------------------------------------------------------------*/
	// WindowsAPI 関連
	namespace winapi
	{
		inline std::string formatError(DWORD error)
		{
			LPVOID msg;
			FormatMessageA(
				FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
				NULL,
				error,
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				reinterpret_cast<LPSTR>(&msg),
				0, NULL);

			std::string result(reinterpret_cast<const char*>(msg));

			LocalFree(msg);

			return std::move(string::trim(result));
		}

		inline std::string formatLastError()
		{
			return std::move(formatError(GetLastError()));
		}

	}// namespace win32

}// namespace rte
