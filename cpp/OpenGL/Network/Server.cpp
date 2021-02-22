#include <sstream>
#include <iostream>

#include "Server.h"

ClientConnection::ClientConnection(TCPSocket* connectionSocket, uint32_t id, const std::string& key, uint32_t timeout) :
  connectionSocket(connectionSocket),
  id(id),
  crypt(nullptr),
  sendCrypt(nullptr),
  key(key),
  timeout(timeout)
{
  sendThread = std::thread(&ClientConnection::sendFunc, this);
}

ClientConnection::~ClientConnection() {
  continueRunning = false;
  sendThread.join();

  try {
    if (connectionSocket && connectionSocket->IsConnected()) {
      connectionSocket->Close();
    }
    delete connectionSocket;
    connectionSocket = nullptr;
  } catch (SocketException const&  ) {
  }
}

bool ClientConnection::isConnected() {
  return connectionSocket && connectionSocket->IsConnected();
}

std::string ClientConnection::checkData() {
  int8_t data[2048];
  const uint32_t maxSize = std::min<uint32_t>(std::max<uint32_t>(4,messageLength),2048);
  const uint32_t bytes = connectionSocket->ReceiveData(data, maxSize, 1);
  if (bytes > 0) {
    return handleIncommingData(data, bytes);
  }
  return "";
}

void ClientConnection::sendRawMessage(const int8_t* rawData, uint32_t size) {
  uint32_t currentBytes = 0;
  uint32_t totalBytes = 0;

  try {
    do {
      currentBytes = connectionSocket->SendData(rawData + totalBytes, size-totalBytes, timeout);
      totalBytes += currentBytes;
    } while (currentBytes > 0 && totalBytes < size);

    if (currentBytes == 0 && totalBytes < size) {
      std::cerr << "lost data while trying to send " << size << " (actually send:" << totalBytes << ", timeout:" <<  timeout << ")" << std::endl;
    }
  } catch (SocketException const&  ) {
  }
}

std::vector<uint8_t> ClientConnection::intToVec(uint32_t i) const {
  std::vector<uint8_t> data(4);
  data[0] = i%256; i /= 256;
  data[1] = i%256; i /= 256;
  data[2] = i%256; i /= 256;
  data[3] = i;
  return data;
}


void ClientConnection::sendRawMessage(std::vector<int8_t> rawData) {
  uint32_t l = uint32_t(rawData.size());
  if (l != rawData.size()) {
    std::cerr << "lost data truncating long message" << std::endl;
  }
  
  std::vector<uint8_t> data = intToVec(l);

  sendRawMessage((int8_t*)data.data(), 4);
  sendRawMessage(rawData.data(), l);
}


void ClientConnection::sendRawMessage(std::string message) {
  const uint32_t l = uint32_t(message.length());
  if (l != message.length()) {
    std::cerr << "lost data truncating long message" << std::endl;
  }

  std::vector<uint8_t> data = intToVec(l);
  sendRawMessage((int8_t*)data.data(), 4);
  const int8_t* cStr = (int8_t*)(message.c_str());
  sendRawMessage(cStr, l);
}


void ClientConnection::sendMessage(std::string message) {
  if (!key.empty()) {
    if (sendCrypt) {
      message = sendCrypt->encryptString(message);
    } else {
      AESCrypt tempCrypt("1234567890123456",key);
      std::string iv = AESCrypt::genIVString();
      std::string initMessage = tempCrypt.encryptString(genHandshake(iv, key));
      sendCrypt = std::make_unique<AESCrypt>(iv,key);
      sendRawMessage(initMessage);
      message = sendCrypt->encryptString(message);
    }
  }
  sendRawMessage(message);
}

std::string ClientConnection::handleIncommingData(int8_t* data, uint32_t bytes) {
  recievedBytes.insert(recievedBytes.end(), data, data+bytes);

  if (messageLength == 0) {
    if (recievedBytes.size() >= 4) {
      messageLength = *((uint32_t*)recievedBytes.data());
    } else {
      return "";
    }
  }
  
  if (recievedBytes.size() >= messageLength+4) {
    std::ostringstream os;
    for (size_t i = 4;i<messageLength+4;++i) {
      os << recievedBytes[i];
    }
    recievedBytes.erase(recievedBytes.begin(), recievedBytes.begin() + messageLength+4);
    messageLength = 0;
    
    if (key.empty()) {
      return os.str();
    } else {
      if (crypt) {
        return crypt->decryptString(os.str());
      } else {
        AESCrypt tempCrypt("1234567890123456",key);
        const std::string firstMessage = tempCrypt.decryptString(os.str());
        const std::string iv = getIVFromHandshake(firstMessage, key);
        crypt = std::make_unique<AESCrypt>(iv,key);
        return "";
      }
    }
  }
  
  return "";
}

void ClientConnection::enqueueMessage(const std::string& m) {
  messageQueueLock.lock();
  messageQueue.push(m);
  messageQueueLock.unlock();
}

