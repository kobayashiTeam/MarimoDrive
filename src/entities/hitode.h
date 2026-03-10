#pragma once
#include"wallobject.h"

class Hitode :public WallObject {
public:
	sf::Sprite sprite;

	static void loadAssets(const std::string& assetName, const sf::IntRect& hitodeRect);
	static sf::Texture hitodeTexture;

	Hitode(const sf::Vector2f& position);

	static sf::Texture& getHitodeTexture();
	sf::Sprite& getSprite() override { return sprite; }
};