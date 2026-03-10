#include"wakame.h"
#include"enums.h"


Wakame::Wakame(const sf::Vector2f& position)
	: WallObject(position, 2.5f, 2.0f) {
	sprite.setTexture(getWakameTexture());
	sf::Vector2u size = sprite.getTexture()->getSize();
	sprite.setOrigin(size.x/2,size.y);//ピボットを中心に
}

sf::Texture& Wakame::getWakameTexture() {
	//クラスインスタンスをいくら生成しても、使うwakame画像は共用なので
	//Textureはstaticにしてある。本来は関数ではなくTexture単体で宣言して
	//cppのグローバル部分で定義すればいいが、エラーが起こるので
	//アクセサ関数の中でstatic宣言して、必要な時は必ずwakameTexture()を呼び
	//メソッドチェーン的にloadFromFile()などのメソッドを呼び出す
	static sf::Texture wakameTexture;
	return wakameTexture;
}

void Wakame::loadAssets(const std::string& assetName, const sf::IntRect& roiWakame) {
    getWakameTexture().loadFromFile(assetName, roiWakame);
}