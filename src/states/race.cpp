#include"race.h"
#include"map/map.h"
#include"enums.h"
#include"entities/driver.h"
#include"entities/collisionhashmap.h"
#include<iostream>
#include"gui/gui.h"
#include"game/game.h"
#include"audio/audio.h"

sf::Time StateRace::currentRaceTime = sf::Time::Zero;

void StateRace::init() {

	currentRaceTime = sf::Time::Zero;
	raceFinished = false;
}

bool StateRace::update(const sf::Time& deltaTime) {
	return true;
}

bool StateRace::fixedUpdate(const sf::Time& fixedDeltaTime) {
	if (raceFinished)return true;//レースが終了したら内部更新をスキップ
	currentRaceTime += fixedDeltaTime;

	//プレイヤーやドライバーの運転の内部処理
	for (unsigned int i = 0; i < drivers.size(); i++) {
		drivers[i]->fixedUpdate(fixedDeltaTime);
		Audio::updateEngine(i, drivers[i]->position, 0.0f,
			drivers[i]->speedForward, drivers[i]->speedTurn);
	}
	Audio::updateListener(player->position, player->posAngle, 0.0f);


    // ドライバーが所属するグリッドマップコンテナを更新
    CollisionHashMap::resetDynamic();
	for (unsigned int i = 0; i < drivers.size(); i++) {
		CollisionHashMap::registerDynamic(drivers[i]);
	}
    // 衝突状況を確認
    CollisionData data;
	for (unsigned int i = 0; i < drivers.size(); i++) {
		if (CollisionHashMap::collide(drivers[i], data)) {
			Audio::play(SfxID::CIRCUIT_COLLISION_PIPE);
			drivers[i]->wallBounceVector = data.momentum;
			drivers[i]->speedForward *= data.speedFactor;
			drivers[i]->speedTurn *= data.speedFactor;
		}
	}

	//順位アップデート
	auto StillRacingBegin = rankOrder.begin();
	// 既にゴールしたドライバーをのぞいて計算する
	while (StillRacingBegin != rankOrder.end() &&
		(*StillRacingBegin)->getLaps() > NUM_LAPS_IN_CIRCUIT){
		++StillRacingBegin;
	}
	std::sort(StillRacingBegin, rankOrder.end(),
		[](const Driver* lhs, const Driver* rhs) {
			// 周回数またはどの勾配値のタイルにいるか（ゴールからの距離）でソート
			if (lhs->getLaps() == rhs->getLaps()) {
				return lhs->lastGradient < rhs->lastGradient;
			}
			else {
				return lhs->getLaps() > rhs->getLaps();
			}
		});

	// 順位GUIの更新
	for (unsigned int i = 0; i < rankOrder.size(); i++) {
		rankOrder[i]->rank = i;
		if (rankOrder[i]->isPlayer) {
			Gui::getInstance().setRank(i + 1);
		}
	}
	//タイマー内部の更新
	Gui::getInstance().update(fixedDeltaTime);

	//プレイヤーが全ての周回を終えた。
	if (player->getLaps() > NUM_LAPS_IN_CIRCUIT ||raceFinished) {
		raceFinished = true;
		Audio::play(SfxID::CIRCUIT_GOAL_END);
		Audio::play(MusicID::CIRCUIT_PLAYER_WIN);
		CollisionHashMap::resetStatic();
		CollisionHashMap::resetDynamic();
		
		player->isPlayer=false;
		game.popState();
	}


	return true;
}

