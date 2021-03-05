#include "BattleShips.h"

#include "BoardSetupPhase.h"

BoardSetupPhase::BoardSetupPhase(BattleShips* app, GamePhaseID gamePhaseID, const Vec2ui& b) :
BoardPhase(app, gamePhaseID, b),
myShipPlacement{b}
{}

std::optional<ShipPlacement> BoardSetupPhase::getPlacement() const {
  if (currentPlacement >= ShipPlacement::completePlacement.size())
    return myShipPlacement;
  else
    return {};
}

void BoardSetupPhase::mouseMove(double xPosition, double yPosition) {
  BoardPhase::mouseMove(xPosition, yPosition);
    
  const Mat4 invMyBoardTrans = Mat4::inverse(myBoardTrans);
  const Vec2 normMyBoardPos = ((invMyBoardTrans * Vec4(normPos,0,1)).xy() + Vec2{1.0f,1.0f}) / 2.0f;
  
  if (normMyBoardPos.x() < 0 || normMyBoardPos.x() > 1.0 || normMyBoardPos.y() < 0 || normMyBoardPos.y() > 1.0) {
    myCellPos = Vec2ui(std::numeric_limits<uint32_t>::max(), std::numeric_limits<uint32_t>::max());
  } else {
    myCellPos = Vec2ui{std::min(boardSize.x()-1, uint32_t(normMyBoardPos.x() * boardSize.x())),
                       std::min(boardSize.y()-1, uint32_t(normMyBoardPos.y() * boardSize.y()))};
  }
}

void BoardSetupPhase::mouseButton(int button, int state, int mods, double xPosition, double yPosition) {
  BoardPhase::mouseButton(button, state, mods, xPosition, yPosition);

  if (button == GLFW_MOUSE_BUTTON_RIGHT && state == GLFW_PRESS) {
    toggleOrientation();
  }

  if (button == GLFW_MOUSE_BUTTON_LEFT && state == GLFW_PRESS) {
    if (myCellPos.x() < boardSize.x() && myCellPos.y() < boardSize.y()) {
      if (myShipPlacement.addShip({ShipPlacement::completePlacement[currentPlacement], currentOrientation, myCellPos})) currentPlacement++;
    }
  }

}
void BoardSetupPhase::keyboard(int key, int scancode, int action, int mods) {
  BoardPhase::keyboard(key, scancode, action, mods);
  
  if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_R :
        toggleOrientation();
        break;
      case GLFW_KEY_U :
        if (currentPlacement > 0) {
          currentPlacement--;
          myShipPlacement.deleteLastShip();
        }
        break;
    }
  }
}

void BoardSetupPhase::draw() {
  BoardPhase::draw();
  
  if(currentPlacement >= ShipPlacement::completePlacement.size()) return;
  
  
  Image prompt = app->fr.render("Position Your Fleet");
  app->setDrawTransform(app->computeImageTransform({prompt.width, prompt.height}) * Mat4::scaling(0.3f) * Mat4::translation(0.0f,0.9f,0.0f));
  app->drawImage(prompt);

  myBoardTrans = app->computeImageTransform(boardSize) * Mat4::scaling(0.8f);

  app->setDrawTransform(myBoardTrans);
  app->drawLines(gridLines, LineDrawType::LIST, 3);
  
  const std::vector<Ship>& ships = myShipPlacement.getShips();
  
  bool shipAdded = myShipPlacement.addShip({ShipPlacement::completePlacement[currentPlacement], currentOrientation, myCellPos});
     
  for (const Ship& ship : ships) {
    const Vec2ui start = ship.pos;
    const Vec2ui end   = ship.computeEnd();
    for (size_t y = start.y(); y <= end.y(); ++y) {
      for (size_t x = start.x(); x <= end.x(); ++x) {
        
        float tX = (x+0.5f)/boardSize.x()*2.0f-1.0f;
        float tY = (y+0.5f)/boardSize.y()*2.0f-1.0f;

        app->setDrawTransform( Mat4::scaling(1.0f/boardSize.x(),1.0f/boardSize.y(),1.0f) * Mat4::translation(tX,tY,0.0f) * myBoardTrans);
        app->drawImage(shipCell);
      }
    }
  }
  
  if (shipAdded)
    myShipPlacement.deleteLastShip();
  
}

void BoardSetupPhase::toggleOrientation() {
  currentOrientation = Orientation(1-uint32_t(currentOrientation));
}