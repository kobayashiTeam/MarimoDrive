#include"wallobject.h"
#include"enums.h"
#include"entities/collisiondata.h"

WallObject::WallObject(const sf::Vector2f& _position, const float _visualRadius,
    const float _hitboxRadius)
    : visualRadius(_visualRadius / MAP_ASSET_WIDTH), hitboxRadius(_hitboxRadius / MAP_ASSET_WIDTH)
{
    float scaleX = 1.0f / MAP_ASSET_WIDTH;
    float scaleY = 1.0f / MAP_ASSET_HEIGHT;
    position = sf::Vector2f(_position.x * scaleX, _position.y * scaleY);
}

bool WallObject::solveCollision(CollisionData& data,
    const sf::Vector2f& otherSpeed, const sf::Vector2f& otherPos,
    const float distance2) {
    
    //低速でぶつかっても0.05fだけ最低限補正し、めりこまないようにする
    float otherSpeedModule =
        sqrtf(otherSpeed.x * otherSpeed.x + otherSpeed.y * otherSpeed.y) +  0.05f;
    //発生したmomentumは座標変化量で、発生から各フレームで減衰しながら
    //ドライバーの位置に加算される。
    sf::Vector2f momentum =
        (otherPos - position) * otherSpeedModule / (35.0f * sqrtf(distance2));
    data = CollisionData(std::move(momentum), 0.4f);
    return true;
}