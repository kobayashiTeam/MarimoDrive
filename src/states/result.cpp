#include"result.h"
#include"game/game.h"
#include"input/input.h"
#include"gui/textutils.h"
#include"audio/audio.h"
#include"entities/driver.h"
#include"race.h"


const sf::Vector2f StateResult::TEXT_THANKS_POS = sf::Vector2f(450, 300);

void StateResult::init() {
	currentPhase = Phase::FADE_IN;
	timeSincePhaseChange = sf::Time::Zero;
	stateFinished = false;
}

void StateResult::handleEvent(const sf::Event& event) {
	if (currentPhase == Phase::WAIT_ENTER) {
		if (Input::pressed(Action::ACCEPT, event) ||
			Input::pressed(Action::ACCELERATE, event)) {
			Audio::play(SfxID::MENU_SELECTION_ACCEPT);
			currentPhase = Phase::FADE_OUT;
			timeSincePhaseChange = sf::Time::Zero;
		}
	}
}

bool StateResult::update(const sf::Time& deltaTime) {
	return true;
}

bool StateResult::fixedUpdate(const sf::Time& fixedDeltaTime) {
	timeSincePhaseChange += fixedDeltaTime;

	if (stateFinished)return true;
	switch (currentPhase) {
	case Phase::FADE_IN:
		if (timeSincePhaseChange > sf::seconds(2.0f)) {
			timeSincePhaseChange = sf::Time::Zero;
			currentPhase = Phase::WAIT_ENTER;
		}
		break;
	case Phase::WAIT_ENTER:

		break;
	case Phase::FADE_OUT:
		if (timeSincePhaseChange > sf::seconds(2.0f)) {
			timeSincePhaseChange = sf::Time::Zero;
			currentPhase = Phase::STATE_CHANGE;
		}
		break;
	case Phase::STATE_CHANGE:
		stateFinished = true;
		game.popState();
		Audio::pauseMusic();
		break;
	}

	return true;
}

void StateResult::draw(sf::RenderTarget& window) {
	// 背景
	sf::Sprite background(getBGTexture());
	background.setPosition(0.0f, 0.0f);
	window.draw(background);

	//感謝テキスト
	TextUtils::write(window, "thank you very much", sf::Vector2f(150,300), 4.0f);
	TextUtils::write(window, "for playing", sf::Vector2f(150, 400), 4.0f);

	//ランク表示テキスト
	std::string rankMsg = "rank " + std::to_string(player->rank+1) + "!";
	TextUtils::write(window, rankMsg, sf::Vector2f(350, 50), 4.0f);

	//クリアタイムを換算して表示
	//currentRaceTime:レース開始からの経過時間を01:11:11で1分11秒11ミリ秒の形式で表示
	int totalMs = StateRace::currentRaceTime.asMilliseconds();//ミリ秒換算
	int minutes = totalMs / 60000;//60000ms=1min
	int seconds = (totalMs % 60000) / 1000;//残りのミリ秒から秒を換算
	int centiseconds = (totalMs % 1000) / 10;//残りのミリ秒から100分の1秒を換算
	char buffer[64];
	std::snprintf(buffer, sizeof(buffer), "time %02dm %02ds %02dms", minutes, seconds, centiseconds);
	std::string clearTimeMsg = buffer;
	TextUtils::write(window, clearTimeMsg, sf::Vector2f(250, 150), 4.0f);

	//Enterでタイトルへ移る趣旨を伝えるテキスト
	if (currentPhase != Phase::FADE_IN) {
		TextUtils::write(window, "enter to title", sf::Vector2f(450, 600), 4.0f);
	}

	//フェード用の黒スプライトの生成
	sf::Image iBlack;
	iBlack.create(1, 1, sf::Color::Black);
	sf::Texture tBlack;
	tBlack.loadFromImage(iBlack);
	sf::Sprite sBlack(tBlack);
	sf::Vector2u blackSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	sBlack.setOrigin(0.5f, 0.5f);
	sBlack.setScale(blackSize.x, blackSize.y);
	sBlack.setPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);

	if (currentPhase == Phase::FADE_IN) {

		int alpha = 255;
		float pct =
			1-(timeSincePhaseChange / sf::seconds(2.0f));//1->0で透明に
		if (pct >= 1.0)pct = 1.0;
		alpha = std::fminf(pct * 255.0f, 255.0f);
		sBlack.setColor(sf::Color(255, 255, 255, alpha));
		window.draw(sBlack);
	}

	if (currentPhase == Phase::FADE_OUT) {

		int alpha = 255;
		float pct =
			timeSincePhaseChange / sf::seconds(2.0f);//1->0で透明に
		if (pct >= 1.0)pct = 1.0;
		alpha = std::fmaxf(pct * 255.0f, 255.0f);//255をこえたら255に
		sBlack.setColor(sf::Color(255, 255, 255, alpha));
		window.draw(sBlack);
	}

	if (currentPhase == Phase::STATE_CHANGE) {
		sBlack.setColor(sf::Color(255, 255, 255, 255));
		window.draw(sBlack);
	}
}

void StateResult::loadBackgroundAssets(const std::string& assetName,
	const sf::IntRect& roiBackground) {
	getBGTexture().loadFromFile(assetName, roiBackground);
}

sf::Texture& StateResult::getBGTexture() {
	static sf::Texture BGTexture;
	return BGTexture;
}