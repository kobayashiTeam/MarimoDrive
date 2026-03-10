#pragma once
#include<SFML/Graphics.hpp>

//レース中に表示されるランクGUIを管理するクラス。GUIクラスのメンバー
class Rank {
public:
	Rank();

	//1-8の数字が描画された画像を切り出して各textureに保存
	sf::Texture ranks[8];
	sf::Sprite rankSprite;//最終的にゲーム画面に描画される1枚のスプライト。
	int playerRank;
	float rankScale;

	void update(const sf::Time& deltaTime);
	void draw(sf::RenderTarget& window);
	void setRank(int r);
	void setLayout();

};