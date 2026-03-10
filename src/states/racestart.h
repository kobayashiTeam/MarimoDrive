#pragma once
#include"statebase.h"
#include"enums.h"
#include<memory>
#include<array>
#include<thread>

//レース直前、3,2,1,GOまで
class StateRaceStart : public State {
public:
	enum class Phase :unsigned int {
		FADE_IN,
		ASYNC_OVER,
		CHECK_RANK,
		COUNTDOWN,
		STATE_CHANGE
	};
	DriverPtr player;//raceManagerからの引継ぎ
	DriverArray drivers;//raceManagerからの引継ぎ
	DriverArray miniDrivers;//下部のミニマップ用ドライバー配列
	RaceRankingArray& rankOrder;//raceManagerからの引継ぎ
	//rankOrderだけ&引継ぎなのはなぜだっけ？

	//非同期処理関連
	bool asyncLoadFinished;
	std::thread loadingThread;

	Phase currentPhase;
	sf::Time timeSincePhaseChange;
	static const sf::Time TIME_COUNTDOWN;
	static const sf::Vector2f COUNTDOWN_TEXT_POS;
	bool stateFinished;
	int lastSecond;

	StateRaceStart(Game& _game,DriverPtr& _player,DriverArray& _drivers,RaceRankingArray& _rankOrder):
		State(_game),player(_player),drivers(_drivers),rankOrder(_rankOrder),miniDrivers(_drivers)
	{ init(); }
	void init();
	bool update(const sf::Time& deltaTime) override;
	void draw(sf::RenderTarget& window) override;
	std::string string() const override {
		return "RaceStartState";
	}

	void asyncLoad();
};