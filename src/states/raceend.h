#pragma once
#include"statebase.h"
#include"enums.h"
#include<array>

//ゴール直後を管理
class StateRaceEnd :public State {
public:
    enum class Phase :unsigned int {
        LITTLE_DELAY,//ゴールしてから少しの間余韻を持たせる演出のフェーズ
        CAN_PROCEED,
        FADE_OUT,
        STATE_CHANGE
    };
    const DriverPtr player;//raceManagerからの引継ぎ
    const DriverArray drivers;//raceManagerからの引継ぎ
    RaceRankingArray& rankOrder;//raceManagerからの引継ぎ
    Phase currentPhase;
    sf::Time timeSincePhaseChange;
    bool StateFinished;

    static const sf::Vector2f TEXT_GOAL_POS;
    //「enterでresult画面へ」の趣旨を伝えるテキストを画面右下へ表示する座標
    static const sf::Vector2f TEXT_PROCEED_POS;

    StateRaceEnd(Game& game, const DriverPtr& _player,
        const DriverArray& _drivers,RaceRankingArray& _rankOrder)
        : State(game),player(_player),drivers(_drivers),rankOrder(_rankOrder)
    {
        init();
    }
    void init();
    void handleEvent(const sf::Event& event) override;
    bool fixedUpdate(const sf::Time& deltaTime) override;
    void draw(sf::RenderTarget& window) override;
    inline std::string string() const override { return "RaceEnd"; }

};