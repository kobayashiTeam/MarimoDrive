#include"rank.h"
#include"enums.h"

Rank::Rank() {
    playerRank = DRIVER_NUM;//どのレースでも初期値は４位
    for (int i = 0; i < 8; i++)
        //1,2,3...8まで数字が並んでいる画像を部分的に切り出して保存
        ranks[i].loadFromFile("assets/gui/ranks.png", sf::IntRect(0 + (i * 17), 0, 16, 20));
	rankSprite.setTexture(ranks[7]);
    rankScale = 3;
    rankSprite.setScale(rankScale, rankScale);
    rankSprite.setColor(sf::Color(255, 255, 255, 180));
    //スプライトのピボットを設定。画像右下を基準に
    rankSprite.setOrigin(rankSprite.getLocalBounds().width,
        rankSprite.getLocalBounds().height);
}

void Rank::update(const sf::Time& deltaTime) {
}

void Rank::draw(sf::RenderTarget& window) {
	window.draw(rankSprite);
}

void Rank::setRank(int r) {
        if (r > DRIVER_NUM) r = DRIVER_NUM;
        if (r < 1) r = 1;
        rankSprite.setTexture(ranks[r - 1]);
        playerRank = r;
}

void Rank::setLayout() {
    
    float factor = SCREEN_WIDTH / 512;//ランク画像は画面幅512px基準で用意されている。
    rankSprite.setScale(rankScale * factor, rankScale * factor);
    rankSprite.setPosition(SCREEN_WIDTH * 95 / 100, SCREEN_HEIGHT * 45 / 100);

}