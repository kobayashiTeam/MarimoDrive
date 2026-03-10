#include"raceend.h"
#include"game/game.h"
#include"input/input.h"
#include"gui/textutils.h"
#include"map/map.h"
#include"entities/driver.h"
#include"gui/gui.h"
#include"entities/collisionhashmap.h"
#include"audio/audio.h"


const sf::Vector2f StateRaceEnd::TEXT_GOAL_POS = sf::Vector2f(350, 300);
const sf::Vector2f StateRaceEnd::TEXT_PROCEED_POS = sf::Vector2f(450, 600);

void StateRaceEnd::init() {
	timeSincePhaseChange = sf::Time::Zero;
	currentPhase = Phase::LITTLE_DELAY;
	StateFinished = false;
}

void StateRaceEnd::handleEvent(const sf::Event& event) {

	if (currentPhase == Phase::CAN_PROCEED) {
		if (Input::pressed(Action::ACCEPT, event) ||
			Input::pressed(Action::ACCELERATE, event)) {
			Audio::play(SfxID::MENU_SELECTION_ACCEPT);
			currentPhase = Phase::FADE_OUT;
			timeSincePhaseChange = sf::Time::Zero;
		}
	}
}

bool StateRaceEnd::fixedUpdate(const sf::Time& deltaTime) {
	timeSincePhaseChange += deltaTime;

	for (unsigned int i = 0; i < drivers.size(); i++) {
		drivers[i]->fixedUpdate(deltaTime);
	}

	// 衝突情報のリセット
	CollisionHashMap::resetDynamic();
	for (unsigned int i = 0; i < drivers.size(); i++) {
		CollisionHashMap::registerDynamic(drivers[i]);
	}
	// 衝突情報の反映
	CollisionData data;
	for (unsigned int i = 0; i < drivers.size(); i++) {
		if (CollisionHashMap::collide(drivers[i], data)) {
			drivers[i]->wallBounceVector = data.momentum;
			drivers[i]->speedForward *= data.speedFactor;
			drivers[i]->speedTurn *= data.speedFactor;
		}
	}
	//ランクとタイマーの内部更新
	Gui::getInstance().update(deltaTime);

	if (StateFinished)return true;
	switch (currentPhase) {
	case Phase::LITTLE_DELAY:
		if (timeSincePhaseChange > sf::seconds(3.0f)) {
			currentPhase = Phase::CAN_PROCEED;
			timeSincePhaseChange = sf::Time::Zero;
		}
		break;
	case Phase::CAN_PROCEED:
		break;
	case Phase::FADE_OUT:
		if (timeSincePhaseChange > sf::seconds(2.0f)) {
			currentPhase = Phase::STATE_CHANGE;
		}
		break;
	case Phase::STATE_CHANGE:
		StateFinished = true;
		Audio::stopEngines();
		game.popState();
		break;
	}
	return true;
}

void StateRaceEnd::draw(sf::RenderTarget& window) {

	//遠景、マップ、ドライバー、ミニマップの描画
	//共通部分の描画処理はrace.cppを参照
	sf::Texture tSkyBack, tSkyFront, tCourse, tMinimap;
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


	std::vector<std::pair<float, sf::Sprite*>> wallObjects;
	Map::getWallDrawables(window, player, 3.0f, wallObjects);
	Map::getDriverDrawables(window, player, drivers, 1.4f, wallObjects);
	player->getDrawables(window, 1.0f, wallObjects);//scale
	std::sort(wallObjects.begin(), wallObjects.end(),
		[](const std::pair<float, sf::Sprite*>& lhs,
			const std::pair<float, sf::Sprite*>& rhs) {
				return lhs.first > rhs.first;
		});
	for (const auto& pair : wallObjects) {
		window.draw(*pair.second);
	}

	float  minimapHeight = SCREEN_HEIGHT * (SKY_HEIGHT_PCT + CIRCUIT_HEIGHT_PCT);
	sMinimap.setPosition(0.0f, minimapHeight);
	window.draw(sMinimap);

	Gui::getInstance().draw(window);

	//GOALテキスト描画　関数の詳細はtextutils.cpp
	TextUtils::write(window, "goal", TEXT_GOAL_POS, 10.0f,sf::Color::Black);

	//「enterでresult画面へ」のテキスト余韻演出中は表示しない
	if (currentPhase != Phase::LITTLE_DELAY) {
		TextUtils::write(window, "enter to result", TEXT_PROCEED_POS, 4.0f, sf::Color::Black);
	}


	//フェード用の黒スプライトを生成、透過度を変える
	sf::Image iBlack;
	iBlack.create(1, 1, sf::Color::Black);
	sf::Texture tBlack;
	tBlack.loadFromImage(iBlack);
	sf::Sprite sBlack(tBlack);
	sf::Vector2u blackSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	sBlack.setOrigin(0.5f, 0.5f);
	sBlack.setScale(blackSize.x, blackSize.y);
	sBlack.setPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

	if (currentPhase == Phase::FADE_OUT) {
		int alpha = 255;
		float pct =
			timeSincePhaseChange / sf::seconds(2.0f);//1->0で透明に
		if (pct >= 1.0)pct = 1.0;
		alpha = std::fminf(pct * 255.0f, 255.0f);//255を越えたら255で止める
		sBlack.setColor(sf::Color(255, 255, 255, alpha));
		window.draw(sBlack);
	}

	if (currentPhase == Phase::STATE_CHANGE) {
		sBlack.setColor(sf::Color(255, 255, 255, 255));
		window.draw(sBlack);
	}

}

