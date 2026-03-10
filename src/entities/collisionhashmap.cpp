#include "collisionhashmap.h"

CollisionHashMap CollisionHashMap::instance;

int CollisionHashMap::hash(const sf::Vector2f& pos) {
    //マップをグリッド分割し、左上から右下まで一連のインデックスで管理している
    //インデックスはhashY * NUM_DIVISIONS + hashXで求められる
    unsigned int hashX =
        std::min((unsigned int)(pos.x * NUM_DIVISIONS), NUM_DIVISIONS - 1);
    unsigned int hashY =
        std::min((unsigned int)(pos.y * NUM_DIVISIONS), NUM_DIVISIONS - 1);
    return hashY * NUM_DIVISIONS + hashX;
}