void ClientConnection::sendFunc() {
  while (continueRunning) {
    try {
      if (!messageQueue.empty()) {
        messageQueueLock.lock();
        std::string front = messageQueue.front();
        messageQueue.pop();
        messageQueueLock.unlock();
        sendMessage(front);
      } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    } catch (SocketException const& e) {
      std::cerr << "sendFunc SocketException: " << e.what() << std::endl;
      continue;
    } catch (AESException const& e) {
      std::cerr << "encryption error: " << e.what() << std::endl;
      continue;
    }
  }
}

Server::Server(short port, const std::string& key, uint32_t timeout) :
  port{port},
  timeout{timeout},
  key{key}
{
}

Server::~Server() {
  continueRunning = false;
  connectionThread.join();
  clientThread.join();
}


void Server::start() {
  connectionThread = std::thread(&Server::serverFunc, this);
  clientThread = std::thread(&Server::clientFunc, this);
}


void Server::shutdownServer() {
  starting = false;
  if (serverSocket) {
    try {
      serverSocket->Close();
    } catch (SocketException const&  ) {
    }
  }
  ok = false;
}

void Server::sendMessage(const std::string& message, uint32_t id, bool invertID) {
  clientVecMutex.lock();
  for (size_t i = 0;i<clientConnections.size();++i) {
    if (id == 0 ||
        (!invertID && clientConnections[i]->getID() == id) ||
        (invertID && clientConnections[i]->getID() != id)) {
      clientConnections[i]->enqueueMessage(message);
    }
  }
  clientVecMutex.unlock();
}

void Server::removeClient(size_t i) {
  const uint32_t cid = clientConnections[i]->getID();
  clientConnections.erase(clientConnections.begin() + i);
  handleClientDisconnection(cid);
}


void Server::clientFunc() {
  while (continueRunning) {
    clientVecMutex.lock();
    for (size_t i = 0;i<clientConnections.size();++i) {
      
      // remove clients that have disconnected
      if (!clientConnections[i]->isConnected()) {
        removeClient(i);
        continue;
      }
      
      try {
        std::string message = clientConnections[i]->checkData();
        if (!message.empty()) {
          clientVecMutex.unlock();
          handleClientMessage(clientConnections[i]->getID(), message);
          clientVecMutex.lock();
        }
      } catch (SocketException const& ) {
        removeClient(i);
        continue;
      } catch (AESException const& e) {
        std::cerr << "encryption error: " << e.what() << std::endl;
        removeClient(i);
        continue;
      }
      
      if (!continueRunning) break;
    }
    clientVecMutex.unlock();
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
  }
}

void Server::serverFunc() {
  // open server port
  while (continueRunning) {
    starting= true;
    try {
      serverSocket = std::make_shared<TCPServer>();
      serverSocket->SetNonBlocking(timeout == INFINITE_TIMEOUT ? false : true);
      serverSocket->SetNoDelay(true);
      serverSocket->SetReuseAddress(true);
      serverSocket->SetNoSigPipe(true);
      serverSocket->Bind(NetworkAddress(NetworkAddress::Any, port));
      serverSocket->Listen();
      serverSocket->GetLocalPort();
    } catch (SocketException const& e) {

      std::stringstream ss;
      ss << "SocketException: " << e.what() << " (" << e.where() << " returned with error code " << e.withErrorCode() << ")";
      std::cerr << ss.str() << std::endl;

      shutdownServer();
      return;
    }
    ok = true;
    starting = false;
    
    // accept connections
    while (continueRunning) {

      try {
        TCPSocket* connectionSocket{nullptr};
        while (connectionSocket == nullptr && continueRunning) {
          std::this_thread::sleep_for(std::chrono::milliseconds(1));
          while (!serverSocket->AcceptNewConnection((ConnectionSocket**)&connectionSocket, timeout)) {
            if (!continueRunning) {
              if (connectionSocket) {
                connectionSocket->Close();
                delete connectionSocket;
                connectionSocket = nullptr;
              }
              shutdownServer();
              return;
            }
          }
        }
        
        try {
          // check if peer already dropped the connection
          if (!connectionSocket->IsConnected()) {
            connectionSocket->Close();
            delete connectionSocket;
            connectionSocket = nullptr;
            continue;
          }
        } catch (SocketException const& ) {
          delete connectionSocket;
          connectionSocket = nullptr;
          continue;
        }

        clientVecMutex.lock();
        ++lastClientId;
        clientConnections.push_back(std::make_shared<ClientConnection>(connectionSocket, lastClientId, key, timeout));
        try {
          handleClientConnection(lastClientId);
        } catch (SocketException const& ) {
        }
        clientVecMutex.unlock();
      } catch (SocketException const& ) {
      }
      
    }
  }
    
  shutdownServer();
}

std::vector<uint32_t> Server::getValidIDs() {
  
  clientVecMutex.lock();
  std::vector<uint32_t> ids(clientConnections.size());
  for (size_t i = 0;i<clientConnections.size();++i) {
    ids[i] = uint32_t(clientConnections[i]->getID());
  }
  clientVecMutex.unlock();

  return ids;
}
