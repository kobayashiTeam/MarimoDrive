#include"driver.h"
#include"input/input.h"
#include"map/map.h"
#include"collisionhashmap.h"
#include"ai/gradientdescent.h"
#include <algorithm>
#include"audio/audio.h"


DriverPtr Driver::realPlayer = nullptr;

Driver::Driver(const char* spriteFile, const sf::Vector2f& _position,bool _isPlayer, 
    RaceRankingArray& _rankOrder)
	: WallObject(_position, VISUALRADIOUS, HITBOXRADIOUS), animator(spriteFile), rankOrder(_rankOrder)
{
    speedForward = 0.0f;
    speedTurn = 0.0f;
    wallBounceVector = sf::Vector2f(0.0f, 0.0f);
    floorBounceVector = sf::Vector2f(0.0f, 0.0f);
    //ウインドウにおいて右向きが角度0、時計回りに正。全てのマップはスタート直後
    //必ず上向きにむかうのでデフォルト値として-90度を設定
    posAngle = -M_PI / 2;
    rank = 1;
    laps = 0;
    maxLapSoFar = 0;
	lastGradient = -1;
	isPlayer = _isPlayer;//ドライバーインスタンスの生成時にこの引数でプレイヤーか判断。
}

float normalize(float angle) {
    float normalizedAngle = angle;
    //角度は0~2*M_PIの範囲に正規化する
    while (normalizedAngle >= 2 * M_PI) {
        normalizedAngle -= 2 * M_PI;
    }
    while (normalizedAngle < 0) {
        normalizedAngle += 2 * M_PI;
    }
    return normalizedAngle;
}

void handlerHitBlock(Driver* self, const sf::Vector2f& nextPosition) {
    sf::Vector2f moveWidth = sf::Vector2f(1.0 / MAP_TILE_NUM_WIDTH, 0.0);
    sf::Vector2f moveHeight = sf::Vector2f(0.0, 1.0 / MAP_TILE_NUM_HEIGHT);

    int widthSize = 0;
    sf::Vector2f checkPos;
    for (int j = -1; j <= 1; j += 2) {
        for (int i = 1; i <= 4; i++) {
            checkPos = nextPosition + float(i * j) * moveWidth;
            if (checkPos.x < 0.0f || checkPos.x >= 1.0f ||
                checkPos.y < 0.0f || checkPos.y >= 1.0f) continue;
            if (Map::getLand(nextPosition + float(i * j) * moveWidth) ==
                MapLand::BLOCK) {
                widthSize++;
            }
            else {
                break;
            }
        }
    }
    int heightSize = 0;
    for (int j = -1; j <= 1; j += 2) {
        for (int i = 1; i <= 4; i++) {
            checkPos = nextPosition + float(i * j) * moveHeight;
            if (checkPos.x < 0.0f || checkPos.x >= 1.0f ||
                checkPos.y < 0.0f || checkPos.y >= 1.0f) continue;
            if (Map::getLand(nextPosition + float(i * j) * moveHeight) ==
                MapLand::BLOCK) {
                heightSize++;
            }
            else {
                break;
            }
        }
    }

    float momentumSpeed = sqrtf(powf(self->wallBounceVector.x, 2.0f) +
        powf(self->wallBounceVector.y, 2.0f));

    float factor;
    float angle;
    if (momentumSpeed == 0.0f) {
        factor = self->speedForward;
        angle = self->posAngle;
    }
    else {
        factor = momentumSpeed;
        angle = atan2f(self->wallBounceVector.y, self->wallBounceVector.x);
    }
    self->wallBounceVector = sf::Vector2f(0.0f, 0.0f);
    factor = std::fmax(factor, MAX_NORMALLAND_LINEARSPEED * 0.5);

    sf::Vector2f momentum =
        sf::Vector2f(cosf(angle), sinf(angle)) * fmaxf(0.01f, factor);

    if (widthSize > 4 && heightSize < 4) {
        self->floorBounceVector = sf::Vector2f(momentum.x, -momentum.y);
    }
    else if (widthSize < 4 && heightSize > 4) {
        self->floorBounceVector = sf::Vector2f(-momentum.x, momentum.y);
    }
    else {
        self->floorBounceVector = sf::Vector2f(-momentum.x, -momentum.y);
    }
}


