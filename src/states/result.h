#pragma once
#include"statebase.h"
#include"enums.h"

//レース終了後、タイムやランクや挨拶を表示するstate
class StateResult :public State {
public:
	enum class Phase :unsigned int {
		FADE_IN,
		WAIT_ENTER,
		FADE_OUT,
		STATE_CHANGE
	};

	DriverPtr player;
	Phase currentPhase;
	sf::Time timeSincePhaseChange;
	static const sf::Vector2f TEXT_THANKS_POS;//感謝のテキストを表示する位置
	bool stateFinished;

	//initload.cppでこれを呼び出し、背景アセットを、領域指定して読み込む。
	static void loadBackgroundAssets(const std::string& assetName,
		const sf::IntRect& roiBackground);
	static sf::Texture& getBGTexture();//取り込んだ背景アセットを参照
	StateResult(Game& _game,DriverPtr _player) :State(_game),player(_player) {
		init();
	}
	void init();
	void handleEvent(const sf::Event& event) override;
	bool update(const sf::Time& deltaTime) override;
	bool fixedUpdate(const sf::Time& fixedDeltaTime) override;
	void draw(sf::RenderTarget& target) override;
	std::string string() const override {
		return "ResultState";
	}

};