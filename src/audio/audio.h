#pragma once

#include <SFML/Audio.hpp>
#include <mutex>

#include "enums.h"
#include<array>
// 参考:
// https://www.youtube.com/watch?v=AlAmXXNz5ac

#include<SFML/Audio/SoundBuffer.hpp>

//sf::Musicがある以上enumがMusicはややこしい
enum class MusicID : int {
    MENU_TITLE_SCREEN,        // タイトル画面
    MENU_PLAYER_CIRCUIT,      // メニュー画面、プレイヤー選択時
    CIRCUIT_ANIMATION_START,  // スタート
    CIRCUIT_NORMAL,           // 最終以外の周回時
    CIRCUIT_PLAYER_WIN,       // 勝利時
    CIRCUIT_PLAYER_LOSE,      // 敗北時
    __COUNT,
};

enum class SfxID : int {
    MENU_INTRO_SCREEN_DING,    // 起動時タイトルロゴ
    MENU_SELECTION_ACCEPT,     // メニュー画面、決定ボタン
	MENU_SELECTION_CANCEL,     // メニュー画面、キャンセルボタン
	MENU_SELECTION_MOVE,       // メニュー画面、カーソル移動
    CIRCUIT_COLLISION_PIPE,    // パイプ衝突時
    CIRCUIT_PASS_MOTOR,        // played when a player passes you (doppler?)
    CIRCUIT_GOAL_END,     // レース終了時
    CIRCUIT_END_VICTORY,  // レース勝利時
    CIRCUIT_END_DEFEAT,   // レース敗北時
    // ----------------
    CIRCUIT_PLAYER_MOTOR,  // モーター音
    CIRCUIT_PLAYER_MOTOR_SPOOK,  // special motor noise for ghost valley
    CIRCUIT_PLAYER_BRAKE,        // ブレーキ
    // ----------------
// ----------------
// ------------
RESULTS_POINTS_UPDATE,  // リザルト画面のポイント加算

CIRCUIT_COUNTING,
CIRCUIT_READYGO,

__COUNT,
};

//BGM(場面やコースで流す長い音楽),SFX(SE,効果音)を管理するクラス
class Audio {
private:
    static constexpr const float VOLUME_MULTIPLIER = 0.8f;
    static constexpr const float VOLUME_LOG_EXP = 1.0f;  

    //配列を生成し、添え字は上記のenumの名前を使って可読性を上げる。実際の格納はload()で行う
    std::array<sf::Music, (int)MusicID::__COUNT> musicList;
    //sfxはmusicに比べ数が多いので管理、利用に工夫が必要
    // sfxはロード後、sf::SoundBufferオブジェクトに保管され、再生時はsf::Soundオブジェクトに
    //参照される。しかし１度に再生できるSoundの数には限度がある(下記MAX_SOUNDS=32)。
    std::array<sf::SoundBuffer, (int)SfxID::__COUNT> sfxList;
    //sf::SoundBufferの少数がplayingSoundsつまりsf::Soundオブジェクトに参照されるが
    //その際どれが参照されているかを管理するためにsfxLastIndexのそれ。ぞれに
    //「今自分はplayingSoundsのこの添え字要素にある」ことを記録する
    //既にplayingSoundsにあるsfxは必要な時に改めて参照し直す必要がない。
    //特定のsfxを停止したい時にも参照される
    std::array<int, (int)SfxID::__COUNT> sfxLastIndex = { -1 };

    //エンジン用のMusicオブジェクト配列。BGMや一般SFXと別の扱いにする
    //ドライバー8人分あるが、これも変えたほうがいいかも
    std::array<sf::Music, DRIVER_NUM> sfxEngines;
    unsigned int playerIndex = PLAYER_DRIVER_INDEX;
    //エンジン音が流されるRaceStart,Raceのうち、Race中か否かを表すbool
    //Race中でなければドライバー間の複雑なエンジン音制御は不要
    bool raceMode = false;
    bool enginesPlaying = false;

    //排他制御のためのmutexクラスインスタンス
    std::mutex musicMutex, sfxMutex;
    static const int MAX_SOUNDS = 32;//同時に再生できるsfxの数
    //sf::Soundは用意された分偏りなく使いたい。新たなsfxの参照の度に
    //playinSoundsの同じ要素だけ書き換えられるのは勿体ない。
    //ex)currentSoundIndex=3の時、新たなsf::SoundBufferを参照したら、currentSoundIndex++とし
    //空白の要素を参照させるようにする。リングバッファ方式として、要素最大数を越えたら0に戻る
    std::array<sf::Sound, MAX_SOUNDS> activeSounds;
    int currentSoundIndex = 0;
    
    //music,sfxのボリューム設定変数
    float musicVolumePct, sfxVolumePct;
    //ユーザーが指定する音量。0~1
    float userMusicValue, userSFXValue;

    Audio() {
        musicVolumePct = logFunc(0.5f) * 100.0;
        sfxVolumePct = logFunc(0.5f) * 100.0;
        userMusicValue = 0.5f;
        userSFXValue = 0.5f;
    }
    static SfxID loadDing();  // ゲームスタート時の効果音のロード
    static void loadAll();  // 音声ファイルをロード

    static float logFunc(const float value);

    void load(const MusicID music, const std::string& filename);
    void load(const SfxID sfx, const std::string& filename);

    friend class StateInitLoad;

public:
    static Audio& getInstance();

    static void loadCircuit(const std::string& folder);
    static void play(const MusicID music, bool loop = true);
    static void play(const SfxID sfx, bool loop = false);

    static bool isPlaying(const SfxID sfx);//特定のsfxが再生中かどうか

    //指定のMusicの音量を指定した時間timeをかけて減衰させていく処理
    static void fadeOut(const MusicID music, const sf::Time& deltaTime,
        const sf::Time& time = sf::seconds(2.0f));

    //pauseが音楽の一時停止
    static void pauseMusic();
    static void pauseSFX();

    //resumeは音楽の再開
    static void resumeMusic();
    static void resumeSFX();

    //stopは音楽の完全停止
    static void stopSFX();
    static void stop(const SfxID sfx);

    static void stopMusic();

    // 0~1で音量をセット
    static void setVolume(const float musicVolumePct, const float sfxVolumePct);
    static float getMusicVolume() {
        Audio& instance = Audio::getInstance();
        // return instance.musicVolumePct / (100.0f * VOLUME_MULTIPLIER);
        return instance.userMusicValue;
    }
    static float getSfxVolume() {
        Audio& instance = Audio::getInstance();
        // instance.sfxVolumePct / (100.0f * VOLUME_MULTIPLIER);
        return instance.userSFXValue;
    }

    static void setPitch(const SfxID sfx, const float sfxPitch);

    static void playEngines(unsigned int playerIndex, bool raceMode = true);
    static void playEngines(bool playerOnly = false);

    static void setEngineVolume(unsigned int i, float volume = 100.0f);
    static void setEnginesVolume(float volume = 100.0f);

    static void updateEngine(unsigned int i, sf::Vector2f position,
        float height, float speedForward, float speedTurn);
    static void updateEngine(sf::Vector2f position, float height,
        float speedForward, float speedTurn);
    static void updateListener(sf::Vector2f position, float angle,
        float height);
    static void pauseEngines();
    static void resumeEngines();
    static void stopEngines();
};