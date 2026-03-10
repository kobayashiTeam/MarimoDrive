#include"sango.h"
#include"enums.h"


Sango::Sango(const sf::Vector2f& position)
	: WallObject(position, 2.5f, 2.0f) {
	//画像は共用でinitloadでファイル読み込み、
	//そしてコースに応じて障害物を生成していく際にコンストラクタで
	//sprite設定を行う
	sprite.setTexture(getSangoTexture());
	sf::Vector2u size = sprite.getTexture()->getSize();
	sprite.setOrigin(size.x / 2, size.y);
}

sf::Texture& Sango::getSangoTexture() {
	static sf::Texture sangoTexture;
	return sangoTexture;
}

void Sango::loadAssets(const std::string& assetName, const sf::IntRect& roiSango) {
	getSangoTexture().loadFromFile(assetName, roiSango);
}