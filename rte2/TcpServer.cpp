#include "TcpServer.h"
#include "Socket.h"
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
	{}

	TcpServer::~TcpServer()
	{
		assert(!mAcceptThread.isAlive());
		assert(mClientDic.size() == 0);

		mem::safeDelete(&mpSocket);
	}

	bool TcpServer::configure(const Config& config)
	{
		assert(mpSocket == nullptr);

		mConfig = config;

		mpSocket = new Socket();
		if (!mpSocket->configure(Socket::ProtocolType::Tcp))
		{
			mem::safeDelete(&mpSocket);
			return false;
		}

		mpSocket->setBlocking(true);

		return true;
	}

	bool TcpServer::open(int port)
	{
		assert(mpSocket != nullptr);

		if (!mpSocket->bind(port))
		{
			return false;
		}
		if (!mpSocket->listen())
		{
			return false;
		}

		mIsClosed = false;
		mAcceptThread.start(std::bind(&TcpServer::acceptThread_, this, std::placeholders::_1));
		mSendThread.start(std::bind(&TcpServer::sendThread_, this, std::placeholders::_1));

		return true;
	}

	void TcpServer::close()
	{
		assert(mpSocket != nullptr);

		// 全部まとめて止める
		mIsClosed = true;

		mAcceptThread.join();

		for (std::pair<Socket*, ClientInfo> item : mClientDic)
		{
			item.second.pReceiveThread->join();
			mem::safeDelete(&item.second.pReceiveThread);
			mem::safeDelete(&item.second.mpLock);

			mem::safeDelete(&item.first);
		}
		mClientDic.clear();

		mpSocket->close();
	}

	void TcpServer::sendAsync(int id, const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		SendData data;
		data.pClientSocket = idToSocket(id);
		data.buffer = buffer;
		data.bufferSize = bufferSize;
		mSendDataList.emplace_back(data);
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
		{
			UniqueLock lock(mCloseRequestLock);
			mCloseRequestList.push_back(socketToId(pClientSocket));
		}

		pReceiveThread->join();
		mem::safeDelete(&pReceiveThread);
		mem::safeDelete(&clientInfo.second.mpLock);

		pClientSocket->close();
		mem::safeDelete(&pClientSocket);
	}

	void TcpServer::acceptThread_(void*)
	{
		auto pClient = new Socket();

		while (true)
		{
			if (mIsClosed)
			{
				break;
			}

			auto result = mpSocket->accept(pClient);
			if (result == TriBool::True)
			{
				// クライアント受付コールバック
				if (mConfig.onAcceptClient != nullptr)
				{
					auto clientId = socketToId(pClient);
					mConfig.onAcceptClient(clientId);
				}

				// データ受信スレッド起動
				auto pReceiveThread = new Thread();
				pReceiveThread->start(std::bind(&TcpServer::receiveThread_, this, std::placeholders::_1), pClient);

				ClientInfo info;
				info.pReceiveThread = pReceiveThread;
				info.mpLock = new Mutex();
				mClientDic[pClient] = info;

				pClient = new Socket();
			}

			Sleep(cThreadPollingInterval);
		}

		mem::safeDelete(&pClient);
	}

	void TcpServer::receiveThread_(void* arg)
	{
		auto pClientSocket = static_cast<Socket*>(arg);
		auto clientId = socketToId(pClientSocket);
		Mutex& clientLock = *mClientDic[pClientSocket].mpLock;

		const int bufferSize = 1024;
		mem::SafeArray<uint8_t> buffer(bufferSize);

		while (true)
		{
			// 停止リクエストをチェック
			{
				UniqueLock lock(mCloseRequestLock);

				auto closeRequest = std::find(mCloseRequestList.begin(), mCloseRequestList.end(), clientId);
				if (mIsClosed || closeRequest != mCloseRequestList.end())
				{
					mCloseRequestList.erase(closeRequest);
					break;
				}
			}

			// データ受信
			int recvBytes;
			{
				UniqueLock lock(clientLock);

				recvBytes = pClientSocket->recv(buffer.get(), buffer.size());
				if (recvBytes > 0)
				{
					buffer.resize(recvBytes);

					// キューが空になるまで受信
					mem::SafeArray<uint8_t> tmp(bufferSize);
					while (pClientSocket->getAvailabieSize() > 0)
					{
						recvBytes = pClientSocket->recv(tmp.get(), tmp.size());
						if (recvBytes > 0)
						{
							tmp.resize(recvBytes);
							buffer.append(tmp.get(), tmp.size());
						}
					}
				}
			}

			if (recvBytes > 0)
			{
				// 受信コールバック
				if (mConfig.onReceiveData != nullptr)
				{
					mConfig.onReceiveData(clientId, buffer.get(), buffer.size());
				}
			}
			else if (recvBytes == 0)
			{
				// 何もしない
			}
			else
			{
				// エラー
				logError("receiving from client failed");
				closeConnection(clientId);
				break;
			}

			Sleep(cThreadPollingInterval);
		}
	}

	void TcpServer::sendThread_(void*)
	{
		while (true)
		{
			// キューが空になるまで送信
			for (auto data : mSendDataList)
			{
				auto pClientSocket = data.pClientSocket;
				Mutex& clientLock = *mClientDic[pClientSocket].mpLock;

				int sendBytes;
				{
					UniqueLock lock(clientLock);
					sendBytes = pClientSocket->send(data.buffer, data.bufferSize);
				}

				// 送信コールバック
				if (mConfig.onSendData)
				{
					auto id = socketToId(pClientSocket);
					mConfig.onSendData(id, data.buffer, data.bufferSize, sendBytes);
				}
			}

			Sleep(cThreadPollingInterval);
		}
	}

}// namespace rte
