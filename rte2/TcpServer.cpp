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

	bool TcpServer::configure(const TcpServerConfig& config)
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

		mIsConnectionClosed = false;
		mAcceptThread.start(std::bind(&TcpServer::acceptThread_, this, std::placeholders::_1));
		mSendThread.start(std::bind(&TcpServer::sendThread_, this, std::placeholders::_1));

		return true;
	}

	void TcpServer::close()
	{
		assert(mpSocket != nullptr);

		// �S���܂Ƃ߂Ď~�߂�
		mIsConnectionClosed = true;

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
		mem::safeDelete(&mpSocket);
	}

	void TcpServer::sendAsync(int id, const uint8_t* buffer, int bufferSize)
	{
		assert(mpSocket != nullptr);

		UniqueLock lock(mSendDataLock);

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

		// �w��N���C�A���g�̃X���b�h�ɒ�~���N�G�X�g�𔭍s
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

	unsigned int TcpServer::acceptThread_(void*)
	{
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
				// �N���C�A���g��t�R�[���o�b�N
				if (mConfig.onAcceptClient != nullptr)
				{
					auto clientId = socketToId(pClient);
					mConfig.onAcceptClient(clientId);
				}

				// �f�[�^��M�X���b�h�N��
				auto pReceiveThread = new Thread();
				pReceiveThread->start(std::bind(&TcpServer::receiveThread_, this, std::placeholders::_1), pClient);

				ClientInfo info;
				info.pReceiveThread = pReceiveThread;
				info.mpLock = new CriticalSection();
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
		unsigned int result = 0;

		auto pClientSocket = static_cast<Socket*>(arg);
		auto clientId = socketToId(pClientSocket);
		CriticalSection& clientLock = *mClientDic[pClientSocket].mpLock;

		const int bufferSize = 1024;
		mem::SafeArray<uint8_t> buffer(bufferSize);

		while (true)
		{
			// ��~���N�G�X�g���`�F�b�N
			{
				UniqueLock lock(mCloseRequestLock);

				auto closeRequest = std::find(mCloseRequestList.begin(), mCloseRequestList.end(), clientId);
				if (mIsConnectionClosed || closeRequest != mCloseRequestList.end())
				{
					mCloseRequestList.erase(closeRequest);
					break;
				}
			}

			// �f�[�^��M
			mem::SafeArray<uint8_t> receivedData;
			{
				UniqueLock lock(clientLock);
				receivedData.shallowCopyFrom(socketUtil::receive(pClientSocket));
			}

			if (receivedData.size() > 0)
			{
				// ��M�R�[���o�b�N
				if (mConfig.onReceiveData != nullptr)
				{
					mConfig.onReceiveData(clientId, buffer.get(), buffer.size());
				}
			}
			else if (receivedData.size() == 0)
			{
				// �������Ȃ�
			}
			else
			{
				// �G���[
				logError("receiving from client failed");
				if (mConfig.onConnectionError != nullptr)
				{
					if (!mConfig.onConnectionError(clientId, nullptr, 0))
					{
						result = 1;
						break;
					}
				}
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
			// ��~���N�G�X�g���`�F�b�N
			if (mIsConnectionClosed)
			{
				break;
			}

			if (mSendDataList.size() > 0)
			{
				// �L���[����ɂȂ�܂ő��M
				std::vector<SendData> dataList;
				{
					UniqueLock lock(mSendDataLock);
					mSendDataList.swap(dataList);
				}

				for (auto data : dataList)
				{
					auto pClientSocket = data.pClientSocket;
					CriticalSection& clientLock = *mClientDic[pClientSocket].mpLock;

					int sendBytes;
					{
						UniqueLock lock(clientLock);
						sendBytes = pClientSocket->send(data.buffer, data.bufferSize);
					}

					auto clientId = socketToId(pClientSocket);
					if (sendBytes == data.bufferSize)
					{
						// ���M�R�[���o�b�N
						if (mConfig.onSendData != nullptr)
						{
							mConfig.onSendData(clientId, data.buffer, data.bufferSize, sendBytes);
						}
					}
					else
					{
						// �G���[
						logError("sending to client failed");
						if (mConfig.onConnectionError != nullptr)
						{
							if (!mConfig.onConnectionError(clientId, data.buffer, data.bufferSize))
							{
								result = 1;
								break;
							}
						}
					}
				}
			}

			Sleep(cThreadPollingInterval);
		}

		return result;
	}

}// namespace rte