void StateRace::draw(sf::RenderTarget& window) {
	//描画するのは遠景、（プレイヤー追尾カメラから見た）マップ、ミニマップ、ドライバー
	//それぞれImage型でmapクラスに保持されている。
	//テクスチャを新規作成、それぞれ専用関数でイメージを変形してテクスチャに設定
	sf::Texture tSkyBack,tSkyFront,tCourse,tMinimap;
	Map::skyImageToTextures(player, tSkyBack, tSkyFront);
	Map::courseImageToTexture(player,tCourse);
	Map::minimapImageToTexture(tMinimap);
	//テクスチャをスプライトに設定
	sf::Sprite sSkyBack(tSkyBack),sSkyFront(tSkyFront), sCourse(tCourse), sMinimap(tMinimap);

	sf::Vector2u windowSize = sf::Vector2u(SCREEN_WIDTH,SCREEN_HEIGHT);
	//サイズ調節。テクスチャの基本サイズにfactorと積を取って画面サイズに合わせる
	float backFactor = windowSize.x / (float)tSkyBack.getSize().x;
	float frontFactor = windowSize.x / (float)tSkyFront.getSize().x;
	sSkyBack.setScale(backFactor, backFactor);
	sSkyFront.setScale(frontFactor, frontFactor);

	// 遠景
	//遠景は空などのback,岩などのfrontで構成されている。遠景スプライトは
	//画面上部に描画され、ピボットは左上なので画面左上にsetPositionする
	sSkyBack.setPosition(0.0f, 0.0f);
	sSkyFront.setPosition(0.0f, 0.0f);
	window.draw(sSkyBack);
	window.draw(sSkyFront);

	//マップ
	//SKY_HEIGHT_PCTはゲームウインドウの縦サイズに対する空の描画領域の縦サイズ割合
	//コースの描画上端位置は空の描画領域の下端と同じ位置になる
	float courseHeight = SCREEN_HEIGHT * SKY_HEIGHT_PCT;
	sCourse.setPosition(0.0f, courseHeight);
	window.draw(sCourse);

	//障害物、ドライバー、プレイヤー
	//それぞれのgetDrawablesでスプライトのスケール、位置を設定したうえでwallObjects配列に
	//加えたうえで最後にforで描画する。その際カメラ深度zもペアで登録して
	//zが大きい、カメラから遠い順に描画、カメラに近いもので上書きする
	std::vector<std::pair<float, sf::Sprite*>> wallObjects;
	Map::getWallDrawables(window, player, 3.0f, wallObjects);
	Map::getDriverDrawables(window, player, drivers, 1.4f, wallObjects);
	player->getDrawables(window, 1.0f, wallObjects);
	std::sort(wallObjects.begin(), wallObjects.end(),
		[](const std::pair<float, sf::Sprite*>& lhs,
			const std::pair<float, sf::Sprite*>& rhs) {
				return lhs.first > rhs.first;
		});
	for (const auto& pair : wallObjects) {
		window.draw(*pair.second);
	}

	//ミニマップ:描画位置は上端から遠景の縦サイズ＋マップの縦サイズ下がった座標
	float  minimapHeight = SCREEN_HEIGHT * (SKY_HEIGHT_PCT+CIRCUIT_HEIGHT_PCT);
	sMinimap.setPosition(0.0f, minimapHeight);
	window.draw(sMinimap);

	//ミニマップのドライバー
	std::sort(miniDrivers.begin(), miniDrivers.end(),
		[](const DriverPtr& lhs, const DriverPtr& rhs) {
			return lhs->position.y < rhs->position.y;
		});
	for (const DriverPtr& driver : miniDrivers) {
		if (!driver)continue;
		//getMinimapSprite:ドライバーが様々な方向を向いているアニメーションアトラスに対して
		//現在ドライバーの向いている角度を引数に適切な向きのアニメーションが入っている
		//スプライトを取得する。詳細はdriveranimator.cpp
		//現在角度だけでなく、そこに角速度*0.2fを加えることで、ハンドル操作感を
		//少しだけ強調する演出。sppedTurnをそのままいれるとアニメーションが忙しなく変わりすぎる
		sf::Sprite miniDriver = driver->animator.getMinimapSprite(
			driver->posAngle + driver->speedTurn * 0.2f, 0.8f);
		//ミニマップは単純な矩形ではなく上記でmode7関数による台形で描画されている。
		//そんなミニマップにドライバーを適切に配置する為計算処理が必要になる
		//詳細はmap.cpp
		//mapCoordinateはスクリーン全体を正規化座標に置き換え
		//mapPositionは画面下半分におくレイアウトまで内部で設定されている
		sf::Vector2f mapPosition = Map::mapCoordinates(driver->position);
		miniDriver.setOrigin(miniDriver.getLocalBounds().width / 2.0f,
			miniDriver.getLocalBounds().height * 0.9f);
		miniDriver.setPosition(mapPosition.x * windowSize.x,
			mapPosition.y * windowSize.y);
		miniDriver.scale(0.5f, 0.5f);
		window.draw(miniDriver);
	}
	//タイマーと順位GUI
	Gui::getInstance().draw(window);
}

