#include"racestart.h"
#include"race.h"
#include"game/game.h"
#include"map/map.h"
#include"gui/textutils.h"
#include"gui/gui.h"
#include"entities/driver.h"
#include"audio/audio.h"

const sf::Time StateRaceStart::TIME_COUNTDOWN = sf::seconds(4.0f);
const sf::Vector2f StateRaceStart::COUNTDOWN_TEXT_POS = sf::Vector2f(450.0f,200.0f);

void StateRaceStart::init() {

	asyncLoadFinished = false;
	//ここでasyncLoad()関数を非同期に呼び出す
	loadingThread = std::thread(&StateRaceStart::asyncLoad, this);
	currentPhase = Phase::FADE_IN;
	timeSincePhaseChange = sf::Time::Zero;
	stateFinished = false;
	lastSecond = 4;
}

bool StateRaceStart::update(const sf::Time& deltaTime) {
	timeSincePhaseChange += deltaTime;
	int currentSecond;

	if (stateFinished)return true;
	switch (currentPhase) {
	case Phase::FADE_IN:
		if (timeSincePhaseChange >= sf::seconds(2.0f)) {
			currentPhase = Phase::ASYNC_OVER;
			
		}
		break;
	case Phase::ASYNC_OVER:
		//asyncLoad関数の終盤でasyncLoadFinished=trueになるのを待つ
		if (asyncLoadFinished) {
			loadingThread.join();//非同期の終了処理
			currentPhase = Phase::COUNTDOWN;
			timeSincePhaseChange = sf::Time::Zero;
		}
		break;
	case Phase::CHECK_RANK:
		break;
	case Phase::COUNTDOWN:
		currentSecond = (int)((TIME_COUNTDOWN - timeSincePhaseChange).asSeconds());
		if (currentSecond != lastSecond&&currentSecond!=0) {
			Audio::play(SfxID::CIRCUIT_COUNTING);
			lastSecond = currentSecond;
		}
		if (TIME_COUNTDOWN - timeSincePhaseChange < sf::seconds(1.0f)) {
				Audio::play(SfxID::CIRCUIT_READYGO);
				currentPhase = Phase::STATE_CHANGE;
			}
		break;
	case Phase::STATE_CHANGE:
		stateFinished = true;
		game.popState();
		for (unsigned int i = 0; i < drivers.size(); i++) {
			Audio::updateEngine(i, drivers[i]->position, 0.0f,
				drivers[i]->speedForward, drivers[i]->speedTurn);
		}
		Audio::updateListener(player->position, player->posAngle, 0.0f);
		Audio::playEngines(0, false);
		Audio::play(MusicID::CIRCUIT_NORMAL);
		break;
	}

	//順位の更新
	std::sort(rankOrder.begin(), rankOrder.end(),
		[](const Driver* lhs, const Driver* rhs) {
			if (lhs->getLaps() == rhs->getLaps()) {
				return lhs->lastGradient < rhs->lastGradient;
			}
			else {
				return lhs->getLaps() > rhs->getLaps();
			}
		});

	//順位GUIの更新
	for (unsigned int i = 0; i < rankOrder.size(); i++) {
		rankOrder[i]->rank = i;
		if (rankOrder[i]->isPlayer) {
			Gui::getInstance().setRank(i + 1);
		}
	}

	Gui::getInstance().update(deltaTime);
	
	return true;
}

void StateRaceStart::asyncLoad() {
	//マップの勾配値データの読み込み、あるいは作成
	//時間がかかれば「3,2,1」が始まるまで少し待ってもらう演出になる
	Map::loadAI();
	asyncLoadFinished = true;
}

