#include "textutils.h"


#include <iostream>

//文字、記号アトラスから文字を切り出してテクスチャ配列に格納していく
void TextUtils::loadAssets(const std::string& assetName,
    const std::string& alphaName,
    const sf::Vector2i& startFlat, const sf::Vector2i& startShadow) {
    static const sf::Vector2i size(8, 8);//アトラスの１文字のサイズ
    sf::Image raw, alpha;
    raw.loadFromFile(assetName);
    alpha.loadFromFile(alphaName);
    //アトラスの上の方にflat(白地)、下の方に影模様つきの文字があるので
    //切り出すスタート位置を別々に代入して使う
    sf::Vector2i posFlat = startFlat;
    sf::Vector2i posShadow = startShadow;
    for (int i = 1; i <= NUM_CHARS; i++) {
        getInstance().charactersFlat[i - 1].loadFromImage(
            raw, sf::IntRect(posFlat, size));
        getInstance().charactersFlatAlpha[i - 1].loadFromImage(
            alpha, sf::IntRect(posFlat, size));
        getInstance().charactersShadow[i - 1].loadFromImage(
            raw, sf::IntRect(posShadow, size));
        getInstance().charactersShadowAlpha[i - 1].loadFromImage(
            alpha, sf::IntRect(posShadow, size));
        //文字間の隙間paddingが1pxなのでそれも追加し、切り出し対象領域をずらして次ループへ
        posFlat += sf::Vector2i(size.x + 1, 0);
        posShadow += sf::Vector2i(size.x + 1, 0);
        //文字はA-Zまで２行に分かれている。１行目、２行目の最後の文字（i==13,i==26）
        if (i % 13 == 0 && i < 27) {
            //少しややこしい表現だが、ここを通るときはi/13が1,2のいずれか
            //つまりstartFlat(１行目の高さ)から２行目にうつるときは「文字の縦サイズ＋隙間」*1,
            //1行目の高さから３行目にうつるときは「文字の縦サイズ＋隙間」*2,を加算する
            int movement = (size.y + 1) * i / 13;
            posFlat = startFlat + sf::Vector2i(0, movement);
            posShadow = startShadow + sf::Vector2i(0, movement);
        }
    }
}

int getCharIndex(char c) {
    int i;
    //ASCIIコードの並びを利用して、aを0としたときの引数文字の相対インデックス
    if (c >= 'a' && c <= 'z') {
        i = c - 'a';
    }
    //あるいは数字かも
    else if (c >= '0' && c <= '9') {
        i = 26 + c - '0';
    }
    else {
        switch (c) {//連続的に表現できない記号などは個別に指定
        case '.':
            i = 36;
            break;
        case '(':
            i = 37;
            break;
        case ')':
            i = 38;
            break;
        case '?':
            i = 39;
            break;
        case '!':
            i = 40;
            break;
        case '\'':
            i = 41;
            break;
        case '"':
            i = 42;
            break;
        case '-':
            i = 43;
            break;
        case '>':
            i = 44;
            break;
        case '<':
            i = 45;
            break;
        default:
            i = 39;  // ?
            break;
        }
    }
    return i;
}
const sf::Texture& TextUtils::getChar(const char c, const bool useFlatFont) {
    //characterテクスチャ配列にはaを０番目、zを添え字の26-1=25番目に格納された。
    //getCharIndex()の操作によって入力文字のaは0という相対コードが返されることになり
    //適切にaのテクスチャが指定される
    int i = getCharIndex(c);
    return useFlatFont ? getInstance().charactersFlat[i]
        : getInstance().charactersShadow[i];
}

const sf::Texture& TextUtils::getCharAlpha(const char c,
    const bool useFlatFont) {
    int i = getCharIndex(c);
    return useFlatFont ? getInstance().charactersFlatAlpha[i]
        : getInstance().charactersShadowAlpha[i];
}

//文字列を描画する本体の関数
void TextUtils::write(sf::RenderTarget& window, const std::string& text,
    sf::Vector2f position, const float scale,
    const sf::Color& color, const bool useFlatFont,
    const TextAlign align, const TextVerticalAlign alignV) {
    //文字列を描画する上で１文字間の間隔をdeltaに定義
    static const sf::Vector2f delta( getInstance().charactersFlat[0].getSize().x + 1.0f, 0.0f);

    //文字列全体の横幅サイズ。文字数＊（１文字サイズ＊画面サイズ依存のスケール）
    float textWidth = delta.x * scale * text.size();
    //高さも同様。文字の縦には隙間はいらない
    float textHeight = getInstance().charactersFlat[0].getSize().y * scale;
    //文字列の左、中央右揃え、上中下揃えを引数で選択する
    //画面上にpositionを設定したうえで、そのpositionが「文字列の左端か、中央化、右端か」選択する
    if (align == TextAlign::LEFT) {
        // デフォルト
    }
    else if (align == TextAlign::CENTER) {
        //ex)画面中央に文字列の中心が重なるように描画したければ
        //positionを画面中央にしたうえで、さらにpositionを描画領域幅の半分だけ左にずらせば
        //描画領域幅の真ん中が画面中央に来るようになる
        position.x -= textWidth / 2.0f;
    }
    else if (align == TextAlign::RIGHT) {
        position.x -= textWidth;
    }

    if (alignV == TextVerticalAlign::TOP) {
        // デフォルト
    }
    else if (alignV == TextVerticalAlign::MIDDLE) {
        //CENTER同様、positionを画面中央にしたうえで、そのままでは文字の上端が画面中央。
        //文字の縦サイズの半分だけ上にずらす（引く）ことで文字の中央が画面の中央になる
        position.y -= textHeight / 2.0f;
    }
    else if (alignV == TextVerticalAlign::BOTTOM) {
        position.y -= textHeight;
    }

    //入力文字列の１文字ずつについて
    for (const char c : text) {
        if (c != ' ') {
            //文字->相対コード->対応テクスチャに変換、スプライトに設定
            sf::Sprite sprite(getChar(c, useFlatFont));
            sprite.setPosition(sf::Vector2f(position));
            sprite.setScale(scale, scale);
            window.draw(sprite);
            //白地の文字を描画したうえで、同じ部分に枠線だけの文字を描画する。
            //引数にしたcolorを加算することで任意の枠線の文字にできる
            sprite.setTexture(getCharAlpha(c, useFlatFont));
            sprite.setColor(color);
            window.draw(sprite);
        }
        position += delta * scale;
    }
}