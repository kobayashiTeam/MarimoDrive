#include"initload.h"
#include"game/game.h"
#include"states/racemanager.h"
#include<iostream>
#include"entities/wakame.h"
#include"entities/sango.h"
#include"entities/hitode.h"
#include"states/courseselect.h"
#include"gui/textutils.h"
#include"states/result.h"
#include"audio/audio.h"

void StateInitLoad::init() {
	loadGlobalAssets();
}

bool StateInitLoad::update(const sf::Time& deltaTime) {
	game.pushState(StatePtr(new StateCourseSelect(game)));
	return true;
}

void StateInitLoad::loadGlobalAssets() {
	
	//WallObjects:障害物
	//複数の障害物が1枚に共有されているため、各領域の左上座標とサイズを指定して切り取っている
	Wakame::loadAssets("assets/objects/wall/wallMisc.png",
		sf::IntRect(sf::Vector2i(0, 0), sf::Vector2i(23, 32)));

	Sango::loadAssets("assets/objects/wall/wallMisc.png",
		sf::IntRect(sf::Vector2i(23, 0), sf::Vector2i(23, 32)));

	Hitode::loadAssets("assets/objects/wall/wallMisc.png",
		sf::IntRect(sf::Vector2i(46, 0), sf::Vector2i(23, 32)));


	//openingBG::タイトル画面の背景
	StateCourseSelect::loadBackgroundAssets("assets/opening/opening.png",
		sf::IntRect(0, 0, 1024, 648));
	//resultBG:リザルト画面の背景
	StateResult ::loadBackgroundAssets("assets/result/resultBG.png",
		sf::IntRect(0, 0, 1024, 648));

	//Text:テクスチャアトラスファイル。アルファベット文字を切り取る。
	TextUtils::loadAssets("assets/gui/letters.png",
		"assets/gui/letters_alpha.png", sf::Vector2i(1, 1),
		sf::Vector2i(1, 32));

	//audio
	Audio::loadAll();
}