void StateRaceStart::draw(sf::RenderTarget& window) {

	//遠景、マップ、ドライバー、ミニマップの描画
	//共通部分の描画処理はrace.cppを参照
	sf::Texture tSkyBack, tSkyFront,tCourse, tMinimap;
	Map::skyImageToTextures(player, tSkyBack, tSkyFront);
	Map::courseImageToTexture(player, tCourse);
	Map::minimapImageToTexture(tMinimap);

	sf::Sprite sSkyBack(tSkyBack), sSkyFront(tSkyFront), sCourse(tCourse), sMinimap(tMinimap);

	sf::Vector2u windowSize = sf::Vector2u(SCREEN_WIDTH, SCREEN_HEIGHT);
	float backFactor = windowSize.x / (float)tSkyBack.getSize().x;
	float frontFactor = windowSize.x / (float)tSkyFront.getSize().x;
	sSkyBack.setScale(backFactor, backFactor);
	sSkyFront.setScale(frontFactor, frontFactor);

	// 遠景
	sSkyBack.setPosition(0.0f, 0.0f);
	sSkyFront.setPosition(0.0f, 0.0f);
	window.draw(sSkyBack);
	window.draw(sSkyFront);

	float courseHeight = SCREEN_HEIGHT * SKY_HEIGHT_PCT;
	sCourse.setPosition(0.0f, courseHeight);
	window.draw(sCourse);

	//障害物、ドライバーの描画
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
	//ミニマップの描画
	float  minimapHeight = SCREEN_HEIGHT * (SKY_HEIGHT_PCT + CIRCUIT_HEIGHT_PCT);
	sMinimap.setPosition(0.0f, minimapHeight);
	window.draw(sMinimap);

	//ミニマップ上のミニドライバーの描画
	//画面のy座標は高いミニドライバーを先に描画して、y座標の低いものを
	//上書きして遠近感のある描画順を安定させる
	std::sort(miniDrivers.begin(), miniDrivers.end(),
		[](const DriverPtr& lhs, const DriverPtr& rhs) {
			return lhs->position.y < rhs->position.y;
		});
	//スプライトの取得
	for (const DriverPtr& driver : miniDrivers) {
		if (!driver)continue;
		sf::Sprite miniDriver = driver->animator.getMinimapSprite(
			driver->posAngle + driver->speedTurn * 0.2f, 0.8f);
	//スプライトの位置設定
		sf::Vector2f mapPosition = Map::mapCoordinates(driver->position);
		miniDriver.setOrigin(miniDriver.getLocalBounds().width / 2.0f,
			miniDriver.getLocalBounds().height * 0.9f);
		miniDriver.setPosition(mapPosition.x * windowSize.x,mapPosition.y * windowSize.y);
		miniDriver.scale(0.5f, 0.5f);
		window.draw(miniDriver);
	}

	//タイマー、現在順位などGUIの描画
	Gui::getInstance().draw(window);


	if (currentPhase == Phase::FADE_IN) {
		//fade black window
		sf::Image iBlack;
		iBlack.create(1, 1, sf::Color::Black);
		sf::Texture tBlack;
		tBlack.loadFromImage(iBlack);
		sf::Sprite sBlack(tBlack);
		sf::Vector2u blackSize(SCREEN_WIDTH, SCREEN_HEIGHT);
		sBlack.setOrigin(0.5f, 0.5f);
		sBlack.setScale(blackSize.x, blackSize.y);
		sBlack.setPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

		//フェードイン演出
		int alpha = 255;
		float pct =
			1 - (timeSincePhaseChange / sf::seconds(2.0f));//1->0で透明に
		if (pct >= 1.0)pct = 1.0;
		alpha = std::fmaxf(pct * 255.0f, 0.0f);
		sBlack.setColor(sf::Color(255, 255, 255, alpha));
		window.draw(sBlack);
	}

	//カウントダウン、処理の詳細はtextutils.cpp
	if (currentPhase == Phase::COUNTDOWN) {
		int remainingTime = (TIME_COUNTDOWN - timeSincePhaseChange).asSeconds();
		switch (remainingTime) {
		case 3:
			TextUtils::write(window, "3", COUNTDOWN_TEXT_POS, 8.0f);
			break;
		case 2:
			TextUtils::write(window, "2", COUNTDOWN_TEXT_POS, 8.0f);
			break;
		case 1:
			TextUtils::write(window, "1", COUNTDOWN_TEXT_POS, 8.0f);
			break;

		}
	}
}