void improvedCheckOfMapLands(Driver* self, const sf::Vector2f& position,
    sf::Vector2f& deltaPosition) {
    sf::Vector2f nextPosition = position + deltaPosition;
    float halfTileWidthInMapCoord =
        float(MAP_TILE_SIZE) / MAP_ASSET_WIDTH / 3.5f;
    float halfTileHeightInMapCoord =
        float(MAP_TILE_SIZE) / MAP_ASSET_HEIGHT / 3.5f;

    float deltaAngle[5] = { 0, M_PI_2, -M_PI_2, M_PI_4, -M_PI_4 };

    for (int i = 0; i < 5; i++) {
        sf::Vector2f shifting = sf::Vector2f(
            cosf(self->posAngle + deltaAngle[i]) * halfTileWidthInMapCoord,
            sinf(self->posAngle + deltaAngle[i]) * halfTileHeightInMapCoord);
        //現在値＋このフレームでの移動量＋進行方向の延長移動量（数フレーム先の到達予定地）に
		//ブロックがあれば今フレームの移動量をキャンセルし、反発移動量を発生させる
        switch (Map::getLand(nextPosition + shifting)) {
        case MapLand::BLOCK:
            Audio::play(SfxID::CIRCUIT_COLLISION_PIPE);
            handlerHitBlock(self, nextPosition + shifting);
            self->speedForward = 0.0f;
            self->wallBounceVector = sf::Vector2f(0.0f, 0.0f);
            deltaPosition = sf::Vector2f(0.0f, 0.0f);
            return;
        default:
            break;
        }
    }
}

void Driver::fixedUpdate(const sf::Time& deltaTime) {

	// 加速度は毎フレーム０に初期化され、関数内で諸々の因子で増減される
    float accelerationLinear = 0.0f;
    // 摩擦力
        accelerationLinear += FRICTION_LINEAR_ACELERATION;
    if ((!Input::held(Action::TURN_LEFT) && !Input::held(Action::TURN_RIGHT)) ||
        (Input::held(Action::TURN_LEFT) && speedTurn > 0.0f) ||
        (Input::held(Action::TURN_RIGHT) && speedTurn < 0.0f)) {
        speedTurn /= 1.2f;
    }

    //メンバーのアニメーターインスタンスへの操作。デフォルトで前向きに設定。
    //speedTurnに応じて向きを変更。animator内部でアニメータースプライトを変更。
    animator.goForward();
    //プレイヤーかそれ以外のドライバーで処理の内訳を分岐する
    if (isPlayer) {
        usePlayerControls(accelerationLinear);
    }
    else {
        useGradientControls(accelerationLinear);
    }

    //順位が低い方が加速度が上がる
    accelerationLinear *= 1.0f + POSITION_ACCELERATION_BONUS_PCT * rank;
    
    //現在値にある属性タイルを参照
    //通常タイル（TRACK）とSLOWで最大速度が変わる。
    MapLand land = Map::getLand(position);
    if (land == MapLand::SLOW) {
        if (speedForward > MAX_SLOWLAND_LINEARSPEED) {
            accelerationLinear += SLOWLAND_LOSE_ACCELERATION;
        }
    }

    
    //速度の上限値は初期値では通常エリアのものを取る
    float maxLinearSpeed=MAX_NORMALLAND_LINEARSPEED;
    //減速エリアでは速度の上限が下がる
    if (land == MapLand::SLOW)maxLinearSpeed = MAX_SLOWLAND_LINEARSPEED;
	//角度の変化量は角速度に前フレームからの時間をかけたもの
    float deltaAngle = speedTurn * deltaTime.asSeconds();
	//座標の変化量＝速度*時間+加速度*時間^2/2
    float deltaSpace = speedForward * deltaTime.asSeconds() +
        accelerationLinear *        (deltaTime.asSeconds() * deltaTime.asSeconds()) / 2.0;
	//求められた座標の変化量が、速度の上限を越えないよう、下限を下回らないようにする
    deltaSpace = std::fminf(deltaSpace, maxLinearSpeed * deltaTime.asSeconds());
    deltaSpace = std::fmaxf(deltaSpace, 0.0f);
	// 速度はフレーム間で保存される。現在の速度に加速度＊経過時間を足したものが新たな速度になる。
    speedForward += accelerationLinear * deltaTime.asSeconds();
    speedForward = std::fminf(speedForward, maxLinearSpeed);
    speedForward = std::fmaxf(speedForward, 0.0f);

	//速度はfloatの一次元値、現在角度に基づいてx,yのベクトルに変換される
    float movementAngle = posAngle;
    sf::Vector2f deltaPosition =
        sf::Vector2f(cosf(movementAngle), sinf(movementAngle)) * deltaSpace;

	//現在座標に対して反発移動座標を足す。フレームごとに半減する
    //速度のように時間との積で決まる量ではなく、連続フレームで移動量自体が変化する。
    deltaPosition += wallBounceVector;
    wallBounceVector /= 1.2f;
	//進入禁止タイルに当たると特有の反発移動が発生する。これは速度と同じで時間との積で決まる量。
    deltaPosition += floorBounceVector * deltaTime.asSeconds();
    floorBounceVector /= 1.3f;

    //あらゆる要素の組み合わせでdeltaPositionが決定、最後にこの移動の結果マップの範囲外にでないか
    //確認され、出る場合は移動キャンセル。
    if (((position + deltaPosition).x < 0.0f ||
        (position + deltaPosition).x > 1.0f) ||
        ((position + deltaPosition).y < 0.0f ||
            (position + deltaPosition).y > 1.0f)) {
        deltaPosition = sf::Vector2f(0.0f, 0.0f);
    }

    //今フレームの移動予定座標から進行方向少し先の座標にブロックがあれば移動キャンセル
    //反発移動量を得る
    if (Map::getLand(position) != MapLand::BLOCK) {
        improvedCheckOfMapLands(this, position, deltaPosition);
    }

    //現在値と現在角度を更新
    position += deltaPosition;
    posAngle += deltaAngle;
	//角度を0~2πの範囲に収める
    posAngle = fmodf(posAngle, 2.0f * M_PI);

	
    updateLap();
    animator.update(speedForward, speedTurn,deltaTime);

    return;
}


