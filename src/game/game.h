#pragma once

#include <SFML/Graphics.hpp>
#include <stack>

#include"states/statebase.h"

//SFMLを利用したゲームのメインクラス。
//ウインドウを生成、ゲームループの管理、各Stateの管理を担う
class Game {
public:
	static const int WINDOW_STYLE = sf::Style::Titlebar | sf::Style::Close;
	sf::RenderWindow window;
	const int width, height;
	const int framerate;
	int tryPop;//複数のstateをstateStackに管理し、stateを終える際にtryPopを増やす。
	//ゲームループの各フレームでtryPopの数だけstateStackの上からStatePtrをpop,削除する。
	//複数のstateを１度に終えてタイトルstateに戻る際などに便利。
	bool gameEnded = false;

	std::stack<StatePtr> stateStack;//stateを新規生成する際に、stateをさすポインタをここにpush。
	StatePtr getCurrentState() const;//現在stateStackの一番上にあるstatePtrを返す。
	Game();
	~Game();
	void Run();
	void createWindow();
	void pushState(const StatePtr& statePtr);
	void popState();
	void handleEvents(const StatePtr& currentState);//キー入力を管理する
	void handleTryPop();//ここでtryPopの数だけstateStackの上からStatePtrをpop,削除する。
};