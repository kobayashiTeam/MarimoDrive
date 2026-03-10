#include "timer.h"
#include"enums.h"
#include"states/race.h"

Timer::Timer() {
	//数字、記号を１枚のアトラスから切り出して各テクスチャに受け取る
    std::string spriteFile = "assets/gui/digits.png";
    for (int i = 0; i < 10; i++)//数字0-9
        digits[i].loadFromFile(spriteFile, sf::IntRect(0 + (i * 9), 0, 8, 14));
    for (int i = 0; i < 2; i++)//'と''
        comas[i].loadFromFile(spriteFile, sf::IntRect(90 + (i * 9), 0, 8, 14));
    for (int i = 0; i < 3; i++)//: - .
        custom[i].loadFromFile(spriteFile,sf::IntRect(137 + (i * 11), 0, 8, 14));

    //scaleFactor = sf::Vector2f(2, 2);
    for (int i = 0; i < 6; i++) {
        timerDigits[i].setTexture(digits[0]);
        timerDigits[i].scale(uiScaleFactor);
    }

    timerCommas[0].setTexture(comas[0]);
    timerCommas[0].scale(uiScaleFactor);
    timerCommas[1].setTexture(comas[1]);
    timerCommas[1].scale(uiScaleFactor);

    time = time.Zero;

    leftUpCorner = sf::Vector2f(0, 0);
}

sf::Vector2f Timer::getItemPos() {
    return sf::Vector2f(
        leftUpCorner.x,
        leftUpCorner.y + timerDigits[0].getGlobalBounds().height / 2);
}

void Timer::setLayout() {
    //スプライト倍率に関してはuiscaleFactorとfresolutionScaleFactorの２種類の積が
    // setScaleに使われている。前者はゲームデザイン依存、後者は画面サイズ依存
    //各スプライトサイズ調節
    for (int i = 0; i < 6; i++) {
        timerDigits[i].setScale(uiScaleFactor.x * resolutionScaleFactor, 
            uiScaleFactor.y * resolutionScaleFactor);
    }

    timerCommas[0].setScale(uiScaleFactor.x * resolutionScaleFactor, 
        uiScaleFactor.y * resolutionScaleFactor);
    timerCommas[1].setScale(uiScaleFactor.x * resolutionScaleFactor, 
        uiScaleFactor.y * resolutionScaleFactor);

    // スプライトの位置
    int separationPixels = 2 * resolutionScaleFactor;
    int xSizeSprite = timerDigits[0].getGlobalBounds().width;//１文字分の幅
    //GUI矩形の左上の座標を画面右上に初期設定。
    leftUpCorner = sf::Vector2f(SCREEN_WIDTH * 98 / 100, SCREEN_HEIGHT * 2 / 100);
    //ここから数字６つ、カンマ記号２つ、計８つのスプライトを描画するスペースを取る
    //leftUpCornerからスプライト幅と隙間幅の和の８文字分左にずれたものが
    //最初の文字スプライトの位置になる。
    int x_pos = leftUpCorner.x - 8 * (xSizeSprite + separationPixels);
    //得られた１文字目の左上をleftUpCorner座標とする。
    leftUpCorner = sf::Vector2f(x_pos, leftUpCorner.y);
    int digitIndex = 0;
    for (int i = 0; i < 8; i++) {
        if (i == 2) {  // １つ目のカンマ'
            timerCommas[0].setPosition(x_pos, leftUpCorner.y);
        }
        else if (i == 5) {  // ２つ目のカンマ''
            timerCommas[1].setPosition(x_pos, leftUpCorner.y);
        }
        else {  // 数字
            timerDigits[digitIndex].setPosition(x_pos, leftUpCorner.y);
            digitIndex++;
        }
        //スプライト位置は１文字分＋隙間分ずつ右にずらす
        x_pos += xSizeSprite + separationPixels;
    }
}

void Timer::update(const sf::Time& deltaTime) {
    //レース開始からの経過時間であるcurrentRaceTimeをtimeに受け取って変換して表示
    sf::Time time = StateRace::currentRaceTime;

	long timeAsMilli = time.asMilliseconds();//ミリ秒に変換
	int minutes = timeAsMilli / 60000;//分を計算
	timeAsMilli -= minutes * 60000;//分を引いて残りのミリ秒を計算。200s-60s*3minutes=20sって感じ
	int seconds = timeAsMilli / 1000;//秒を計算
	timeAsMilli -= seconds * 1000;//秒を引いて残りのミリ秒を計算
    //ミリ秒を10で割って2桁表示にする
    //ex)2600-1000*2=600,600/10=60で0.6秒は60と表示される
	int millis = timeAsMilli / 10;

    //各桁のスプライトに入れるテクスチャを設定。23なら10で割った商は2,余りは3という処理
    timerDigits[0].setTexture(digits[minutes / 10]);
    timerDigits[1].setTexture(digits[minutes % 10]);

    timerDigits[2].setTexture(digits[seconds / 10]);
    timerDigits[3].setTexture(digits[seconds % 10]);

    timerDigits[4].setTexture(digits[millis / 10]);
    timerDigits[5].setTexture(digits[millis % 10]);
}

void Timer::draw(sf::RenderTarget& window) {
    //決定された数値、記号スプライトを規定の位置に描画。
    for (int i = 0; i < 6; i++) {
        window.draw(timerDigits[i]);
    }
    window.draw(timerCommas[0]);
    window.draw(timerCommas[1]);
}

void Timer::reset() {
    time = time.Zero;
    //全てを0に
    timerDigits[0].setTexture(digits[0]);
    timerDigits[1].setTexture(digits[0]);
    timerDigits[2].setTexture(digits[0]);
    timerDigits[3].setTexture(digits[0]);
    timerDigits[4].setTexture(digits[0]);
    timerDigits[5].setTexture(digits[0]);
}