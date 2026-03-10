#pragma once

#include <SFML/Graphics.hpp>

//ドライバーと障害物（ドライバー同士も含む）の接触が検知された場合
//速度、中心座標間の距離、角度を元にドライバーがどのような反発リアクションを取るか、
//算出され、CollisionDataインスタンスに保存される
struct CollisionData {
    //衝突直後にドライバーがmomentumの座標量移動する。フレームごとに減衰されながら作用する
    sf::Vector2f momentum;
    //反発の結果、その時持っていた速度(speedForward)がこの割合だけ減衰する。停止はしないが急減する
    float speedFactor; 

    //衝突タイプは実質１種類だから消してもいいかも
    CollisionData() {}
    CollisionData(const sf::Vector2f&& _momentum, const float _speedFactor)
        : momentum(_momentum), speedFactor(_speedFactor) {
    }
};