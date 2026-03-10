#pragma once
#include<memory>
class State;
typedef std::shared_ptr<State> StatePtr;

#include <SFML/Graphics.hpp>
class Game;

//場面を管理するクラスのベースクラス。場面インスタンスがスタックされ、
//そのtopにあるものがgame.cppでupdateで処理されdrawで描画される
class State {
protected:
    Game& game;

public:
    State(Game& _game) : game(_game) {}

    virtual void handleEvent(const sf::Event&) {}
    virtual bool update(const sf::Time&) { return false; }
    virtual bool fixedUpdate(const sf::Time&) { return false; }
    virtual void draw(sf::RenderTarget&) {}

    virtual std::string string() const = 0;
};