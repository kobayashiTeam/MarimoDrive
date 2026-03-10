#include<iostream>

#include"Game.h"
#include"enums.h"
#include"states/initload.h"
#include"input/input.h"
#include"gui/gui.h"

Game::Game():width(SCREEN_WIDTH),height(SCREEN_HEIGHT),framerate(FRAMERATE),tryPop(0){
	createWindow();
    Input::setGameWindow(window);//Inputクラスの初期化関数
    Gui::getInstance().setLayout();
    pushState(StatePtr(new StateInitLoad(*this)));
}

Game::~Game() {
}

void Game::Run() {
    
    sf::Clock timer;
    sf::Time lastTime = sf::Time::Zero;
    sf::Time fixedUpdateStep = sf::seconds(1.0f / framerate);//設定フレームレートにおける
    //1フレームでの秒数
    sf::Time fixedUpdateTime = sf::Time::Zero;

    while (window.isOpen()) {
        StatePtr currentState = getCurrentState();

		sf::Time time = timer.getElapsedTime();//ゲーム開始からの経過時間
		sf::Time deltaTime = time - lastTime;//前フレームからの経過時間
        lastTime = time;//last:直前フレームでのElapsedTime
        if (deltaTime > sf::seconds(1.0f)) continue;
        //前回のフレームで時間がかかりすぎた場合、このフレームで異常に大きな移動や回転が
        //発生してしまう。今回の間隔でのロジック処理は打ち切る。
        handleEvents(currentState);
        bool updated = currentState->update(deltaTime);
        fixedUpdateTime += deltaTime;
        while (fixedUpdateTime >= fixedUpdateStep) {
            //fixedUpdateを呼び出す時間を越えて時間経過していた場合、
            //必要なら複数回連続で呼び出して処理の元を取る。
			//ex)60fpsで180フレーム経過していた場合、3回呼び出す。
            fixedUpdateTime -= fixedUpdateStep;
            updated = currentState->fixedUpdate(fixedUpdateStep) || updated;
        }
        if (updated) {
            currentState->draw(window);
        }
        window.display();

        handleTryPop();
        if (gameEnded) {
            window.close();
        }
    }
}

void Game::createWindow() {
    if (window.isOpen()) {
        window.close();
    }
    window.create(sf::VideoMode(width,height),"Marimo Drive", WINDOW_STYLE);
    window.setFramerateLimit(framerate);
}

StatePtr Game::getCurrentState() const { return stateStack.top(); }

void Game::handleEvents(const StatePtr& currentState) {
    sf::Event event;
    while (window.pollEvent(event)) {
        currentState->handleEvent(event);
        //各stateにそれぞれキー入力のイベント処理関数を持たせ、それを呼び出す
        //メニュー画面ではカーソル移動や決定、レース中はアクセル、ブレーキなど
        if (event.type == sf::Event::Closed) {
            gameEnded = true;
        }
    }
}

void Game::handleTryPop() {
    while (tryPop > 0) {
        tryPop--;
        if (stateStack.empty()) {
            std::cerr << "Error: Popped too many states" << std::endl;
            gameEnded = true;
        }
        else {
			//stateStackのトップをpopする。stateStackが空になった場合はゲーム終了。
            stateStack.pop();
                if (stateStack.empty()) {
                    gameEnded = true;
                }
        }
    }
}

void Game::pushState(const StatePtr& statePtr) {
    stateStack.push(statePtr);
}

void Game::popState() { tryPop++; }