void Driver::getDrawables(
    const sf::RenderTarget& window, const float scale,
    std::vector<std::pair<float, sf::Sprite*>>& drawables) {
    //race.cppのupdateでは描画対象のオブジェクトそれぞれがgetDrawablesメソッドを持ち
    // スケールと描画位置を設定したスプライトを用意し、参照渡しされた
    // drawables配列に参加させる。
    //今回はplayer専用のgetDrawables
    float width = window.getSize().x;
    //スプライトのピボットは中央上端。画面の45パーセントの高さの所に上端を合わせる
    float y = window.getSize().y * 45.0f / 100.0f;
    animator.sprite.setPosition(width / 2.0f, y);
    //演出でレース中ドライバーは上下振動する。spriteMovementSpeedだけ
    //spriteはsetPositionの座標から相対的に移動する
    float moveY = animator.spriteMovementSpeed;
    animator.sprite.move(0, moveY * scale);
    animator.sprite.scale(scale, scale);

    //レース中の重複スプライトの描画順序を決める
    //カメラからプレイヤーへの距離をマップ画像スケールに正規化する
    float z = CAM_TO_PLAYER_DST / MAP_ASSET_WIDTH;
    drawables.push_back(std::make_pair(z, &animator.sprite));
}


void simulateSpeedGraph(Driver* self, float& accelerationLinear) {

    //現在速度と通常エリアの速度上限の割合に応じて、加速度の変化率が決まる
    float speedPercentage = self->speedForward / MAX_NORMALLAND_LINEARSPEED;
    //~0.25:立ち上がりは強く
    if (speedPercentage < 0.25f) {
        accelerationLinear += MOTOR_ACCELERATION / 2.0f;
    }
    //0.25~0.45:速度上昇と加速度が並んで上昇。
    else if (speedPercentage < 0.45f) {
        accelerationLinear +=
            MOTOR_ACCELERATION * (speedPercentage + 0.075f);
    }
    //0.45~0.95:もたつかず強い加速
    else if (speedPercentage < 0.95f) {
        accelerationLinear += MOTOR_ACCELERATION / 2.0f;
    }
    //0.95~1:滑らかに最高速度
    else {
        accelerationLinear +=
            (0.05f * MAX_NORMALLAND_LINEARSPEED) / 4.0f;
    }

}


