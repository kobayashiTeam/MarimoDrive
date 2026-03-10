#pragma once
#include"states/statebase.h"

//Å‰‚ÉstateStack‚É‚Â‚Ü‚ê‚éState
class StateInitLoad : public State {
public:
	StateInitLoad(Game& _game) : State(_game) { init(); }

	void init();
	bool update(const sf::Time& deltaTime) override;
	std::string string() const override {
		return "InitLoadState";
	}
	void loadGlobalAssets();
};