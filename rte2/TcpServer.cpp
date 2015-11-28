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
		: mpSocket(nullptr), mKeepAliveIntervalSeconds(60)
	{ }

	TcpServer::~TcpServer()
	{
		assert(mIsConnectionClosed);
		assert(mConnectionDic.size() == 0);
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

		return true;
	}

	void TcpServer::close()
	{
		assert(mpSocket != nullptr);

		mIsConnectionClosed = true;

		mAcceptThread.join();

		// closeConnection()の中でmConnectionDicを変更するのでコピーしておく
		auto tmp = mConnectionDic;
		for (auto conn : tmp)
		{
			auto pClient = conn.first;
			auto clientId = socketToId(pClient);
			closeConnection(clientId);
		}

		mpSocket->close();
		mem::safeDelete(&mpSocket);
	}

	bool TcpServer::cleanupInvalidConnection()
	{
		auto cleaned = (mCleanupRequestList.size() > 0);

		mCleanupRequestList.lock();
		{
			for (auto pClient : mCleanupRequestList)
			{
				auto clientId = socketToId(pClient);
				closeConnection(clientId);
			}
			mCleanupRequestList.clear();
		}
		mCleanupRequestList.unlock();

		return cleaned;
	}

	void TcpServer::sendAsync(int id, const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		TcpSentData data;
		data.clientId = id;
		data.buffer = const_cast<uint8_t*>(buffer);
		data.bufferSize = bufferSize;
		mSendRequestList.emplaceBack(std::move(data));
	}

	void TcpServer::broadcastAsync(const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		for (auto conn : mConnectionDic)
		{
			auto id = socketToId(conn.first);
			sendAsync(id, buffer, bufferSize);
		}
	}

	std::vector<int> TcpServer::getClientList()
	{
		std::vector<int> result;
		for (auto conn : mConnectionDic)
		{
			auto id = socketToId(conn.first);
			result.push_back(id);
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

	void TcpServer::closeConnection(int clientId)
	{
		assert(mpSocket != nullptr);
		logInfo("close: " + std::to_string(clientId));

		auto pClient = idToSocket(clientId);
		auto found = mConnectionDic.find(pClient);
		if (found == mConnectionDic.end())
		{
			return;
		}

		auto pConn = found->second;
		pConn->join();
		mem::safeDelete(&pConn);

		pClient->close();
		mem::safeDelete(&pClient);

		mConnectionDic.erase(found);
		mClosedList.pushBack(clientId);
	}

	int TcpServer::acceptThread_(void*)
	{
		logInfo("enter");

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

				auto clientId = socketToId(pClient);
				mAcceptedList.pushBack(clientId);

				// 送受信スレッド起動
				auto pConn = new Thread();
				pConn->start(std::bind(&TcpServer::connectionThread_, this, std::placeholders::_1), pClient);

				assert(mConnectionDic.find(pClient) == mConnectionDic.end());
				mConnectionDic[pClient] = pConn;

				pClient = new Socket();
			}

			Sleep(cThreadPollingInterval);
		}

		mem::safeDelete(&pClient);

		logInfo("leave");

		return 0;
	}

	int TcpServer::connectionThread_(void* arg)
	{
		logInfo("enter");

		auto pClient = static_cast<Socket*>(arg);
		auto lastCheckKeepAlive = std::chrono::system_clock::now();

		while (true)
		{
			// 停止チェック
			if (mIsConnectionClosed)
			{
				return 0;
			}

			// 受信
			if (!receiveData_(pClient))
			{
				break;
			}

			// 送信
			if (!sendData_(pClient))
			{
				break;
			}

			// keep-alive
			if (!checkKeepAlive_(pClient, &lastCheckKeepAlive))
			{
				break;
			}

			Sleep(cThreadPollingInterval);
		}

		mCleanupRequestList.pushBack(pClient);

		logInfo("leave");
		return -1;
	}

	bool TcpServer::sendData_(Socket* pClient)
	{
		if (mSendRequestList.size() == 0)
		{
			return true;
		}

		std::vector<TcpSentData> dataList;
		mSendRequestList.swap(&dataList);

		auto success = true;

		for (auto data : dataList)
		{
			if (sendDataToSocket(pClient, &data))
			{
				mSentList.emplaceBack(std::move(data));
				success &= true;
			}
			else
			{
				// 送信失敗
				logInfo("failed to send data: " + data.toString());
				success = false;
			}
		}

		return success;
	}

	bool TcpServer::receiveData_(Socket* pClient)
	{
		TcpReceivedData result;
		if (receiveDataFromSocket(&result, pClient))
		{
			result.clientId = socketToId(pClient);
			mReceivedList.emplaceBack(std::move(result));
			return true;
		}
		return false;
	}

	bool TcpServer::checkKeepAlive_(Socket* pClient, std::chrono::system_clock::time_point* pLastCheckKeepAlive)
	{
		auto now = std::chrono::system_clock::now();
		if (now - *pLastCheckKeepAlive < std::chrono::seconds(mKeepAliveIntervalSeconds))
		{
			return true;
		}

		auto clientId = socketToId(pClient);

		if (socketUtil::sendKeepAlive(pClient))
		{
			*pLastCheckKeepAlive = std::chrono::system_clock::now();
			logInfo("check keep-alive: " + std::to_string(clientId));
			return true;
		}

		logInfo("client does not keep alive: " + std::to_string(clientId));

		return false;
	}

