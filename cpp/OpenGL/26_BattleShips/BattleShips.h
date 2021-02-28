#pragma once

#include <iostream>
#include <string>
#include <memory>
#include <vector>
#include <array>
#include <optional>

#include <GLApp.h>
#include <GLTexture2D.h>
#include <FontRenderer.h>

#include "GameClient.h"

enum class GameState {
  Startup,
  Connecting,
  Pairing,
  BoardSetup,
  WaitingBoardSetup,
  Firing,
  WaitingFiring,
  Finished
};


class BattleShips : public GLApp {
public:
  BattleShips();
  virtual ~BattleShips();
  
  virtual void init() override;
  virtual void mouseMove(double xPosition, double yPosition) override;
  virtual void mouseButton(int button, int state, int mods, double xPosition, double yPosition) override;
  virtual void mouseWheel(double x_offset, double y_offset, double xPosition, double yPosition) override;
  virtual void keyboardChar(unsigned int codepoint) override;
  virtual void keyboard(int key, int scancode, int action, int mods) override;
  virtual void animate(double animationTime) override ;
  virtual void draw() override;

private:
  std::shared_ptr<GameClient> client{nullptr};

  Vec2 normPos{0,0};
  bool rightMouseDown{false};
  Vec2i lastMousePos{-1,-1};
  float wheelScale{100};
  Vec2 startDragPos{0,0};
  double xPositionMouse{ 0.0 };
  double yPositionMouse{0.0};
  Mat4 baseTransformation;
  
  size_t currentImage{0};

  bool addressComplete{false};
  bool nameComplete{false};
  std::string serverAddress{""};
  std::string userName{""};
  Image responseImage;
  void tryToLoadSettings();
  
  void updateMousePos();
  
  static FontRenderer fr;
  
  GameState gameState{GameState::Startup};
  uint32_t level{0};
  size_t pairingMessage{0};
  
  std::vector<std::string> pairingMessages{
    "Searching for victim",
    "Search boat's cook",
    "Triggering final boss",
    "Hero captain looking for work",
    "Is anyone there? hello",
    "Searching for ez target",
    "Activate Radar"
  };
  
  void drawStartup();
  void drawConnecting();
  void drawPairing();
  
  MD5Sum otherShipPlacementMD5;
  bool shipsPlaced{false};
  ShipPlacement myShipPlacement;
  
  MD5Sum shipPlacementToMD5(const ShipPlacement& sp);
};
