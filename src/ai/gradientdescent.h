#pragma once

#include <SFML/Graphics.hpp>
#include <iostream>
#include "enums.h"

//敵AIがルート検索に利用する勾配値マップ制作にまつわるクラス
class AIGradientDescent {
    static const int WALL_PENALTY_MAX = 4096;//通行不能タイルに与える勾配値。最も重いもの
    static const int WALL_PENALTY_FACTOR = 2;//通行不能タイル周囲のタイルへの勾配値の減衰率
    static const int WALL_PENALTY_ITERS = 3;//通行不能タイルから周囲いくつのタイルに影響を与えるか
    typedef std::array<std::array<int, MAP_TILE_NUM_WIDTH>, MAP_TILE_NUM_HEIGHT>
        IntMapMatrix;//勾配値を格納する２重配列、タイルの縦横のサイズ分
    static IntMapMatrix gradientMatrix, positionMatrix;
    //gradientは勾配値.positionはゴール領域から当タイルがどれくらい離れているかのステップ数を格納。
    //gradientを決定する因子の一部。
    static const std::array<sf::Vector2i, 8> eightNeighbours;
    //タイルの周囲８近傍を指定するときに使う２次元ベクトル。詳細はcpp
    //「現在タイルのx座標＋eightNeighboursのいずれかの要素のx座標」のように近傍タイルを指定して調べる
    static int weightLand(const MapLand landType);
    //通常タイル、遅延タイルなど、タイルごとに異なる重みがあり、これを取り出す関数。
	//これも勾配値を決定する因子の一部。

public:
    static int GRADIENT_LAP_CHECK;//スタート直後が最も重い勾配値で、ゴール直前が最も軽い勾配値で
    //race時にはドライバーのいるタイルが持つ勾配値の変化を調べて、大きな変化があれば
    //ゴールをまたぎ次の周へ移ったと判断する。この値はその基準値。
    static int MAX_POSITION_MATRIX;
    static void loadOrCreateGradient(const MapLandMatrix& mapMatrix, const sf::FloatRect& goalLineFloat);
    //勾配値マップの制作関数。
    static int getPositionValue(unsigned int col, unsigned int row);
    //タイルの縦横のインデックスを引数に、そのタイルのposition値を得る。
    static int getPositionValue(const sf::Vector2f& position);
    //position血を得る関数だが、こちらはマップ画像正規化座標を引数に得られる。
    static sf::Vector2f getNextDirection(const sf::Vector2f& position);
    //敵AIが次に進むべきタイルを知る関数。周囲タイルから最も軽い勾配値タイルを探す。詳細はcpp
};