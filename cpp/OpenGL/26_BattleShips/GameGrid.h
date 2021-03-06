#pragma once

#include <vector>
#include <string>

#include <Vec2.h>

#include "ShipPlacement.h"

enum class Cell {
  Unknown,
  Empty,
  Ship,
  EmptyShot,
  ShipShot
};

class GameGrid {
public:
  GameGrid(const Vec2ui& gridSize);

  void setShips(const ShipPlacement& sp);
  void setEnemyShips(const ShipPlacement& sp);
  
  void addHit(const Vec2ui& pos);
  void addMiss(const Vec2ui& pos);
  void addShot(const Vec2ui& pos);

  bool shipSunk(const Vec2ui& pos) const;
  
  bool validate(const std::string& encryptedString, const std::string& password);
  
  Vec2ui getSize() const {return gridSize;}
  Cell getCell(uint32_t x, uint32_t y) const;
  
  void clearUnknown();

  size_t getRemainingHits() const;
  
  ShipLocation findSunkenShip(const Vec2ui& pos) const;
  uint32_t markAsSunk(const Vec2ui& pos);
  uint32_t shipLength(const Vec2ui& pos) const;

private:
  Vec2ui gridSize;
  std::vector<Cell> grid;
  std::vector<Vec2ui> hits;
  std::vector<Vec2ui> misses;
  std::vector<ShipLocation> sunken;

  void clearEmpty();
  
  void addShip(const Vec2ui& pos);
  void setCell(uint32_t x, uint32_t y, Cell c);
};