bool incrisingAngularAceleration(Driver* self, float& accelerationAngular) {
    bool drifting = false;
    //角速度、速度が一定以上で高調子のコーナリングをしていれば
	//角加速度を最大にする。高調子でなければ角加速度はかなり小さく、あまり大きく曲がらない
    if (std::fabs(self->speedTurn) >
        (MAX_TURNING_ANGULARSPEED * 0.4f) &&
        std::fabs(self->speedForward) >
        (MAX_NORMALLAND_LINEARSPEED * 0.4f)) {
        accelerationAngular = MOTOR_TURNING_ACCELERATION * 1.0f;
        drifting = true;
    }
    else {
        accelerationAngular = MOTOR_TURNING_ACCELERATION * 0.15f;
    }
    return drifting;
}

void reduceLinearSpeedWhileTurning(Driver* self, float& accelerationLinear,
    float& speedTurn) {
    float speedTurnPercentage = std::fabs(speedTurn / MAX_TURNING_ANGULARSPEED);

	//最高速度に近ければ、角速度に比例して加速度を減らす。
    // 最高速度に近い状態で曲がろうとすれば加速度が大きく減る。？
    if (self->speedForward > MAX_NORMALLAND_LINEARSPEED * 0.9f) {
        accelerationLinear =
            -1.0 * MOTOR_ACCELERATION * speedTurnPercentage;
    }
}


void Driver::usePlayerControls(float& accelerationLinear) {
    // 現在速度に合わせて加速度が決定される
    if (Input::held(Action::ACCELERATE)) {
        simulateSpeedGraph(this, accelerationLinear);
    }
    static bool isBrake = false;
    if (Input::held(Action::BRAKE)) {
        //一定以下の速度ならブレーキはキャンセル
        if (speedForward > 0.2 * MAX_NORMALLAND_LINEARSPEED &&
            !isBrake) {
            isBrake = true;
        }
        //常に摩擦はかかっているので、ブレーキはそれに加算される
        accelerationLinear += BREAK_ACELERATION;
    }
    else {
        isBrake = false;
    }

    bool drift = false;
    if (Input::held(Action::TURN_LEFT) && !Input::held(Action::TURN_RIGHT)) {
        float accelerationAngular = 0.0;
        //角加速度を決定する
        drift = incrisingAngularAceleration(this, accelerationAngular);
		//求めた角加速度を現在の角速度に足す。角速度の上限も考慮する。
        speedTurn = std::fmaxf(speedTurn - accelerationAngular,
            MAX_TURNING_ANGULARSPEED * -1.0f);
		//現在の角速度に応じて、直線速度の加速度を減らす
        reduceLinearSpeedWhileTurning(this, accelerationLinear, speedTurn);
        animator.goLeft();
    }
    else if (Input::held(Action::TURN_RIGHT) && !Input::held(Action::TURN_LEFT)) {
        float accelerationAngular = 0.0;
        drift = incrisingAngularAceleration(this, accelerationAngular);
        speedTurn = std::fminf(speedTurn + accelerationAngular,
            MAX_TURNING_ANGULARSPEED);
        reduceLinearSpeedWhileTurning(this, accelerationLinear, speedTurn);
        animator.goRight();
    }
}


