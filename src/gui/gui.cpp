#include"gui.h"

void Gui::setRank(int r) { getInstance().rank.setRank(r); }

void Gui::update(const sf::Time& deltaTime) {
	getInstance().rank.update(deltaTime);
    getInstance().timer.update(deltaTime);
}

void Gui::draw(sf::RenderTarget& window) {
    getInstance().rank.draw(window);
    getInstance().timer.draw(window);
}

void Gui::setLayout() {
	//タイマーと順位表示の位置を設定する関数
    getInstance().timer.setLayout();
    getInstance().rank.setLayout();
    
}