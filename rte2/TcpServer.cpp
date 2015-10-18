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
}

namespace rte {

	TcpServer::TcpServer()
		: mpSocket(nullptr)
	{}

	TcpServer::~TcpServer()
	{
		assert(!mAcceptThread.isAlive());
		assert(mClientThreadDic.size() == 0);

		mem::safeDelete(&mpSocket);
	}

	bool TcpServer::configure(const Config& config)
	{
		assert(mpSocket == nullptr);

		mConfig = config;
		mNextPort = config.portBegin;

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

		// TODO: 排他処理

		// クライアント受付スレッド起動
		mAcceptThread.start([this](void*)
		{
			auto pClient = new Socket();
			while (true)
			{
				auto result = mpSocket->accept(pClient);
				if (result == TriBool::True)
				{
					// クライアント受付コールバック
					auto clientId = socketToId(pClient);
					mConfig.onAcceptClient(clientId);

					// データ受信スレッド起動
					auto pClientThraed = new Thread();
					pClientThraed->start([this, pClient, clientId](void*)
					{
						const int bufferSize = 1024;
						mem::SafeArray<uint8_t> buffer(bufferSize);

						while (true)
						{
							// 停止リクエストをチェック
							auto closeRequest = std::find(mCloseRequestList.begin(), mCloseRequestList.end(), clientId);
							if (closeRequest != mCloseRequestList.end())
							{
								mCloseRequestList.erase(closeRequest);
								break;
							}

							// データ受信
							auto recvBytes = pClient->recv(buffer.get(), buffer.size());
							if (recvBytes > 0)
							{
								buffer.resize(recvBytes);

								// キューが空になるまで受信
								mem::SafeArray<uint8_t> tmp(bufferSize);
								while (pClient->getAvailabieSize() > 0)
								{
									recvBytes = pClient->recv(tmp.get(), tmp.size());
									if (recvBytes > 0)
									{
										tmp.resize(recvBytes);
										buffer.append(tmp.get(), tmp.size());
									}
								}

								// 受信コールバック
								mConfig.onReceiveData(clientId, buffer.get(), buffer.size());
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

							Sleep(1);// スレッド切り替えタイミング
						}
					});

					mClientThreadDic[pClient] = pClientThraed;
					pClient = new Socket();
				}

				Sleep(1);// スレッド切り替えタイミング
			}

			mem::safeDelete(&pClient);
		});

		return true;
	}

	void TcpServer::close()
	{
		assert(mpSocket != nullptr);

		mAcceptThread.join();

		for (std::pair<Socket*, Thread*> item : mClientThreadDic)
		{
			mem::safeDelete(&item.first);
			item.second->join();
			mem::safeDelete(&item.second);
		}
		mClientThreadDic.clear();

		mpSocket->close();
	}

	TcpServer::SendResult TcpServer::send(int id, const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		TcpServer::SendResult result;
		auto pSocket = idToSocket(id);
		result.id = id;
		result.sendBytes = pSocket->send(buffer, bufferSize);

		return result;
	}

	std::vector<TcpServer::SendResult> TcpServer::broadcast(const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		std::vector<SendResult> result;
		for (auto client : mClientThreadDic)
		{
			auto id = socketToId(client.first);
			result.push_back(send(id, buffer, bufferSize));
		}
		return std::move(result);
	}

	void TcpServer::closeConnection(int id)
	{
		assert(mpSocket != nullptr);

		auto ptr = idToSocket(id);
		auto pClient = mClientThreadDic.find(ptr);
		if (pClient == mClientThreadDic.end())
		{
			return;
		}

		auto pClientSocket = pClient->first;
		auto pClientThread = pClient->second;

		mCloseRequestList.push_back(socketToId(pClientSocket));
		pClientThread->join();
		mem::safeDelete(&pClientThread);

		pClientSocket->close();
		mem::safeDelete(&pClientSocket);

		mClientThreadDic.erase(pClient);
	}

}// namespace rte
