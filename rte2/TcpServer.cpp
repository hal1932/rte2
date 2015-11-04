#include "TcpServer.h"
#include "Socket.h"
#include "socketUtil.h"
#include <algorithm>

namespace {

	int socketToId(rte::Socket* pSocket)
	{
		return reinterpret_cast<int>(pSocket);
	}

	rte::Socket* idToSocket(int id)
	{
		return reinterpret_cast<rte::Socket*>(id);
	}

	static const int cThreadPollingInterval = 10;
}

namespace rte {

	TcpServer::TcpServer()
		: mpSocket(nullptr)
	{ }

	TcpServer::~TcpServer()
	{
		assert(mIsConnectionClosed);
		assert(mClientDic.size() == 0);
		assert(mpSocket == nullptr);
	}

	bool TcpServer::open(int port)
	{
		assert(mpSocket == nullptr);

		mpSocket = new Socket();
		if (!mpSocket->configure(Socket::ProtocolType::Tcp))
		{
			mem::safeDelete(&mpSocket);
			return false;
		}

		mpSocket->setBlocking(true);

		if (!mpSocket->bind(port))
		{
			return false;
		}
		if (!mpSocket->listen())
		{
			return false;
		}

		mIsConnectionClosed = false;
		mAcceptThread.start(std::bind(&TcpServer::acceptThread_, this, std::placeholders::_1));
		mSendThread.start(std::bind(&TcpServer::sendThread_, this, std::placeholders::_1));

		return true;
	}

	void TcpServer::close()
	{
		assert(mpSocket != nullptr);

		// 全部まとめて止める
		mIsConnectionClosed = true;

		mAcceptThread.join();

		for (std::pair<Socket*, ClientInfo> item : mClientDic)
		{
			item.second.pReceiveThread->join();
			mem::safeDelete(&item.second.pReceiveThread);
			mem::safeDelete(&item.second.pLock);

			mem::safeDelete(&item.first);
		}
		mClientDic.clear();

		mpSocket->close();
		mem::safeDelete(&mpSocket);
	}

	void TcpServer::sendAsync(int id, const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		TcpSentData data;
		data.clientId = id;
		data.buffer = const_cast<uint8_t*>(buffer);
		data.bufferSize = bufferSize;
		mSentList.emplaceBack(std::move(data));
	}

	void TcpServer::broadcastAsync(const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		for (auto client : mClientDic)
		{
			auto id = socketToId(client.first);
			sendAsync(id, buffer, bufferSize);
		}
	}

	std::vector<int> TcpServer::getClientList()
	{
		std::vector<int> result;
		for (auto client : mClientDic)
		{
			result.push_back(socketToId(client.first));
		}
		return result;
	}

	std::vector<int> TcpServer::popAcceptedQueue()
	{
		std::vector<int> result;
		mAcceptedList.swap(&result);
		return std::move(result);
	}

	std::vector<TcpReceivedData> TcpServer::popReceivedQueue()
	{
		std::vector<TcpReceivedData> result;
		mReceivedList.swap(&result);
		return std::move(result);
	}

	std::vector<TcpSentData> TcpServer::popSentQueue()
	{
		std::vector<TcpSentData> result;
		mSentList.swap(&result);
		return std::move(result);
	}

	std::vector<int> TcpServer::popClosedQueue()
	{
		std::vector<int> result;
		mClosedList.swap(&result);

		return std::move(result);
	}

	void TcpServer::closeConnection(int id)
	{
		assert(mpSocket != nullptr);

		auto ptr = idToSocket(id);
		auto found = mClientDic.find(ptr);
		if (found == mClientDic.end())
		{
			return;
		}
		auto clientInfo = *found;
		mClientDic.erase(found);

		auto pClientSocket = clientInfo.first;
		auto pReceiveThread = clientInfo.second.pReceiveThread;

		// 指定クライアントのスレッドに停止リクエストを発行
		auto clientId = socketToId(pClientSocket);
		mCloseRequestList.pushBack(clientId);

		mClosedList.erase(clientId);

		pReceiveThread->join();
		mem::safeDelete(&pReceiveThread);
		mem::safeDelete(&clientInfo.second.pLock);

		pClientSocket->close();
		mem::safeDelete(&pClientSocket);
	}

