#pragma once

#include <SFML/Graphics.hpp>
#include <vector>

#include "entities/collisiondata.h"
#include "entities/driver.h"
#include "entities/wallobject.h"
#include "map/map.h"
#include <array>
#include<set>

//レース中の衝突を管理するクラス
class CollisionHashMap {
private:
    //マップを縦横にNUM_DIVISIONSずつ、つまり8*8の格子状に区切り、
    //各グリッド内で静的、動的オブジェクトを登録し、レース中は毎フレーム
    //ドライバーがいるグリッドを特定、その中に登録されているオブジェクトとの
    //位置関係から衝突を検出する。この方法でマップ全てのオブジェクトを調べるより効率がいい
    static constexpr const unsigned int NUM_DIVISIONS = 8;
    static CollisionHashMap instance;
    template <typename T>
    using CollisionMap = std::array<std::set<T>, NUM_DIVISIONS* NUM_DIVISIONS>;
    CollisionMap<WallObject*> staticMap, dynamicMap;

    //マップ画像正規化座標を代入することで、それがどのグリッドに位置するか調べ、インデックスを返す
    static int hash(const sf::Vector2f& pos);
    //静的（レース中位置が変わらないwakame等の障害物オブジェクト）を登録する
    static inline void insertStatic(const sf::Vector2f& pos, const WallObjectPtr& object) {
        instance.staticMap[hash(pos)].insert(object.get());
    }
    //動的（レース中位置が変わるオブジェクト、つまりドライバー）を登録する
    static inline void insertDynamic(const sf::Vector2f& pos, const WallObjectPtr& object) {
        instance.dynamicMap[hash(pos)].insert(object.get());
    }

    CollisionHashMap() : staticMap(), dynamicMap() {}

public:
    static void registerStatic(const WallObjectPtr& object) {
        sf::Vector2f position = object->position;
        float radius = object->hitboxRadius;
        //障害物オブジェクトは位置する座標を中心に当たり判定領域が円状に広がる。
        // それがオブジェクト自体の隣接グリッドにも含まれているかもしれないので、
        // それらのグリッドにも登録する
        // 基本的な上下左右の4近傍
        insertStatic(position + sf::Vector2f(radius, 0.0f), object);
        insertStatic(position + sf::Vector2f(radius * -1.0f, 0.0f), object);
        insertStatic(position + sf::Vector2f(0.0f, radius), object);
        insertStatic(position + sf::Vector2f(0.0f, radius * -1.0f), object);
        // 斜め上などを加えた４近傍
        insertStatic(position + sf::Vector2f(radius, radius), object);
        insertStatic(position + sf::Vector2f(radius * -1.0f, radius), object);
        insertStatic(position + sf::Vector2f(radius * -1.0f, radius * -1.0f), object);
        insertStatic(position + sf::Vector2f(radius, radius * -1.0f), object);
    }

    // レース中に生成される動的オブジェクト
    static void registerDynamic(const WallObjectPtr& object) {
        sf::Vector2f position = object->position;
        float radius = object->hitboxRadius;
        // 4近傍
        insertDynamic(position + sf::Vector2f(radius, 0.0f), object);
        insertDynamic(position + sf::Vector2f(radius * -1.0f, 0.0f), object);
        insertDynamic(position + sf::Vector2f(0.0f, radius), object);
        insertDynamic(position + sf::Vector2f(0.0f, radius * -1.0f), object);
        // 残り４近傍
        insertDynamic(position + sf::Vector2f(radius, radius), object);
        insertDynamic(position + sf::Vector2f(radius * -1.0f, radius), object);
        insertDynamic(position + sf::Vector2f(radius * -1.0f, radius * -1.0f),object);
        insertDynamic(position + sf::Vector2f(radius, radius * -1.0f), object);
    }

    // ドライバー
    static void registerDynamic(const DriverPtr& object) {
        sf::Vector2f position = object->position;
        float radius = object->hitboxRadius;
        //これは４近傍しか登録しない。毎フレーム登録する計算コストを削減する
        insertDynamic(position + sf::Vector2f(radius, 0.0f), object);
        insertDynamic(position + sf::Vector2f(radius * -1.0f, 0.0f), object);
        insertDynamic(position + sf::Vector2f(0.0f, radius), object);
        insertDynamic(position + sf::Vector2f(0.0f, radius * -1.0f), object);
    }

