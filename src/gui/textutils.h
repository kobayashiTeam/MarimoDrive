#pragma once

#include <SFML/Graphics.hpp>
#include <string>
#include<array>

namespace TextColor {
    static const sf::Color Default(81, 142, 225);
    static const sf::Color MenuPrimary(40, 71, 112);
    static const sf::Color MenuPrimaryOnFocus(255, 0, 0);
}  

//文字の描画を担当するクラス
class TextUtils {
public:
    enum class TextAlign : int {
        CENTER,
        LEFT,
        RIGHT,
    };
    enum class TextVerticalAlign : int {
        TOP,
        MIDDLE,
        BOTTOM,
    };
    static constexpr const int CHAR_SIZE = 8;

private:
    //文字（char）は白地(flat)と、影模様付き(shadow)の２種類
    //１枚の文字記号アトラスにはflat,shadowそれぞれ文字と数字と記号で４６文字ある
    static constexpr const int NUM_CHARS = 46;
    std::array<sf::Texture, NUM_CHARS> charactersFlat, charactersShadow;
    //本来の文字が書かれたアトラスと、文字の枠線だけが書かかれ中身が透過されたアトラスがある
    //後者に任意炉の色を加算して枠線を変色させ前者を使った文字に重ねることで
    //枠線だけがカラフルになる文字演出ができるようになる。
    std::array<sf::Texture, NUM_CHARS> charactersFlatAlpha,charactersShadowAlpha;
    static TextUtils& getInstance() {
        static TextUtils instance;
        return instance;
    }

    static const sf::Texture& getChar(const char c, const bool useFlatFont);
    static const sf::Texture& getCharAlpha(const char c, const bool useFlatFont);
    TextUtils() {}

public:
    static void loadAssets(const std::string& assetName,
        const std::string& alphaName,
        const sf::Vector2i& startFlat,
        const sf::Vector2i& startShadow);

    static void write(sf::RenderTarget& window, const std::string& text,
        sf::Vector2f position, const float scale,
        const sf::Color& color = TextColor::Default,
        const bool useFlatFont = true,
        const TextAlign align = TextAlign::LEFT,
        const TextVerticalAlign alignV = TextVerticalAlign::TOP);
};