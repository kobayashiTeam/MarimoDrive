#include"courseselect.h"
#include"game/game.h"
#include"states/racemanager.h"
#include"gui/textutils.h"
#include"input/input.h"
#include"audio/audio.h"

const sf::Time StateCourseSelect::FADE_IN_TIME = sf::seconds(2.0f);
const sf::Time StateCourseSelect::FADE_OUT_TIME = sf::seconds(1.5f);
const sf::Vector2f StateCourseSelect::ABS_COURSE_TEXT = sf::Vector2f(500,300);
const sf::Vector2f StateCourseSelect::REL_COURSE_TEXT= sf::Vector2f(0,100);

void StateCourseSelect::init() {
	
	selectedCourse = (unsigned int)CourseOption::COURSE1;
	currentPhase = CourseSelectPhase::FADE_IN;
	timeSincePhaseChange = sf::Time::Zero;

	Audio::play(MusicID::MENU_TITLE_SCREEN);
}

void StateCourseSelect::handleEvent(const sf::Event& event) {
	if (currentPhase == CourseSelectPhase::SELECTING) {
		//矢印キーの上下でコース項目選択。エンター（ACCEPET）でコース決定
		if (Input::pressed(Action::MENU_DOWN, event)) {
			Audio::play(SfxID::MENU_SELECTION_MOVE);
			selectedCourse =
				((unsigned int)selectedCourse + 1) % (unsigned int)CourseOption::__COUNT;
		}
		else if (Input::pressed(Action::MENU_UP, event)) {
			Audio::play(SfxID::MENU_SELECTION_MOVE);
			selectedCourse =
				((unsigned int)selectedCourse - 1) % (unsigned int)CourseOption::__COUNT;
		}
		else if (Input::pressed(Action::ACCEPT, event) ||
			Input::pressed(Action::ACCELERATE, event)){
			Audio::play(SfxID::MENU_SELECTION_ACCEPT);
			timeSincePhaseChange = sf::Time::Zero;
			currentPhase = CourseSelectPhase::FADE_OUT;
			}
	}
}

bool StateCourseSelect::update(const sf::Time& deltaTime) {
	timeSincePhaseChange += deltaTime;

	switch(currentPhase) {
	case CourseSelectPhase::FADE_IN:
		// フェードイン時間が過ぎたら選択フェーズに移行
		if (timeSincePhaseChange > FADE_IN_TIME) {
			currentPhase = CourseSelectPhase::SELECTING;
		}
		break;

	case CourseSelectPhase::SELECTING:
		//選択はhandleEventとdrawが担う。やることなし
		break;

	case CourseSelectPhase::FADE_OUT:
		if (timeSincePhaseChange > FADE_OUT_TIME) {
			Audio::pauseMusic();
			//一定時間待ったらフェードアウト後にフェーズ遷移。
			currentPhase = CourseSelectPhase::STATE_CHANGE;
		}
		break;

	case CourseSelectPhase::STATE_CHANGE:
		switch (selectedCourse) {
			//handleEventで選択したコースを元にselectedCourseを引き継いで
			//StateRaceManagerに遷移
		case (unsigned int)CourseOption::COURSE1:
			game.pushState(StatePtr(new StateRaceManager(game,selectedCourse)));
			break;
		case (unsigned int)CourseOption::COURSE2:
			game.pushState(StatePtr(new StateRaceManager(game, selectedCourse)));
			break;
		case (unsigned int)CourseOption::COURSE3:
			game.pushState(StatePtr(new StateRaceManager(game, selectedCourse)));
			break;
		}
		break;
	}

	return true; 
}

bool StateCourseSelect::fixedUpdate(const sf::Time& fixedDeltaTime) {
	return true; 
}

void StateCourseSelect::draw(sf::RenderTarget& window) {
	sf::Sprite background(getBGTexture());
	background.setPosition(0.0f, 0.0f);
	window.draw(background);

	//コーステキスト
	//選択中のコースとそれ以外で色分けをしている
	//Color型で用意した3種類の変数を３つの表示コースのうち、
	//選択中のものとそれ以外で２色で分類する
	sf::Color colors[3] = {
	TextColor::MenuPrimary,
	TextColor::MenuPrimary,
	TextColor::MenuPrimary
	};
	colors[selectedCourse] = TextColor::MenuPrimaryOnFocus;

	TextUtils::write(window, "course 1", ABS_COURSE_TEXT, 4.0f, colors[0]);
	TextUtils::write(window, "course 2", ABS_COURSE_TEXT + REL_COURSE_TEXT, 4.0f, colors[1]);
	TextUtils::write(window, "course 3", ABS_COURSE_TEXT + REL_COURSE_TEXT * 2.0f, 4.0f, colors[2]);


	//フェード用の黒いスプライト。1x1の黒いテクスチャを引き伸ばして使う
	sf::Image iBlack;
	iBlack.create(1, 1, sf::Color::Black);
	sf::Texture tBlack;
	tBlack.loadFromImage(iBlack);
	sf::Sprite sBlack(tBlack);
	sf::Vector2u blackSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	sBlack.setOrigin(0.5f, 0.5f);
	sBlack.setScale(blackSize.x, blackSize.y);
	sBlack.setPosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT/2);

	int alpha =255;
	if (currentPhase == CourseSelectPhase::FADE_IN) {
		float pct = 1-(timeSincePhaseChange / FADE_IN_TIME) ;//1->0で透明に
		//時間経過でpctがマイナスになるので最小値を0に
		alpha = std::fmaxf(pct * 255.0f, 0.0f);
		sBlack.setColor(sf::Color(255, 255, 255, alpha));
		window.draw(sBlack);
	}
	else if (currentPhase == CourseSelectPhase::FADE_OUT) {
		float pct = timeSincePhaseChange /FADE_OUT_TIME;
		//時間経過でpctが1を超えるので最大値を1に
		alpha = std::fminf(pct * 255.0f, 1.0f);
		sBlack.setColor(sf::Color(255, 255, 255, alpha));
		window.draw(sBlack);
	}
	else if (currentPhase == CourseSelectPhase::STATE_CHANGE) {
		sBlack.setColor(sf::Color(255, 255, 255, 255));
		window.draw(sBlack);
	}
	
}

void StateCourseSelect::loadBackgroundAssets(const std::string& assetName,
	const sf::IntRect& roiBackground) {
		getBGTexture().loadFromFile(assetName, roiBackground);
}

sf::Texture& StateCourseSelect::getBGTexture() {
	static sf::Texture BGTexture;
	return BGTexture;
}