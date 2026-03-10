#include"hitode.h"
#include"enums.h"

//áŠQ•¨
Hitode::Hitode(const sf::Vector2f& position)
	: WallObject(position, 2.5f, 2.0f) {
	sprite.setTexture(getHitodeTexture());
	sf::Vector2u size = sprite.getTexture()->getSize();
	sprite.setOrigin(size.x / 2, size.y);
}

sf::Texture& Hitode::getHitodeTexture() {
	static sf::Texture hitodeTexture;
	return hitodeTexture;
}

void Hitode::loadAssets(const std::string& assetName, const sf::IntRect& roiHitode) {
	getHitodeTexture().loadFromFile(assetName, roiHitode);
}