//勾配マップに基づく敵の加速度の決定
void Driver::useGradientControls(float& accelerationLinear) {
    sf::Vector2f dirSum(0.0f, 0.0f);

    //速度が最高速度より十分小さければ確認するタイルは周囲の１枚ずつ、
	//そうでなければ周囲の数枚先まで確認する。
    int tilesForward =
        (speedForward < MAX_NORMALLAND_LINEARSPEED / 4.0f) ? 1: Map::getAIFarVision();
    //求めた確認距離に基づいて
    // 「現在値から周囲のタイルを確認、最も勾配値の低いタイルへの座標を取得」
	//の関数を繰り返す、これで広い範囲を想定したより合理的な進行方向を得る。
    for (int i = 0; i < tilesForward; i++) {
        dirSum += AIGradientDescent::getNextDirection(position + dirSum);
    }

	sf::Vector2f evadeVector;  // 障害物をよけるためのベクトル
    bool evadeFound = false;//探索範囲に障害物があったかどうかを示すフラグ
    //カーブも含めて進行方向わずか先を示すベクトル
    sf::Vector2f scaledForward(
        cosf(posAngle + speedTurn * 0.15f) * speedForward * 0.06f,
        sinf(posAngle + speedTurn * 0.15f) * speedForward * 0.06f);
    //一定以上の速度が出ていれば
    if (speedForward / MAX_NORMALLAND_LINEARSPEED > 0.3f) {
        for (int i = 1; i < 12; i++) {
            //進行方向に少しずつずらして円形の当たり判定領域を並べていく様に衝突を検知し
            //障害物からドライバーをさすベクトルを取得、evadeVectorに格納する関数
            if (CollisionHashMap::evade(this,
                position + scaledForward * (float)i,
                hitboxRadius * 2.0f, evadeVector)) {
                evadeFound = true;
                break;
            }
        }
    }
    // 障害物をよける計算::evadeVectorを変形し、ドライバーが障害物をかわすような
    //回転角度を求める
    float evadeAngle = 0.0f;
    if (evadeFound) {
        static constexpr const float MAX_EVADE_ANGLE = M_PI / 3.0f;//かわす角度は最大30度
        //障害物がこの距離より遠くにあれば回避を考慮しなくていい目安
        static constexpr const float MAX_EVADE_DISTANCE = 1.5f / MAP_TILE_NUM_WIDTH;
        //進行方向から時計回り90度を向いたベクトル
        sf::Vector2f perpDirection(scaledForward.y, scaledForward.x * -1.0f);
        //障害物からドライバーへのベクトルと、ドライバーから右向きのベクトルの内積を取る
        //内積の符号から障害物がドライバーの正面180度のうち、右にあるか左にあるか求められる
        float dotProduct =  perpDirection.x * evadeVector.x + perpDirection.y * evadeVector.y;
        //障害物からドライバーへの距離
        float evadeModule = 
            sqrtf(fabsf(evadeVector.x * evadeVector.x + evadeVector.y * evadeVector.y));
        //両者の距離があまりに遠ければ避けなくていい
        if (evadeModule < MAX_EVADE_DISTANCE) {
            //距離が小さい＝両者が近い時、evadePctを大きくする
            float evadePct = 1.0f - (evadeModule / MAX_EVADE_DISTANCE);
            //最大30度のうち、距離因子で大きさが変わり、内積の符号で右か左かを決める
            evadeAngle = evadePct * MAX_EVADE_ANGLE * (dotProduct > 0.0f ? -1.0f : 1.0f);
        }
    }
    //進行方向に落下タイルがあるかどうか
    bool goingToFall =
        (Map::getLand(position + scaledForward * 16.0f) == MapLand::OUTER ||
            Map::getLand(position + scaledForward * 8.0f) == MapLand::OUTER) &&
        (speedForward / MAX_NORMALLAND_LINEARSPEED > 0.3f);

    //このフレームで向かうべき勾配値タイルへのベクトルを角度に変換
    float targetAngle = std::atan2(dirSum.y, dirSum.x);

	// １つ順位の後ろのプレイヤーの進路をふさぐための角度
    //瞬間的に飲み発生する処理で、ゴールが左前、後続が右前に抜きかけて
    //ドライバーの正面半円に侵入したときのみ妨害角度が発生する
    float angleP2P = 0.0f;
    float goHitBackMultiplier = 1.0f;
    if (rank >= 0 && rank < DRIVER_NUM - 1) {
        const Driver* backPlayer = rankOrder[rank + 1];
		// 後ろにいるのがプレイヤーなら、さらに強く当たりに行く
        goHitBackMultiplier =  backPlayer->isPlayer ? 1.0f : 8.0f;
        sf::Vector2f vecP2P = this->position - backPlayer->position;
        //２者間の距離の平方
        float dP2P_2 = vecP2P.x * vecP2P.x + vecP2P.y * vecP2P.y;
        const int NUM_TILES_FOR_OCCLUSION = 6;
        //6タイル分の距離を正規化し、その平方を求める
        const float DIST_FOR_OCCLUSION =
            (NUM_TILES_FOR_OCCLUSION / (float)MAP_TILE_NUM_WIDTH) *
            (NUM_TILES_FOR_OCCLUSION / (float)MAP_TILE_NUM_WIDTH);
        //妨害を考慮する範囲まで後ろのドライバーが近づいていたら
        if (dP2P_2 < DIST_FOR_OCCLUSION) {
            //自分と後続ドライバーのベクトルを角度に変換する
            //右向きが角度0なので、ex)先行に対して、後続が左にいればangleP2Pは0
            //下にいれば-M_PI/2,上にいればM_PI/2。後続が背後にいる条件は-M_PI/2~M_PI/2
            angleP2P = atan2f(vecP2P.y, vecP2P.x);
            //自分の進行角度との差分を取る
            angleP2P = angleP2P - posAngle;
            angleP2P = normalize(angleP2P);
            if (angleP2P > M_PI_2 && angleP2P < 3.0f * M_PI_2) {
                // 後続がドライバーの正面180度にいる場合、妨害角度は発生しない
                angleP2P = 0.0f;
            }
            
        }
    }
    //後続に対する妨害角度は1/8に影響を押されられる
    //angleP2Pは象限の下側にいる間は0~-M_PI正規化でM_PI~2*M_PI,上側にいると0~M_PI
    //これは先行にとっての右側、左側に反映される。
    //相対角度によって下側（右側）が時計回りにハンドルを切り、上側（左側）が反時計回り
    float goHitBackPlayer = (angleP2P < M_PI ? -1.0f : 1.0f) * angleP2P / 8.0f;
    //ドライバーの向きと角速度から「勾配値タイルへの角度」「障害物を避ける角度」＋「妨害角度」
    //の差を取ったものをdiffとする
    float diff = (targetAngle + evadeAngle + goHitBackPlayer / goHitBackMultiplier)-
        (posAngle + speedTurn * 0.15f);
    diff = fmodf(diff, 2.0f * M_PI);//-2*M_PI~2*M_PIにおさえる
    if (diff < 0.0f) diff += 2.0f * M_PI;//0~2*M_PIにおさえる

    //以下、補助的な角度調節が小さければ、プレイヤーとの距離に応じて加速度が足される可能性の処理
    //diffが0.15f*M_PI以内、つまり変更角度が小さければ
    if (fabsf(M_PI - diff) > 0.85f * M_PI) {
        //十分高い速度で走行していると
        if (speedForward >= 0.4f * MAX_NORMALLAND_LINEARSPEED) {
            //現在の、周回を含めたゴールまでのステップ数を
            //player,driverそれぞれで取得する
            long long int playerPositionValue =
                AIGradientDescent::getPositionValue(realPlayer->position) +
                AIGradientDescent::MAX_POSITION_MATRIX * realPlayer->laps;
            long long int currentPositionValue =
                AIGradientDescent::getPositionValue(position) +
                AIGradientDescent::MAX_POSITION_MATRIX * laps;
            int maxBehind = 100;
            float minProbBehind = 0.7f;
            int maxAhead = 200;
            float minProbAhead = 0.45f;
            //互いの総ステップ数の差を取る。距離があるほど大きくなる
            long long int distance = std::abs(playerPositionValue - currentPositionValue);
            if (rank > realPlayer->rank) {
                //プレイヤーより下位の時
                float variance = 1.0f - minProbBehind;//0.3
                float prob = (((float)distance / maxBehind) * variance) + minProbBehind;//0.7~1.0
                //probは0.7~1.0
                prob = fmaxf(0.5f, fminf(prob, 1.0f));
                //probが高ければif以下に入る。割合高確率
                if (rand() / (float)RAND_MAX <= prob) {
                    simulateSpeedGraph(this, accelerationLinear);//現在速度に合わせて加速度を増加
                    animator.goForward();
                }
            }
            else {
                // プレイヤーより先行しているとき
                float variance = minProbBehind - minProbAhead;//0.35
                //最も離れているとき1.1で最も近いときに1.45
                float prob = ((1.0f - (float)distance / maxAhead) * variance) + minProbAhead;
                //0.5にしかならない？
                prob = fmaxf(0.40f, fminf(prob, 0.5f));
                //比較的低確率で加速処理に入れる
                if (rand() / (float)RAND_MAX <= prob) {
                    simulateSpeedGraph(this, accelerationLinear);
                    animator.goForward();
                }
            }
        }
        else {
            //低速度だと、必ず加速処理に入れる
            simulateSpeedGraph(this, accelerationLinear);
            animator.goForward();
        }
    }

    //ここまで勾配値
    //直進を除くほとんどの場合
    if (diff >= 0.05f * M_PI && diff <= 1.95f * M_PI) {
        float accelerationAngular = MOTOR_TURNING_ACCELERATION;
        float turnMultiplier = goingToFall ? 5.0f : 2.0f;//落下タイルに向かっているか
        float totalMultiplier = goingToFall ? 1.5f : 1.0f;
        if (diff > M_PI) {
            // 左旋回
            speedTurn =
                std::fmaxf(speedTurn - accelerationAngular * turnMultiplier,
                    MAX_TURNING_ANGULARSPEED * -totalMultiplier);
            reduceLinearSpeedWhileTurning(this, accelerationLinear, speedTurn);
            animator.goLeft();
        }
        else {
            // 右旋回
            speedTurn =
                std::fminf(speedTurn + accelerationAngular * turnMultiplier,
                    MAX_TURNING_ANGULARSPEED * totalMultiplier);
            reduceLinearSpeedWhileTurning(this, accelerationLinear, speedTurn);
            animator.goRight();
        }
    }
}


