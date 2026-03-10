#pragma once
#include "statebase.h"
#include"enums.h"
#include<memory>
#include<array>

//ƒŒ[ƒXŠJn’¼ŒãAƒS[ƒ‹‚Ü‚Å‚ğŠÇ—‚·‚é
class StateRace : public State {
public:
	DriverPtr player;//raceManager‚©‚ç‚ÌˆøŒp‚¬
	DriverArray drivers;//raceManager‚©‚ç‚ÌˆøŒp‚¬
	DriverArray miniDrivers;//drivers‚ªˆø‚«Œp‚®‚à‚Ì‚ğ‹¤—L‚·‚é
	RaceRankingArray& rankOrder;//raceManager‚©‚ç‚ÌˆøŒp‚¬
	static sf::Time currentRaceTime;
	bool raceFinished;//ƒS[ƒ‹‚Étrue

	StateRace(Game& _game,DriverPtr& _player,DriverArray& _drivers,RaceRankingArray& _rankOrder) :
		State(_game),player(_player),drivers(_drivers),rankOrder(_rankOrder),miniDrivers(_drivers)
	{ init(); }

	void init();
	bool update(const sf::Time& deltaTime) override;
	bool fixedUpdate(const sf::Time& fixedDeltaTime) override;
	void draw(sf::RenderTarget& target) override;
	std::string string() const override {
		return "RaceState";
	}
};