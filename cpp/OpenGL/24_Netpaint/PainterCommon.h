#pragma once

#include <string>
#include <NetCommon.h>

#include <Image.h>
#include <Vec2.h>
#include <Vec4.h>

constexpr uint32_t imageWidth = 400;
constexpr uint32_t imageHeight= 400;
constexpr uint16_t serverPort = 11002;

class ClientInfo {
public:
  uint32_t id{0};
  std::string name{""};
  Vec4 color{0,0,0,0};
  Vec2 pos{0,0};
  
  ClientInfo() {}
  
  ClientInfo(uint32_t id, const std::string& name, const Vec4& color, const Vec2 pos) :
    id{id},
    name{cleanupName(name)},
    color{color},
    pos{pos}
  {}

  virtual ~ClientInfo() {}
  
  static std::string cleanupName(const std::string& name) {
    std::string cName{name};
    for (size_t i = 0;i<name.length();++i) {
      cName[i] = cleanupChar(name[i]);
    }
    return cName;
  }

private:
  static char cleanupChar(char c) {
    std::string validChars{"01234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ(),._ ;:"};
    if (validChars.find(c) != std::string::npos) return c; else return '_';
  }

};

class ClientInfoServerSide : public ClientInfo {
public:
  bool fastCursorUpdates;
  
  ClientInfoServerSide(uint32_t id, const std::string& name, const Vec4& color,
                       const Vec2 pos, bool fastCursorUpdates) :
  ClientInfo(id, name, color, pos),
  fastCursorUpdates(fastCursorUpdates)
  {
  }

  virtual ~ClientInfoServerSide() {}
};

class ClientInfoClientSide : public ClientInfo {
public:
  Image image;
  
  ClientInfoClientSide(uint32_t id, const std::string& name, const Vec4& color,
                       const Vec2 pos, const Image& image) :
  ClientInfo(id, name, color, pos),
  image(image)
  {
  }
  
  virtual ~ClientInfoClientSide() {}
};

enum class MessageType {
  InvalidMessage = 0,
  BasicMessage = 1,
  MousePosMessage = 2,
  NewUserMessage = 3,
  LostUserMessage = 4,
  CanvasUpdateMessage = 5,
  InitMessage = 6,
  ConnectMessage = 7
};

MessageType identifyString(const std::string& s);

struct BasicMessage {
  MessageType pt;
  uint32_t userID{0};
  Tokenizer tokenizer;

  BasicMessage(const std::string& message) :
    tokenizer{message}
  {
    if (tokenizer.nextString() != "painter") throw MessageException("Invalid message");
    pt = MessageType(tokenizer.nextUint32());
    userID = tokenizer.nextUint32();
    pt = MessageType::BasicMessage;
  }
  
  BasicMessage() :
    tokenizer{""}
  {
    pt = MessageType::BasicMessage;
  }
  
  virtual ~BasicMessage() {}
  
  virtual std::string toString() {
    Encoder e;
    e.add("painter");
    e.add(uint32_t(pt));
    e.add(userID);
    return e.getEncodedMessage();
  }
  
};

struct MousePosMessage : public BasicMessage {
  Vec2 mousePos;

  MousePosMessage(const std::string& message) :
    BasicMessage(message)
  {
    float x = tokenizer.nextFloat();
    float y = tokenizer.nextFloat();
    mousePos = Vec2(x,y);
    pt = MessageType::MousePosMessage;
  }
  
  MousePosMessage(const Vec2& mousePos) :
    mousePos(mousePos)
  {
    pt = MessageType::MousePosMessage;
  }
  
  virtual ~MousePosMessage() {}

  virtual std::string toString() override {
    Encoder e;
    e.add(mousePos.x());
    e.add(mousePos.y());
    return BasicMessage::toString() + e.getEncodedMessage();
  }

};

struct NewUserMessage : public BasicMessage {
  std::string name;
  Vec4 color;
  
  NewUserMessage(const std::string& message) :
    BasicMessage(message)
  {
    name = tokenizer.nextString();
    float r = tokenizer.nextFloat();
    float g = tokenizer.nextFloat();
    float b = tokenizer.nextFloat();
    float a = tokenizer.nextFloat();
    color = Vec4(r,g,b,a);
    pt = MessageType::NewUserMessage;
  }
  
  NewUserMessage(const std::string& name, const Vec4& color) :
    name(name),
    color(color)
  {
    pt = MessageType::NewUserMessage;
  }
  
  virtual ~NewUserMessage() {}
  
  virtual std::string toString() override {
    Encoder e;
    e.add(name);
    e.add(color.x());
    e.add(color.y());
    e.add(color.z());
    e.add(color.w());
    return BasicMessage::toString() + e.getEncodedMessage();
  }
};

