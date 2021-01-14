#pragma once 

#include "Renderer.h"

class TextRenderer : public Renderer {
public:
	TextRenderer(uint32_t width, uint32_t height) : Renderer(width,height) {}
	virtual ~TextRenderer() {}
	virtual void render(const std::array<Vec2i,4>& tetrominoPos, const Vec3& currentColor, 
						const std::array<Vec2i,4>& nextTetrominoPos, const Vec3& nextColor,
						const std::array<Vec2i,4>& targerTetrominoPos, float time);
	
	virtual void dropAnimation(const std::array<Vec2i,4>& source, const Vec3& sourceColor, const std::array<Vec2i,4>& target, const std::vector<uint32_t>& clearedRows) {}
	virtual bool isAnimating() const {return false;}
    virtual void setGameOver(bool gameOver, uint32_t score) {}

private:
	static void setColor(const Vec3& color);
	
};
	
	
