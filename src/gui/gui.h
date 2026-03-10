#pragma once
#include<SFML/Graphics.hpp>
#include"rank.h"
#include"timer.h"

//レース中に表示されるGUIを管理するクラス、シングルトン
class Gui {
private :
	Gui()=default;

public:
	Timer timer;//詳細はtimer.h
	Rank rank;//詳細はrank.h

	static Gui& getInstance() {
		static Gui instance;  // ← 初回呼び出し時に安全に初期化
		return instance;
	}
	void setRank(int rank);//内部にrankインスタンスの関数を呼び出し、プレイヤーの順位を設定する
	static void update(const sf::Time& deltaTime);
	static void draw(sf::RenderTarget& window);
	static void setLayout();


};