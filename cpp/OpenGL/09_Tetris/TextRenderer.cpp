#include <iostream>

#include "TextRenderer.h"


struct SpritePixel {
	SpritePixel(Vec2i pos, const Vec3& rgb, float alpha) :
		pos{pos},
		rgb(rgb),
		alpha(alpha)
	{}

	Vec2i pos;
	Vec3 rgb;
	float alpha;
};

void TextRenderer::render(const std::array<Vec2i,4>& tetrominoPos, const Vec3& currentColor,
						  const std::array<Vec2i,4>& nextTetrominoPos, const Vec3& nextColor,
						  const std::array<Vec2i,4>& targerTetrominoPos, float) {

	std::vector<Vec3> colorData{getObstacles()};

	std::vector<SpritePixel> spritePixels;
	for (const Vec2i& p : tetrominoPos) {
		spritePixels.push_back(SpritePixel{p,currentColor,1.0f});
	}	

	for (const Vec2i& p : nextTetrominoPos) {
		spritePixels.push_back(SpritePixel{p,nextColor,0.2f});
	}	

	for (const Vec2i& p : targerTetrominoPos) {
		spritePixels.push_back(SpritePixel{p,nextColor,0.2f});
	}	

	for (const auto& p : spritePixels) {
		if (p.pos.y() < 0 || p.pos.x() < 0 || int64_t(p.pos.y()) >= height() || int64_t(p.pos.x()) >= width()) continue;
		size_t index = gridIndex(p.pos.x(), p.pos.y());
		colorData[index] = colorData.at(index) * (1-p.alpha) + p.rgb * p.alpha;
	}
	
	std::cout << "\033[2J\033[;H";
	uint32_t i{0};
	for (uint32_t y = 0;y < height();++y) {
		for (uint32_t x = 0;x < width();++x) {
			setColor(colorData[i++]);
			std::cout << "  ";
		}
		setColor(Vec3{0,0,0});
		std::cout << "\n";
	}
	std::cout << std::flush;
}

void TextRenderer::setColor(const Vec3& color) {
	const uint32_t index = 16 + uint32_t(color.r()*5) +
							6 * uint32_t(color.g()*5) +
						   36 * uint32_t(color.b()*5);

	std::cout << "\033[48;5;" << index << "m";
}