struct ConnectMessage : public NewUserMessage {
  bool fastCursorUpdates;
  
  ConnectMessage(const std::string& message) :
    NewUserMessage(message)
  {
    fastCursorUpdates = tokenizer.nextBool();
    pt = MessageType::ConnectMessage;
  }
  
  ConnectMessage(const std::string& name, const Vec4& color, bool fastCursorUpdates) :
  NewUserMessage(name, color),
  fastCursorUpdates(fastCursorUpdates)
  {
    pt = MessageType::ConnectMessage;
  }
  
  virtual ~ConnectMessage() {}
  
  virtual std::string toString() override {
    Encoder e;
    e.add(fastCursorUpdates);
    return NewUserMessage::toString() + e.getEncodedMessage();
  }

};

struct LostUserMessage : public BasicMessage {
  
  LostUserMessage(const std::string& message) :
    BasicMessage(message)
  {
    pt = MessageType::LostUserMessage;
  }
  
  LostUserMessage() {
    pt = MessageType::LostUserMessage;
  }
  
  virtual ~LostUserMessage() {}
};

struct CanvasUpdateMessage : public BasicMessage {
  Vec4 color;
  Vec2i pos;

  CanvasUpdateMessage(const std::string& message) :
    BasicMessage(message)
  {
    float r = tokenizer.nextFloat();
    float g = tokenizer.nextFloat();
    float b = tokenizer.nextFloat();
    float a = tokenizer.nextFloat();
    color = Vec4(r,g,b,a);

    float x = tokenizer.nextInt32();
    float y = tokenizer.nextInt32();
    pos = Vec2i(x,y);

    pt = MessageType::CanvasUpdateMessage;
  }
  
  CanvasUpdateMessage(const Vec4& color, const Vec2i& pos) :
    color(color),
    pos(pos)
  {
    pt = MessageType::CanvasUpdateMessage;
  }
  
  virtual ~CanvasUpdateMessage() {}
  
  virtual std::string toString() override {
    Encoder e;
    e.add(color.x());
    e.add(color.y());
    e.add(color.z());
    e.add(color.w());
    e.add(pos.x());
    e.add(pos.y());
    return BasicMessage::toString() + e.getEncodedMessage();
  }
};

struct InitMessage : public BasicMessage {
  Image image;
  std::vector<ClientInfo> clientInfos;

  InitMessage(const std::string& message) :
    BasicMessage(message)
  {
    const uint32_t w = tokenizer.nextUint32();
    const uint32_t h = tokenizer.nextUint32();
    image = Image(w,h);
    for (size_t i = 0;i<image.data.size();++i) image.data[i] = tokenizer.nextUint8();

    clientInfos.resize(tokenizer.nextUint32());
    for (size_t i = 0;i<clientInfos.size();++i) {
      clientInfos[i].id = tokenizer.nextUint32();
      clientInfos[i].name = tokenizer.nextString();
      
      float r = tokenizer.nextFloat();
      float g = tokenizer.nextFloat();
      float b = tokenizer.nextFloat();
      float a = tokenizer.nextFloat();
      clientInfos[i].color = Vec4(r,g,b,a);

      float x = tokenizer.nextFloat();
      float y = tokenizer.nextFloat();
      clientInfos[i].pos = Vec2(x,y);
    }
    pt = MessageType::InitMessage;
  }

  InitMessage(const Image& image, const std::vector<ClientInfoServerSide>& clientInfosSS) :
    image(image)
  {
    clientInfos.resize(clientInfosSS.size());
    for (size_t i = 0;i<clientInfos.size();++i) {
      clientInfos[i].id = clientInfosSS[i].id;
      clientInfos[i].name = clientInfosSS[i].name;
      clientInfos[i].color = clientInfosSS[i].color;
      clientInfos[i].pos = clientInfosSS[i].pos;
    }
    pt = MessageType::InitMessage;
  }
  
  virtual ~InitMessage() {}
  
  virtual std::string toString() override {
    Encoder e;
    
    e.add(image.width);
    e.add(image.height);
    for (size_t i = 0;i<image.data.size();++i) {
      e.add(image.data[i]);
    }

    e.add(uint32_t(clientInfos.size()));
    for (size_t i = 0;i<clientInfos.size();++i) {
      e.add(clientInfos[i].id);
      e.add(clientInfos[i].name);
      e.add(clientInfos[i].color.x());
      e.add(clientInfos[i].color.y());
      e.add(clientInfos[i].color.z());
      e.add(clientInfos[i].color.w());
      e.add(clientInfos[i].pos.x());
      e.add(clientInfos[i].pos.y());
    }
    return BasicMessage::toString() + e.getEncodedMessage();
  }
};