#if false
	int TcpServer::receiveThread_(void* arg)
	{
		logInfo("enter");

		auto pClientSocket = static_cast<Socket*>(arg);
		auto clientId = socketToId(pClientSocket);
		CriticalSection& clientLock = *mClientDic[pClientSocket].pLock;

		mem::Array<uint8_t> tmpReceivedData;

		while (true)
		{
			logInfo(std::to_string(mCloseRequestList.size()));

			// 停止リクエストをチェック
			if (mCloseRequestList.erase(clientId) || mIsConnectionClosed)
			{
				break;
			}

			// データ受信
			auto receivedSize = 0;
			{
				logInfo("a");
				UniqueLock lock(clientLock);
				logInfo("b");
				receivedSize = socketUtil::receive(&tmpReceivedData, pClientSocket);

				if (receivedSize > 0)
				{
					// 受信通知を送信
					if (!socketUtil::sendReceivedConfirmation(pClientSocket))
					{
						logError("sending receive-confirmation failed");
						receivedSize = -1;
					}
				}
			}


			if (receivedSize > 0)
			{
				// 受信データをキューに詰める
				TcpReceivedData result;
				result.clientId = clientId;
				result.buffer = tmpReceivedData.get();
				result.bufferSize = tmpReceivedData.size();
				mReceivedList.emplaceBack(std::move(result));
			}
			else if (receivedSize == 0)
			{
				// 何もしない
			}
			else
			{
				// エラー
				socketUtil::handleWsaError(__FUNCTION__);
				closeConnection(clientId);
				break;
			}

			Sleep(cThreadPollingInterval);
		}

		logInfo("leave");

		return 0;
	}

	int TcpServer::sendThread_(void*)
	{
		logInfo("enter");

		while (true)
		{
			// 停止リクエストをチェック
			if (mIsConnectionClosed)
			{
				break;
			}

			if (mSendRequestList.size() > 0)
			{
				// キューが空になるまで送信
				std::vector<TcpSentData> dataList;
				mSendRequestList.swap(&dataList);

				std::vector<int> sendFailedSockets;

				for (auto data : dataList)
				{
					auto failed = std::find(sendFailedSockets.begin(), sendFailedSockets.end(), data.clientId);
					if (failed != sendFailedSockets.end())
					{
						continue;
					}

					auto pClientSocket = idToSocket(data.clientId);
					CriticalSection& clientLock = *mClientDic[pClientSocket].pLock;

					int sentSize;
					{
						UniqueLock lock(clientLock);
						sentSize = pClientSocket->send(data.buffer, data.bufferSize);

						if (sentSize == data.bufferSize)
						{
							// 受信通知のチェック
							if (!socketUtil::receiveReceivedConfirmation(pClientSocket))
							{
								logError("receiving receive-confirmation failed");
								sentSize = 0;
							}
						}
					}

					if (sentSize == data.bufferSize)
					{
						TcpSentData result;
						result.clientId = socketToId(pClientSocket);
						result.buffer = const_cast<uint8_t*>(data.buffer);
						result.bufferSize = data.bufferSize;
						result.sentSize = sentSize;
						mSentList.emplaceBack(std::move(result));
					}
					else
					{
						// エラー
						logError("sending to client failed");
						closeConnection(data.clientId);
						sendFailedSockets.push_back(data.clientId);
					}
				}
			}

			Sleep(cThreadPollingInterval);
		}

		logInfo("leave");

		return 0;
	}

	int TcpServer::keepAliveThread_(void*)
	{
		logInfo("enter");

		while (true)
		{
			// 停止リクエストをチェック
			if (mIsConnectionClosed)
			{
				break;
			}

			// keep-alive
			for (auto client : mClientDic)
			{
				auto pClientSocket = client.first;
				auto clientId = socketToId(pClientSocket);
				auto& clientLock = *client.second.pLock;

				logInfo("keep-alive: " + std::to_string(clientId));
				
				auto keepAlive = true;
				{
					UniqueLock lock(clientLock);
					keepAlive = socketUtil::sendKeepAlive(pClientSocket);
				}

				if (!keepAlive)
				{
					logInfo("connection does not keep alive: " + std::to_string(clientId));
					pushCloseRequest_(nullptr, clientId);
				}
				else
				{
					logInfo("check keep-alive: " + std::to_string(clientId));
				}
			}

			Sleep(mKeepAliveIntervalSeconds * 1000);
		}

		logInfo("leave");

		return 0;
	}

	bool TcpServer::pushCloseRequest_(TcpServer::ClientInfo* pOut, int clientId)
	{
		auto ptr = idToSocket(clientId);

		auto found = mClientDic.find(ptr);
		if (found == mClientDic.end())
		{
			return false;
		}

		auto clientInfo = *found;
		mClientDic.erase(found);

		if (pOut != nullptr)
		{
			*pOut = found->second;
		}
		mCloseRequestList.pushBack(clientId);

		return true;
	}
#endif

}// namespace rte
