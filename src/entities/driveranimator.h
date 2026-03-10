#pragma once
#include <SFML/Graphics.hpp>
#include<array>

//ドライバーのアニメーションクラス。driverクラスのメンバ
class DriverAnimator {
public:
    enum class PlayerState {
        GO_RIGHT,
        GO_LEFT,
        GO_FORWARD,
        GO_BACK,
    };

    DriverAnimator(const char* spriteFile);
    //アニメーション用画像には角度を変えてドライバーを描画したものが１２種類並んでいる
    sf::Texture driving[12];//１２種類を格納する
    sf::Sprite sprite;//レース中選ばれた角度のテクスチャをセットして描画する
    //速度や進行角度に応じて決まる状態enum
    PlayerState state;
    float uiScaleFactor;//スプライトのスケール、ゲームデザイン依存
    //運転中ドライバーが揺れる
    float spriteMovementSpeed;
    float spriteMovementSpeedTime;
    static constexpr const float MOVEMENT_SPEED_PERIOD = 0.015f;
    static constexpr const float MOVEMENT_SPEED_AMPLITUDE = 0.4f;
    //ミニマップ用:ミニマップ上でのドライバーの向きは進行角度で３６０度を32段階に分けられる
    //ある角度の時は上記のドライバーテクスチャのどのインデックスを使うかを示す配列
    static constexpr const int NUM_MINIMAP_DRIVER_FRAMES = 32;
    std::array<int, NUM_MINIMAP_DRIVER_FRAMES> angleToFrameIndices = {
        0,  1,  2,  3, 4, 5, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11,
        11, 10, 10, 9, 9, 8, 8, 7, 7, 6, 5, 4, 3, 2,  1,  0 };

    void goForward();
    void goRight();
    void goLeft();
    void update(const float speedForward, const float speedTurn, const sf::Time& deltaTime);
    void setViewSprite(float angle);
    sf::Sprite getMinimapSprite(float angle, const float screenScale) const;

};