#include "gradientdescent.h"
#include <fstream>
#include "map/map.h"
//staticフィールドの宣言と初期化
AIGradientDescent::IntMapMatrix AIGradientDescent::gradientMatrix,
AIGradientDescent::positionMatrix;

int AIGradientDescent::GRADIENT_LAP_CHECK = 0;
int AIGradientDescent::MAX_POSITION_MATRIX = 0;

const std::array<sf::Vector2i, 8> AIGradientDescent::eightNeighbours = {
    sf::Vector2i(0, -1), sf::Vector2i(-1, 0), sf::Vector2i(0, 1),
    sf::Vector2i(1, 0),  sf::Vector2i(1, -1), sf::Vector2i(1, 1),
    sf::Vector2i(-1, 1), sf::Vector2i(-1, -1) };

int AIGradientDescent::weightLand(const MapLand landType) {
    switch (landType) {
    case MapLand::TRACK:
        return 10;
    case MapLand::SLOW:
        return 100;
    case MapLand::OUTER:
    case MapLand::BLOCK:
        return 500000;
    default:
        std::cerr << "AIGradientDescent::weightLand: Invalid landType ("
            << (int)landType << ")" << std::endl;
        return 0;
    }
}

void AIGradientDescent::loadOrCreateGradient(const MapLandMatrix& mapMatrix,
    const sf::FloatRect& goalLineFloat) {
    const std::string gradient = Map::getCourseName() + "/gradient.txt";
    const std::string position = Map::getCourseName() + "/position.txt";

    std::ifstream in(gradient);
    std::ifstream in2(position);
    //外部からゴールまでの距離マップ、総合的な勾配マップをクラスフィールドに取り込む
    if (in.good() && in2.good()) {
        for (unsigned int row = 0; row < mapMatrix.size(); row++) {
            for (unsigned int col = 0; col < mapMatrix[0].size(); col++) {
                in >> gradientMatrix[row][col];
            }
        }
        for (unsigned int row = 0; row < mapMatrix.size(); row++) {
            for (unsigned int col = 0; col < mapMatrix[0].size(); col++) {
                in2 >> positionMatrix[row][col];
            }
        }
        in2 >> GRADIENT_LAP_CHECK;
        //作成するマップによって重さは変わるため
        // 周回の変わり目から余裕を持たせてMAX_POSITION_MATRIXを設定する
        MAX_POSITION_MATRIX = GRADIENT_LAP_CHECK + 10;
    }
    else {
        //まだ作ってなかったら-2で初期化。これから作る
        for (auto& row : gradientMatrix) {
            row.fill(-2);
        }
        for (auto& row : positionMatrix) {
            row.fill(-1);
        }

        // 画面の端、ブロックタイル、遅延エリア等通常領域を除くすべてについて。
        //いったん重みを最大にしておく。
        IntMapMatrix wallPenalty;
        //frontier:話題にしているタイル（の座標）の配列。
        std::vector<sf::Vector2i> wallPenaltyFrontier;
        unsigned int numRows = mapMatrix.size();
        unsigned int numCols = mapMatrix[0].size();
        for (unsigned int row = 0; row < numRows; row++) {
            for (unsigned int col = 0; col < numCols; col++) {
                //ブロックタイル、マップの端は通過不能、敵AIが避けるよう最大の勾配をつける
                if (mapMatrix[row][col] == MapLand::BLOCK || row == 0 ||
                    row == numRows - 1 || col == 0 || col == numCols - 1) {
                    gradientMatrix[row][col] = -1;
                    wallPenalty[row][col] = WALL_PENALTY_MAX;
                    wallPenaltyFrontier.push_back(sf::Vector2i(col, row));
                }
                else if (mapMatrix[row][col] == MapLand::SLOW ||
                    mapMatrix[row][col] == MapLand::OUTER) {
                    //遅延エリア、（落下エリア）も同様
                    wallPenalty[row][col] = WALL_PENALTY_MAX;
                    wallPenaltyFrontier.push_back(sf::Vector2i(col, row));
                }
                else {
                    //通常領域は重み無し
                    wallPenalty[row][col] = 0;
                }
            }
        }

        // 通常不能に近い通常タイルに重みをつけていく。
        int currentPenalty = WALL_PENALTY_MAX;
        for (int pen = 0; pen < WALL_PENALTY_ITERS; pen++) {
            std::vector<sf::Vector2i> nextWallPenaltyFrontier;
            for (const sf::Vector2i& point : wallPenaltyFrontier) {
                int row = point.y;
                int col = point.x;
                int penalty = wallPenalty[row][col];
                if (penalty != currentPenalty) {
                    continue;
                }
                for (const sf::Vector2i& neighbour : eightNeighbours) {
					// 対象の勾配値を持つ隣接タイルは半減させた重みにする。
                    //最大で３つ隣のタイルまで影響を及ぼす
                    unsigned int prow = row + neighbour.y;
                    unsigned int pcol = col + neighbour.x;
                    //重要:通行不能領域の重みづけはすでに終わっている。今回は通行不能の
                    //その周囲にある通常領域（=0）に半分くらいの重みをつける。これで
                    //通行不能ほどではないが、余り近寄らないようにさせる
                    if (prow < numRows && pcol < numCols && wallPenalty[prow][pcol] == 0)
                    {
                        wallPenalty[prow][pcol] = penalty / WALL_PENALTY_FACTOR;
                        nextWallPenaltyFrontier.push_back(sf::Vector2i(pcol, prow));
                    }
                }
            }
            wallPenaltyFrontier = nextWallPenaltyFrontier;
            currentPenalty /= WALL_PENALTY_FACTOR;
        }

        // ゴールポジションを基準にpos(各タイルのゴールまでのステップ数)と
        //gradient（ペナルティも基準にした正式な勾配値）を決定していく
        sf::IntRect goalLineInt(goalLineFloat.left * MAP_TILE_NUM_WIDTH,
            goalLineFloat.top * MAP_TILE_NUM_HEIGHT,
            goalLineFloat.width * MAP_TILE_NUM_WIDTH,
            goalLineFloat.height * MAP_TILE_NUM_HEIGHT);
        using WeightTuple = std::tuple<int, int, int, int>;  // x, y, pos, grad
		//frontier:これからゴール直下（プレイヤー目線ではレース最期の瞬間）から
        //少しずつタイルをずらして勾配をつけていく。その前線をfrontierに
        // 同時波紋上に追加していく。
        std::vector<WeightTuple> frontier;
        sf::Vector2i lapCheckPos;
        //ゴール領域の整数座標値それぞれについて
        for (int irow = 0; irow < goalLineInt.height; irow++) {
            for (int icol = 0; icol < goalLineInt.width; icol++) {
                int row = goalLineInt.top + irow;
                int col = goalLineInt.left + icol;
                //ゴール領域内はステップ、勾配0に設定
                gradientMatrix[row][col] = 0;
                positionMatrix[row][col] = 0;
                // ゴール直下が通常タイルだった場合
                //タイルの重み、「壁からの距離に応じた重み」の和を勾配マップの値とする
                if (mapMatrix[row + 1][col] == MapLand::TRACK) {
                    int initialWeight = weightLand(MapLand::TRACK) + wallPenalty[row + 1][col];
                    gradientMatrix[row + 1][col] = initialWeight;
                    positionMatrix[row + 1][col] = 1;
                    frontier.push_back( WeightTuple(col, row + 1, initialWeight, 1));
                }
                //ゴール直上を周回の変わり目として個別に記録する。単位はマップ画像座標
                if (mapMatrix[row - 1][col] == MapLand::TRACK) {
                    lapCheckPos = sf::Vector2i(col, row - 1);
                }
            }
        }

        // ゴール直上直下以外を埋める
        while (!frontier.empty()) {
            std::vector<WeightTuple> nextFrontier;
            //ゴール直下からスタートして近傍8マスへ波紋上に勾配決定地域を広げていく。
            for (const WeightTuple& point : frontier) {
                for (const sf::Vector2i& neighbour : eightNeighbours) {
                    int row = std::get<1>(point);
                    int col = std::get<0>(point);
                    row += neighbour.y;
                    col += neighbour.x;
                    //勾配値には「ゴールからのステップ数」＋「タイルの種類による重み」＋
                    //「侵入不可エリアからの影響による重み」の和が入る
                    int weight = std::get<2>(point) +
                        weightLand(mapMatrix[row][col]) +
                        wallPenalty[row][col];
                    //現在のfrontierのステップ数に1足したものが新しい外側の
                    //ステップ数になる。
                    int position = std::get<3>(point) + 1;
					// 勾配未定義は更新。
                    // 各pointからweightを引き継ぐうえで、同じタイルが異なるpointからの近傍で
                    //weight更新される場合があり、その際は軽い方のweightを優先する。
                    //そちらが最短経路として記録される。
                    if (gradientMatrix[row][col] == -2 || gradientMatrix[row][col] > weight) 
                    {
                        gradientMatrix[row][col] = weight;
                        positionMatrix[row][col] = position;
                        bool found = false;
                        //新しくweight更新されたタイルについて、
						//nextFrontier配列に添え字インデックスで検索、もうあるなら内容更新。
                        for (auto& element : nextFrontier)
                        {
                            if (std::get<0>(element) == col && std::get<1>(element) == row) 
                            {
                                std::get<2>(element) = weight;
                                std::get<3>(element) = position;
                                found = true;
                                break;
                            }
                        }
						//nextFrontierにまだないなら新規追加する。
                        if (!found)
                        {
                            nextFrontier.push_back( WeightTuple(col, row, weight, position));
                        }
                    }
                }
            }
			//今の境界層が終われば拡大した層を新しいfrontierにする
            frontier = nextFrontier;
        }

        GRADIENT_LAP_CHECK = positionMatrix[lapCheckPos.y][lapCheckPos.x] - 10;
        MAX_POSITION_MATRIX = GRADIENT_LAP_CHECK + 10;
		//出来上がった勾配マップと位置マップを外部に保存しておく。
        // 次回以降はそれを読み込むだけで済むようにする
        std::ofstream out(gradient);
        for (unsigned int row = 0; row < mapMatrix.size(); row++) {
            for (unsigned int col = 0; col < mapMatrix[0].size(); col++) {
                out << gradientMatrix[row][col] << " ";
            }
            out << std::endl;
        }
        std::ofstream out2(position);
        for (unsigned int row = 0; row < mapMatrix.size(); row++) {
            for (unsigned int col = 0; col < mapMatrix[0].size(); col++) {
                out2 << positionMatrix[row][col] << " ";
            }
            out2 << std::endl;
        }
        out2 << GRADIENT_LAP_CHECK << std::endl;
    }
}

