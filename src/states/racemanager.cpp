#include"racemanager.h"
#include"map/map.h"
#include"race.h"
#include"game/game.h"
#include"entities/driver.h"
#include"racestart.h"
#include"enums.h"
#include"raceend.h"
#include"result.h"
#include"audio/audio.h"

const std::array<std::string, DRIVER_NUM> StateRaceManager::driverTexturePaths = {
	"assets/driver/marimo.png",
	"assets/driver/enemy1.png",
	"assets/driver/enemy2.png",
	"assets/driver/enemy3.png"
};

const std::array<std::string, 3> StateRaceManager::courseFilePaths = {
	"assets/course/course1",
	"assets/course/course2",
	"assets/course/course3"
};

const std::array<sf::Vector2f, 3> StateRaceManager::PLAYER_START_POSITIONS = {
	sf::Vector2f(160.f, 650.f),
	sf::Vector2f(140.f, 660.f),
	sf::Vector2f(135.f, 790.f)
};
const std::array<sf::Vector2f, 3> StateRaceManager::ENEMY_START_POSITIONS = {
	sf::Vector2f(130.f, 630.f),
	sf::Vector2f(110.f, 640.f),
	sf::Vector2f(105.f, 770.f)
};
//基準となる敵の位置に対してx座標横に15pxずつずらして配置する
const sf::Vector2f StateRaceManager::ENEMY_POS_REL{15.0f,0.0f};


void StateRaceManager::init() {
	//ドライバー設定、詳細はdriver.h
	for (int i = 0; i < DRIVER_NUM; i++) {
		bool isPlayer = (i == PLAYER_DRIVER_INDEX);

		drivers[i] = std::make_shared<Driver>(
			driverTexturePaths[i],sf::Vector2f(0.f, 0.f),isPlayer,rankOrder
		);

		if (isPlayer) {
			player = drivers[i];
			Driver::realPlayer = player;
		}
	}
	for (int i = 0; i < DRIVER_NUM; i++) {
		rankOrder[i] = drivers[i].get();
	}
	//各コースのロード、詳しくはmap.cpp
	std::string currentCoursePath= courseFilePaths[selectedCourse];
	Map::loadCourse(currentCoursePath);
	//コース内音源のロード、詳しくはaudio.cpp
	Audio::loadCircuit(currentCoursePath);

	//ドライバーのスタート位置
	setDriversPos();

	currentPhase = RacePhase::RACING;
}

bool StateRaceManager::update(const sf::Time& deltaTime) {
	
	switch (currentPhase) {
	case RacePhase::RACING:
		//注意：raceEndからpushが始まって驚くかもしれない。
		//この順番で入れるとstateStackが上からRaceStart,Race,RaceEndから始まって
		//順番に終わってpopしていく。１つのstate内で複数pushする時には逆順になることに注意。
		game.pushState(StatePtr(new StateRaceEnd(game, player, drivers, rankOrder)));
		game.pushState(StatePtr(new StateRace(game, player, drivers, rankOrder)));
		game.pushState(StatePtr(new StateRaceStart(game, player,drivers,rankOrder)));
		currentPhase = RacePhase::RESULT;
		break;
	case RacePhase::RESULT:
		game.pushState(StatePtr(new StateResult(game, player)));
		currentPhase = RacePhase::DONE;
			break;
	case RacePhase::DONE:
		game.popState();
		game.popState();
		break;
	}
	return true;
}

void StateRaceManager::setDriversPos() {
	float x = ENEMY_START_POSITIONS[0].x;
	switch (selectedCourse) {
	case (unsigned int)CourseOption::COURSE1:
		player->setPosition(PLAYER_START_POSITIONS[0]);
		for (int i = 0; i < DRIVER_NUM; i++) {
			if (drivers[i] == player)continue;
			sf::Vector2f pos =
				ENEMY_START_POSITIONS[0] + ENEMY_POS_REL * (float)i;
			drivers[i]->setPosition(pos);
		}
		break;
	case (unsigned int)CourseOption::COURSE2:
		player->setPosition(PLAYER_START_POSITIONS[1]);
		for (int i = 0; i < DRIVER_NUM; i++) {
			if (drivers[i] == player)continue;
			sf::Vector2f pos =
				ENEMY_START_POSITIONS[1] + ENEMY_POS_REL * (float)i;
			drivers[i]->setPosition(pos);
		}
		break;
	case (unsigned int)CourseOption::COURSE3:
		player->setPosition(PLAYER_START_POSITIONS[2]);
		for (int i = 0; i < DRIVER_NUM; i++) {
			if (drivers[i] == player)continue;
			sf::Vector2f pos =
				ENEMY_START_POSITIONS[2] + ENEMY_POS_REL * (float)i;
			drivers[i]->setPosition(pos);
		}
		break;
	}
}