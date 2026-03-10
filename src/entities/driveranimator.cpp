#include"driveranimator.h"
#include"enums.h"

DriverAnimator::DriverAnimator(const char* spriteFile) {

	for (int i = 0; i < 12; i++)
        //ドライバーアニメーションアトラスから領域を切り出してテクスチャに格納する
		driving[i].loadFromFile(spriteFile, sf::IntRect(32 * i, 32, 32, 32));

	sprite.setTexture(driving[0]);
	state = PlayerState::GO_FORWARD;
	sprite.setOrigin(sprite.getGlobalBounds().width / 2,
		sprite.getGlobalBounds().height);//ピボットは上端中央
	uiScaleFactor = 3.0f;
    sprite.setScale(uiScaleFactor,uiScaleFactor);
	spriteMovementSpeed = 0.0f;
	spriteMovementSpeedTime = 0.0f;
}

void DriverAnimator::goForward() {
	state = PlayerState::GO_FORWARD;
}

void DriverAnimator::goRight() {
	state = PlayerState::GO_RIGHT;
}

void DriverAnimator::goLeft() {
	state = PlayerState::GO_LEFT;
}

void DriverAnimator::update(const float speedForward, const float speedTurn,
    const sf::Time& deltaTime) {
    switch (state) {
    case PlayerState::GO_FORWARD:
        sprite.setTexture(driving[0]);
        sprite.setScale(uiScaleFactor,uiScaleFactor);
        break;

    case PlayerState::GO_RIGHT:
		//0~1/4,1/4~1/2,1/2~の３段階でドライバーの向きテクスチャの入れ替え
        if (speedTurn < 1.0f * (1.f / 4))
            sprite.setTexture(driving[1]);
        else if (speedTurn < 1.0f * (1.f / 2))
            sprite.setTexture(driving[2]);
        else
            sprite.setTexture(driving[3]);
        sprite.setScale(uiScaleFactor, uiScaleFactor);
        break;

    case PlayerState::GO_LEFT:
        //ゲームスクリーンにおいて右折、時計回りが正の角度
        //反時計回りが負の角度
        if (speedTurn > -1.0f * (1.f / 4))
            sprite.setTexture(driving[1]);
        else if (speedTurn > -1.0f * (1.f / 2))
            sprite.setTexture(driving[2]);
        else
            sprite.setTexture(driving[3]);
        sprite.setScale(-uiScaleFactor, uiScaleFactor);
        break;

    case PlayerState::GO_BACK:
        break;

    default:
        sprite.setTexture(driving[0]);
        sprite.setScale(uiScaleFactor, uiScaleFactor);
        break;
    }

    //ドライバーspriteをレース中に揺らす演出の処理。
    //揺れは速度に高いほど揺れが激しく（＝周期が短くなる）もので、
    //限度（MOVEMENT_SPEED_PERIOD）の範囲内で毎フレーム求める。
    if (speedForward > 0.0f) {//進行中
        spriteMovementSpeedTime = fmodf(
            spriteMovementSpeedTime + speedForward * deltaTime.asSeconds(),
            MOVEMENT_SPEED_PERIOD);
    }
    else {
        //停止中、いきなり0にするのではなく、毎フレーム減衰させる
        spriteMovementSpeedTime /= 1.5f;
    }
    //spriteMovementSpeedはdriverのgetDrawablesで使われる。
    //sf::Sprite::move関数によってドライバーspriteのy座標に毎フレームこの値が加算される
	//MOVEMENT_SPEED_AMPLITUDEの範囲で上下する
    spriteMovementSpeed =
        sinf(spriteMovementSpeedTime * 2.0f * M_PI / MOVEMENT_SPEED_PERIOD) *
        MOVEMENT_SPEED_AMPLITUDE;
    //追加説明:spriteMovementSpeedTimeは0~MOVEMENT_SPEED_PERIO
    // イメージは円の中をspriteMovementSpeedTimeがフレームごとに角速度を変えながら
    // 回転している。角速度はカートの直線速度speedForwardが変換されたもの
}


void DriverAnimator::setViewSprite(float angle) {
    angle = fmodf(angle, 2.0f * M_PI);//-2*M_PI-2*M_PIに抑える
	if (angle < 0) angle += 2.0f * M_PI;//0^-2*M_PIに抑える

    sprite.setScale(CIRCUIT_HEIGHT_PCT, CIRCUIT_HEIGHT_PCT);
}

sf::Sprite DriverAnimator::getMinimapSprite(
    float angle, const float resolutionScaleFactor) const
{
    //スクリーン上における向きの認識が曖昧なので復習したほうがいい
    sf::Sprite minimapSprite(sprite);
    //スプライトサイズは
    //画面サイズ依存のスケール因子とゲームデザインベースのスケール因子の積で決める
    minimapSprite.setScale(uiScaleFactor * resolutionScaleFactor, 
        uiScaleFactor * resolutionScaleFactor);

    //レースにおいてゲーム画面右向きが0度、時計回りに正に増えていく
    //ドライバーアニメーションのテクスチャ配列は数字の小さい順から
    //0で奥、M_PI/2で右、M_PIで手前、3*M_PI/2で左、2*M_PIで奥、で一周を表現する
    //ゲームとしてはangle=0で右向きなので、
    //インデックスに変換する際にはM_PI/2を足して補正する
    angle += M_PI / 2;                  
	angle = fmodf(angle, 2.0f * M_PI);  //-2*M_PI-2*M_PIに抑える

	if (angle < 0) angle += 2.0f * M_PI;//0^-2*M_PIに抑える

        for (int i = 1; i < NUM_MINIMAP_DRIVER_FRAMES + 1; i++) {
            //FRAMESの数で分割された円周のうち、どのangleが領域に入るかでテクスチャを決定する。
            // FRAMES=32なら、2*M_PIを~0.5/32,0.5~1.5/32,1.5~2.5/32...30.5~31.5/32のように分割する
            //しかしこの場合では最期の31.5~32/32までの領域が考慮されていないので
            //処理はスキップされ直前までのテクスチャが使われる。
            if (angle <= ((i - 0.5f) * 2.0f * M_PI) / (float)NUM_MINIMAP_DRIVER_FRAMES) {
                minimapSprite.setTexture(driving[angleToFrameIndices[i - 1]]);
                if (angle > M_PI) {  
                    //アニメーションは奥から右、そこから手前を向いているものしかないので、
                    //0~M_PIをそちらに、以降~2*M_PIは決定したスプライトを反転させて表現する
                    minimapSprite.scale(-1, 1);
                }
                break;
            }
        }

    return minimapSprite;
}