void Driver::updateLap() {
    static constexpr const int CONSECUTIVE_INCREMENTS_FOR_BACKWARDS = 5;
    // positionはマップ内座標で0~1で正規化されているはずなので、それ以外の座標なら
    //0,1 またそれよりわずかに内側の領域も範囲外と見なしている。
    if (position.x < 1e-4f || position.x > 1.0f - 1e-4f || position.y < 1e-4f ||
        position.y > 1.0f - 1e-4f) {
        return;
    }
    //ドライバーの現在座標にあるタイルの勾配値を得る
    int gradient = AIGradientDescent::getPositionValue(
        position.x * MAP_TILE_NUM_WIDTH, position.y * MAP_TILE_NUM_HEIGHT);
    // 勾配が設定されてないタイルは-1になっている。
    // lastGradientは各ドライバーが前フレームまで保持していたメンバ勾配値。
    //フレーム間で勾配値が同じ＝動いてない場合は今回はスキップ
    if (gradient == -1 || gradient == lastGradient) {
        return;
    }
    //スタート直後限定。lastGradientの初期値は-1なので、直下のタイルから勾配値を読み取り
    //lastGradientに代入
    if (lastGradient == -1) {  
        lastGradient = gradient;
        return;
    }
   
    int diff = gradient - lastGradient; //フレーム間の勾配値差を求める
    //勾配値はゴール直前が最小0、遠ざかるほど増えていき、スタート直後が最も大きい。
    //多くの場合レース中のdiffは負になる
    //正しい向きでゴールをまたいだフレーム間ではdiffは大きな正の値になる。
    //マップ毎の勾配値設定であらかじめ初期化しておいた定数GRADIENT_LAP_CHECK
    //diffがこの値より大きければゴールしたと見なされる
    if (diff > AIGradientDescent::GRADIENT_LAP_CHECK) {
        laps = laps + 1;
        if (laps > maxLapSoFar) {
            maxLapSoFar = laps;
        }

    }
    //スタート直後からゴールラインを逆に通った場合は
    //diffは負に大きな値になり、これもGRADIENT_LAP_CHECKと比較され
    //逆周回と見なされlapsが減らされる
    else if (diff < AIGradientDescent::GRADIENT_LAP_CHECK * -1 ) {
        laps--;
    }//laps>0
    lastGradient = gradient;
}

void Driver::setPosition(const sf::Vector2f _pos) {
    float scaleX = 1.0f / MAP_ASSET_WIDTH;
    float scaleY = 1.0f / MAP_ASSET_HEIGHT;
    position = sf::Vector2f(_pos.x * scaleX, _pos.y * scaleY);
}