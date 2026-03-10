#pragma once
#include"wallobject.h"

//áŠQ•¨
class Wakame :public WallObject {
public:
	sf::Sprite sprite;
	//initload‚Å‰æ‘œ‚ğæ‚è‚İ
	static void loadAssets(const std::string& assetName,const sf::IntRect& wakameRect);
	static sf::Texture wakameTexture;

	Wakame(const sf::Vector2f& position);

	static sf::Texture& getWakameTexture();
	sf::Sprite& getSprite() override {return sprite;}
};