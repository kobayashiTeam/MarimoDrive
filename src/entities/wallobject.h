#pragma once
#include <SFML/Graphics.hpp>
#include"entities/collisiondata.h"

//障害物とドライバーのベースクラス
class WallObject {
public:
    //マップ画像での正規化座標。コンストラクタではマップ画像絶対座標で代入され、
	//それをマップサイズで割り正規化してpositionに格納する。
    sf::Vector2f position;
    //visualRadiousは見た目の半径。
    // カメラ~障害物の中心座標よりvisualRadious分だけカメラ側に寄った位置に
    //障害物が見える。
    //hitBoxRadiousは当たり判定の領域。他障害物やドライバーとお互いの領域が重なると
    //衝突と見なされる
    float visualRadius, hitboxRadius;

    WallObject(const sf::Vector2f& _position, const float _visualRadius,
        const float _hitboxRadius);

    virtual void update(const sf::Time&) {}
    virtual void fixedUpdate(const sf::Time&) {}
    virtual sf::Sprite& getSprite() = 0;

    //衝突者同士の位置関係や速度を引数に、ドライバーに発生する衝突の影響として速度や角度の変化量を
    // dataに格納し、ドライバーの速度や角度に干渉させる。
    virtual bool solveCollision(CollisionData& data,
        const sf::Vector2f& otherSpeed,
        const sf::Vector2f& otherPos,
        const float distance2);
};