int AIGradientDescent::getPositionValue(unsigned int col, unsigned int row) {
    row = std::min(row, (unsigned int)MAP_TILE_NUM_WIDTH - 1u);
    col = std::min(col, (unsigned int)MAP_TILE_NUM_WIDTH - 1u);
    return positionMatrix[row][col];
}

int AIGradientDescent::getPositionValue(const sf::Vector2f& position) {
    return getPositionValue(position.x * MAP_TILE_NUM_WIDTH, position.y * MAP_TILE_NUM_HEIGHT);
}

sf::Vector2f AIGradientDescent::getNextDirection(const sf::Vector2f& position) {
    //画像正規化座標のpositionをマップタイルインデックスに変換、何番目のタイルにあるか
    int row = position.y * MAP_TILE_NUM_HEIGHT;
    int col = position.x * MAP_TILE_NUM_WIDTH;
    sf::Vector2i bestDirection(0, 0);
    //当該タイルの勾配値を取得
    int bestWeight = gradientMatrix[row][col];
    // ゴール直前、ゴール内にいるときの特別処理
    //ゴールより勾配値の小さいタイルは無いので、従来のガイドではなく、
    //上に行ってもらえば順路のコース、新たな周回の最も高い勾配のタイルへ移れる構造。
    if (bestWeight == 0) {
        sf::Vector2f up = sf::Vector2f(0.0f, -1.0f / MAP_TILE_NUM_HEIGHT);
        return up;
    }
    // 8近傍を調べる
    auto bestIter = eightNeighbours.begin();
    int bestValue = -1;
    while (bestValue == -1) {
        //近傍のうち、通行化のタイルを調べる
        bestValue = gradientMatrix[row + bestIter->y][col + bestIter->x];
        bestIter++;
    }
    //ループの終わりに++していたので、--で戻しておく
    //８近傍のうち最も勾配の低いタイルを選ぶために順番ソートしていく。
    //候補(candidate)は-1,0(通行不能orゴール)ではなく、また
    //ゴール通過直後にゴール領域の勾配値(0)に引き寄せられて戻ることのないように
    //例外的にゴール領域付近で下方向(iter->y=1)にはいかないようにする
    auto iter = --bestIter;
    while (iter != eightNeighbours.end()) {
        int candValue = gradientMatrix[row + iter->y][col + iter->x];
        bool isGoalBelow = (candValue == 0 && iter->y == 1);
        if (bestValue > candValue && candValue != -1 && !isGoalBelow) {//(candValue != 0 || iter->y != 1)
            bestValue = candValue;
            bestIter = iter;
        }
        iter++;
    }
	// bestIterは、8近傍のうち、最もゴールに近い（勾配値が小さい）方向へ
    // タイル1枚分の大きさのへのベクトルを指している。
    sf::Vector2f dirScaled(bestIter->x / (float)MAP_TILE_NUM_WIDTH,
        bestIter->y / (float)MAP_TILE_NUM_HEIGHT);
    return dirScaled;
}