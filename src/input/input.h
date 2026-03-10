#pragma once

#include <SFML/Graphics.hpp>

enum class Action : int {
    // レース画面
    ACCELERATE,
    BRAKE,
    DRIFT,
    TURN_LEFT,
    TURN_RIGHT,
    ITEM_FRONT,
    ITEM_BACK,
    // メニュー画面
    PAUSE,
    ACCEPT,
    CANCEL,
    MENU_UP,
    MENU_DOWN,
    MENU_LEFT,
    MENU_RIGHT,
    __COUNT
};
//sf;;Keyboard::Keyとenumで用意したkeyは異なるので名前が同じなのはややこしいのではないか？
//キー入力を管理するクラス
class Input {
private:
    static Input instance;
    //sf::keyboard::keyというenum型がsfmlに用意されている。
    //同型の配列を用意し、インデックスを自前に用意し、対応する
    //sf::keyboard::の列挙子内容を格納する
    sf::Keyboard::Key map[(int)Action::__COUNT];
	//map[Key::MENU_UP]=sf::Keyboard::Upのように、Keyとsf::Keyboard::Keyを紐づける。
    //使用するキーの分だけ要素数を用意する
    const sf::RenderWindow* gameWindow;

    Input();

public:
	// Gameクラスのwindowをセットする
    static inline void setGameWindow(const sf::RenderWindow& window) {
        instance.gameWindow = &window;
    }

    // sfmlの物理キーと、自前で用意したKeyを紐づける。
	//sfml::Keyboard::Upを直接コード上で使うよりも、Key::MENU_UPを使う方が
    // stateや用途に合わせて変名して使えるのでコードの可読性が上がる。
    static inline void set(const Action action, const sf::Keyboard::Key code) {
        instance.map[(int)action] = code;
    }
    static inline const sf::Keyboard::Key& get(Action action) {
        return instance.map[(int)action];
    }

    // 引数のキーを押したか、離したか、押しっぱなしかを知る
    //pressed,releasedはメニュー画面などで使う、sf::eventを毎フレームpollし、
    //キー入力を「押しているか？その物理キーのコードはどれか？」を判定する。
	//heldはレース画面などで使う、sfmlの機能で別経路でOSにキー入力を問い合わせ
    // キー押下状態を確認する。
    static inline bool pressed(const Action action, const sf::Event& event) {
        return event.type == sf::Event::KeyPressed &&  event.key.code == get(action);
    }
    static inline bool released(const Action action, const sf::Event& event) {
        return event.type == sf::Event::KeyReleased &&  event.key.code == get(action);
    }
    static inline bool held(const Action action) {
        return sf::Keyboard::isKeyPressed(get(action)) && instance.gameWindow->hasFocus();
    }

    static std::string getActionName(const Action action);
    //押されたキーに割り当てられたアクションを文字列で返す
    // ex)EnterをKey::"ACCEPT"に割り当てた後、キーボードでEnterを押すと、"accept"と表示される。
    
    // 参考
    // https://en.sfml-dev.org/forums/index.php?topic=15226.0
	// キーボード上で押されたキーが何かを文字列で返す
    static std::string getKeyCodeName(const sf::Keyboard::Key code);
};