	unsigned int TcpServer::acceptThread_(void*)
	{
		logInfo("enter");

		unsigned int result = 0;

		auto pClient = new Socket();
		while (true)
		{
			if (mIsConnectionClosed)
			{
				break;
			}

			auto accepted = mpSocket->accept(pClient);
			if (accepted == TriBool::True)
			{
				logInfo("accept");

				// クライアント記録
				{
					auto clientId = socketToId(pClient);
					mAcceptedList.pushBack(clientId);
				}

				// データ受信スレッド起動
				auto pReceiveThread = new Thread();
				pReceiveThread->start(std::bind(&TcpServer::receiveThread_, this, std::placeholders::_1), pClient);

				ClientInfo info;
				info.pReceiveThread = pReceiveThread;
				info.pLock = new CriticalSection();
				mClientDic[pClient] = info;

				pClient = new Socket();
			}

			Sleep(cThreadPollingInterval);
		}

		mem::safeDelete(&pClient);
		return result;
	}

	unsigned int TcpServer::receiveThread_(void* arg)
	{
		logInfo("");

		unsigned int result = 0;

		auto pClientSocket = static_cast<Socket*>(arg);
		auto clientId = socketToId(pClientSocket);
		CriticalSection& clientLock = *mClientDic[pClientSocket].pLock;

		const int bufferSize = 1024;

		while (true)
		{
			// 停止リクエストをチェック
			if (mCloseRequestList.erase(clientId) || mIsConnectionClosed)
			{
				break;
			}

			// データ受信
			mem::Array<uint8_t> receivedData;
			{
				UniqueLock lock(clientLock);
				receivedData = std::move(socketUtil::receive(pClientSocket));
			}

			if (receivedData.size() > 0)
			{
				// 受信データをキューに詰める
				TcpReceivedData result;
				result.clientId = clientId;
				result.buffer = receivedData.get();
				result.bufferSize = receivedData.size();

				mReceivedList.emplaceBack(std::move(result));
			}
			else if (receivedData.size() == 0)
			{
				// 何もしない
			}
			else
			{
				// エラー
				logError("receiving from client failed");

				// 不正データをキューに詰める
				TcpReceivedData result;
				result.clientId = clientId;
				result.buffer = nullptr;
				result.bufferSize = -1;

				mReceivedList.emplaceBack(std::move(result));
			}

			Sleep(cThreadPollingInterval);
		}

		return result;
	}

	unsigned int TcpServer::sendThread_(void*)
	{
		unsigned int result = 0;

		while (true)
		{
			// 停止リクエストをチェック
			if (mIsConnectionClosed)
			{
				break;
			}

			if (mSentList.size() > 0)
			{
				// キューが空になるまで送信
				std::vector<TcpSentData> dataList;
				mSentList.swap(&dataList);

				for (auto data : dataList)
				{
					auto pClientSocket = idToSocket(data.clientId);
					CriticalSection& clientLock = *mClientDic[pClientSocket].pLock;

					int sendBytes;
					{
						UniqueLock lock(clientLock);
						sendBytes = pClientSocket->send(data.buffer, data.bufferSize);
					}

					{
						auto clientId = socketToId(pClientSocket);

						TcpSentData result;
						result.clientId = clientId;
						result.buffer = const_cast<uint8_t*>(data.buffer);
						result.bufferSize = data.bufferSize;
						result.sentSize = sendBytes;

						mSentList.emplaceBack(std::move(result));
					}
				}
			}

			Sleep(cThreadPollingInterval);
		}

		return result;
	}

}// namespace rte