    static bool collide(const DriverPtr& object, CollisionData& data) {
        sf::Vector2f objPos = object->position;
        float objRadius = object->hitboxRadius;
        sf::Vector2f objSpeed =
            object->speedForward *
            sf::Vector2f(cosf(object->posAngle), sinf(object->posAngle));
		//距離の二乗を求めるラムダ関数。平方根を取らないことで計算量を減らす
        const auto dist2 = [&objPos](const WallObject* candidate) {
            sf::Vector2f candPos = candidate->position;
            float dx = objPos.x - candPos.x;
            float dy = objPos.y - candPos.y;
            return dx * dx + dy * dy;
            };
        //ドライバーと同じグリッド内にある各動的オブジェクトと
        //中心間の距離と衝突領域の半径の和を比較し、衝突しているかどうか確認
        for (const auto& candidate : instance.dynamicMap[hash(objPos)]) {
            if (candidate == object.get()) {
                continue;
            }
            float sum = objRadius + candidate->hitboxRadius;
            float d2 = dist2(candidate);
            if (d2 > sum * sum) {
                continue;
            }
            //衝突していた場合の処理は障害物側のwallObejectの持つsolveCollisionで計算され、
            //ドライバーが行うべきリアクションは引数のdataに格納される
            if (candidate->solveCollision(
                data, objSpeed, objPos,d2)) {
                return true;
            }
        }
        //ドライバーと同じグリッド内にある各静的オブジェクトと
        //中心間の距離と衝突領域の半径の和を比較し、衝突しているかどうか確認
        for (const auto& candidate : instance.staticMap[hash(objPos)]) {
            if (candidate == object.get()) {
                continue;
            }
            float sum = objRadius + candidate->hitboxRadius;
            float d2 = dist2(candidate);
            //衝突していた場合の処理は障害物側のwallObejectの持つsolveCollisionで計算され、
            //ドライバーが行うべきリアクションは引数のdataに格納される
            if (d2 < sum * sum &&
                candidate->solveCollision(
                    data, objSpeed, objPos,d2)) {
                return true;
            }
        }
        return false;
    }

    //敵AIは障害物を検知したら避ける（evade）。詳細はdriver.cpp
    //レース中では「敵ドライバーの現在座標に、現在の進行方向に数フレーム分進めた座標」を
    //引数にpositionに代入する。その予定上の座標と障害物とで接触が検知された場合
    //二者を結ぶベクトルがevadeVectorとして保存される
    //レース中にはevadeVectorをもとに障害物を避けるための追加角度を算出し
    //現在角度に加算する。
    static bool evade(const Driver* self, const sf::Vector2f& position,
        float hitboxRadius, sf::Vector2f& evadeVector) {
        const auto dist2 = [&position](const WallObject* candidate) {
            sf::Vector2f candPos = candidate->position;
            float dx = position.x - candPos.x;
            float dy = position.y - candPos.y;
            return dx * dx + dy * dy;
            };
        //動的オブジェクトに
        for (const WallObject* candidate :
            instance.dynamicMap[hash(position)]) {
            if (candidate == self) {
                continue;
            }
            float sum = hitboxRadius + candidate->hitboxRadius;
            float d2 = dist2(candidate);
            if (d2 > sum * sum) {
                continue;
            }
            evadeVector = position - candidate->position;
            return true;
        }
        //静的オブジェクトに
        for (const auto& candidate : instance.staticMap[hash(position)]) {
            if (candidate == self) {
                continue;
            }
            float sum = hitboxRadius + candidate->hitboxRadius;
            float d2 = dist2(candidate);
            if (d2 > sum * sum) {
                continue;
            }
            evadeVector = position - candidate->position;
            return true;
        }
        return false;
    }

    static void resetStatic() {
        for (auto& set : instance.staticMap) {
            set.clear();
        }
    }

    static void resetDynamic() {
        for (auto& set : instance.dynamicMap) {
            set.clear();
        }
    }
};