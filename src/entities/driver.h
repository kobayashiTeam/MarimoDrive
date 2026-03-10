#pragma once
#include"wallobject.h"
#include"enums.h"
#include"driveranimator.h"
#include<vector>
#include<utility>

//ドライバーの管理クラス。wallObjectの派生
class Driver:public WallObject {
public:
    Driver(const char* spriteFile, const sf::Vector2f& _position,bool _isPlayer,
        RaceRankingArray& _rankOrder);

    float posAngle;
    float speedForward, speedTurn;
    sf::Vector2f wallBounceVector;//壁との反射で生まれた反射座標量はここに保存されpositionに加算される
    sf::Vector2f floorBounceVector;
    int rank;//順位
    int laps;//現在周回数
    //レース中の最高周回数。逆周回すればlapsが減らされmaxlapSoFarと食い違う
    int maxLapSoFar;
    const RaceRankingArray& rankOrder;
	DriverAnimator animator;
    bool isPlayer;
    static DriverPtr realPlayer;//プレイヤーが操作するドライバーのみこのポインタと接続する
    int lastGradient;//今いるタイルの持つ勾配値。これを参考にドライバー間の順位を決定する

    void fixedUpdate(const sf::Time& deltaTime) override;
    void getDrawables(
        const sf::RenderTarget& window, const float scale,
        std::vector<std::pair<float, sf::Sprite*>>& drawables);
	sf::Sprite& getSprite() override { return animator.sprite; }
    void usePlayerControls(float& accelerationLinear);
    void useGradientControls(float& accelerationLinear);
    inline int getLaps() const { return laps; }
    void updateLap();
    void setPosition(const sf::Vector2f _pos);

};