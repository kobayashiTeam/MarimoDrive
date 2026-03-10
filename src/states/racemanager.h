#pragma once
#include"statebase.h"
#include"enums.h"
#include<memory>
#include<array>

//CourseSelectのあと、ここでRaceStart,Race,RaceEndのstateをpopさせる。レース管理の要
class StateRaceManager : public State {
public:
	DriverArray drivers;//4人のドライバーをさすsharedスマートポインタ、の配列
	RaceRankingArray rankOrder;//レース順位を管理するドライバークラスポインタ、の配列
	DriverPtr player;//playerのみをさすポインタ
	RacePhase currentPhase;
	unsigned int selectedCourse;//courseSelectから引継ぎ

	//ドライバーのテクスチャのパス配列
	static const std::array<std::string, DRIVER_NUM> driverTexturePaths;
	//コースファイルのパス配列
	static const std::array<std::string, 3> courseFilePaths;
	//マップ画像整数座標での各ドライバーの座標。コース3つ分
	static const std::array<sf::Vector2f, 3> PLAYER_START_POSITIONS;
	static const std::array<sf::Vector2f, 3> ENEMY_START_POSITIONS;
	//ENEMY_START_POSITIONSで３つ分のコースの１つの敵の位置だけ設定しておいて、
	//ENEMY_POS_RELを相対位置として加算すれば他の敵の位置もずらして設定できる
	static const sf::Vector2f ENEMY_POS_REL;

	StateRaceManager(Game& _game,unsigned int _selectedCourse) : 
		State(_game),selectedCourse(_selectedCourse) { init(); }
	void init();
	bool update(const sf::Time& deltaTime) override;
	std::string string() const override {
		return "RaceManagerState";
	}
	//コースに応じて各ドライバーのスタート位置を設定する
	void setDriversPos();
};