#pragma once
#include<memory>
class State;
typedef std::shared_ptr<State> StatePtr;

#include <SFML/Graphics.hpp>
class Game;

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