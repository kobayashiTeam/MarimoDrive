#pragma once

#include <SFML/Graphics.hpp>
#include "enums.h"

//レース中のタイマーGUIクラス。guiクラスメンバ
class Timer {
public:
    //タイマー用アトラスに含まれる数字、記号パーツ。textureで受け取る。
    sf::Texture digits[10];  // 0 1 2 3 4 5 6 7 8 9
    sf::Texture comas[2];    // ' ''
    sf::Texture custom[3];   // : - .

    sf::Sprite timerDigits[6];//分、秒、ミリ秒の数字テクスチャを２桁ずつ表示する
	sf::Sprite timerCommas[2];//分と秒、秒とミリ秒の区切りのスプライト

    //一連のスプライトを１つの矩形と見なしたときの左上の座標
    sf::Vector2f leftUpCorner;

    sf::Time time;

    //サイズ調節関連
    //sf::Vector2f scaleFactor;
	const float resolutionScaleFactor = SCREEN_WIDTH / 512;
    sf::Vector2f uiScaleFactor=sf::Vector2f(2.0f,2.0f);

    Timer();

    void setLayout();
    sf::Vector2f getItemPos();

    void update(const sf::Time& deltaTime);
    void draw(sf::RenderTarget& window);

    void reset();
};