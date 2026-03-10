#include "audio/audio.h"

#define _USE_MATH_DEFINES
#include <cmath>

#include "enums.h"

Audio& Audio::getInstance() {
    static Audio instance;  
    return instance;
}

SfxID Audio::loadDing() {
    Audio& instance = Audio::getInstance();
    SfxID ding = SfxID::MENU_INTRO_SCREEN_DING;
    instance.load(ding, "assets/sfx/nintendologo.ogg");
    return ding;
}

void Audio::loadAll() {
    Audio& instance = Audio::getInstance();
    instance.load(MusicID::MENU_TITLE_SCREEN, "assets/music/menu_title_screen.ogg");
    instance.load(MusicID::MENU_PLAYER_CIRCUIT, "assets/music/menu_player_circuit.ogg");
    instance.load(MusicID::CIRCUIT_ANIMATION_START, "assets/music/circuit_opening.ogg");
    instance.load(MusicID::CIRCUIT_PLAYER_WIN, "assets/music/tournament_win.ogg");
    instance.load(MusicID::CIRCUIT_PLAYER_LOSE, "assets/music/tournament_lose.ogg");

    instance.load(SfxID::MENU_SELECTION_ACCEPT, "assets/sfx/menuselect.ogg");
    instance.load(SfxID::MENU_SELECTION_CANCEL, "assets/sfx/menuback.ogg");
    instance.load(SfxID::MENU_SELECTION_MOVE, "assets/sfx/menumove.ogg");


    instance.load(SfxID::CIRCUIT_COLLISION_PIPE, "assets/sfx/thudpipe.ogg");

    instance.load(SfxID::CIRCUIT_GOAL_END, "assets/sfx/goal.ogg");
    instance.load(SfxID::CIRCUIT_END_VICTORY, "assets/sfx/win.ogg");
    instance.load(SfxID::CIRCUIT_END_DEFEAT, "assets/sfx/lose.ogg");

    instance.load(SfxID::CIRCUIT_PLAYER_MOTOR, "assets/sfx/engine.ogg");
    instance.load(SfxID::CIRCUIT_PLAYER_BRAKE, "assets/sfx/brake.ogg");

    instance.load(SfxID::CIRCUIT_COUNTING, "assets/sfx/counting.ogg");
    instance.load(SfxID::CIRCUIT_READYGO, "assets/sfx/readyGo.ogg");

}

//コース選択時に専用bgmを格納。仕様は後のloadを
void Audio::loadCircuit(const std::string& folder) {
    Audio& instance = Audio::getInstance();
    instance.load(MusicID::CIRCUIT_NORMAL, folder + "/music.ogg");
}

//musicのファイル名と、enumで登録したい名前を引数にmusicListに登録
void Audio::load(const MusicID music, const std::string& filename) {
    musicList[(int)music].openFromFile(filename);
}
//sfxのファイル名と、enumで登録したい名前を引数にsfxListに登録
void Audio::load(const SfxID sfx, const std::string& filename) {
    sfxList[(int)sfx].loadFromFile(filename);
}

void Audio::play(const MusicID music, bool loop) {
    Audio& instance = Audio::getInstance();
    //マルチスレッドの場合、lock()以降unlock()まではmusicListに干渉できない
    //music,sfxにまつわる操作はすべてmusicMutexとsfxMutexでlockされるように記述している
    //メインスレッドでplay()を呼び出し、マルチスレッドを生成しplay()を非同期的に呼び出した場合
    //メインでlock()にさしかかったらサブの方は直前で待機する。ということ
    //メインとサブスレッドそれぞれの関数で同じmutexでlockしている場合
    //後から来た方は処理をlockコード直前で止める
    instance.musicMutex.lock();
    for (auto& music : instance.musicList) {
        music.stop();
    }
    instance.musicList[(int)music].play();
    instance.musicList[(int)music].setLoop(loop);
    instance.musicList[(int)music].setVolume(instance.musicVolumePct);
    instance.musicMutex.unlock();
}

void Audio::play(const SfxID sfx, bool loop) {
    Audio& instance = Audio::getInstance();
    if (loop) {
        stop(sfx);
    }
    instance.sfxMutex.lock();
    int i = 0;
    for (int j = 0; j < MAX_SOUNDS; j++) {
        //未使用あるいは最も前に更新されたsf::Soundの添え字を探す
        i = instance.currentSoundIndex++ % MAX_SOUNDS;
        //どんなSoundにせよ、今再生中じゃないなら交換可能なものとして添え字採用
        if (instance.activeSounds[i].getStatus() == sf::SoundSource::Status::Stopped) {
            break;
        }
    }
    instance.sfxMutex.unlock();
    //sfxLastIndexは、自分のsoundBufferがSound配列のいずれかに参照されているか
    //されているなら参照しているSoundの添え字は、を管理している
    //loopするかどうか、がtrueなら長くかかるから登録
    //loopしないならすぐ終わるから登録しない
    instance.sfxLastIndex[(int)sfx] = loop ? i : -1;
    //選んだsoundBufferをsoundにセット
    instance.activeSounds[i].setBuffer(instance.sfxList[(int)sfx]);
    instance.activeSounds[i].play();
    instance.activeSounds[i].setLoop(loop);
    //sf::Soundの標準関数。音は常にリスナーの位置にある。３ｄ設定にしない
    instance.activeSounds[i].setRelativeToListener(true);
    instance.activeSounds[i].setVolume(instance.sfxVolumePct);
}

bool Audio::isPlaying(const SfxID sfx) {
    Audio& instance = Audio::getInstance();
    bool playing;
    instance.sfxMutex.lock();
    int i = instance.sfxLastIndex[(int)sfx];
    if (i >= 0) {
        playing = instance.activeSounds[i].getStatus() == sf::SoundSource::Status::Playing;
    }
    else {
        playing = false;
    }
    instance.sfxMutex.unlock();
    return playing;
}

//指定したsfxにつながるsf::Soundを停止する
void Audio::stop(const SfxID sfx) {
    Audio& instance = Audio::getInstance();
    instance.sfxMutex.lock();
    int i = instance.sfxLastIndex[(int)sfx];
    //sfxLastIndexの当該配列には、もし再生中ならそのSound配列の添え字を補完してある
    //もし無かったら（鳴ってない音を止めようとしたら）-1が返ってくるはず
    if (i >= 0) instance.activeSounds[i].stop();
    instance.sfxMutex.unlock();
}

void Audio::fadeOut(const MusicID music, const sf::Time& deltaTime, const sf::Time& time)
{
    //deltaTimeはフレーム間で代入される経過時間、timeは減衰にかける時間
    Audio& instance = Audio::getInstance();
    //instance.sfxVolumePct / time.asSeconds()は現在の音量を減衰時間で割ったもので、
    //1秒当たりに減る音量割合で、deltaTime.asSeconds()との積は累積の減衰音量分。
    //getVolume
    float volume =
        instance.musicList[(int)music].getVolume() -
        ((instance.sfxVolumePct / time.asSeconds()) * deltaTime.asSeconds());
    volume = fmax(0.0f, volume);//0まで下がって止まる予定で、0を下回らないように
    instance.musicList[(int)music].setVolume(volume);
}

void Audio::pauseMusic() {
    Audio& instance = Audio::getInstance();
    instance.musicMutex.lock();
    for (int i = 0; i < (int)MusicID::__COUNT; i++) {
        //すべてのmusicで再生中のものすべて一時停止する
        if (instance.musicList[i].getStatus() == sf::SoundSource::Status::Playing)
        {
            instance.musicList[i].pause();
        }
    }
    instance.musicMutex.unlock();
}

void Audio::pauseSFX() {
    Audio& instance = Audio::getInstance();
    instance.sfxMutex.lock();
    for (int i = 0; i < MAX_SOUNDS; i++) {
        //Soundにあるsfxのうち、再生中のものをすべて一時停止する
        if (instance.activeSounds[i].getStatus() == sf::SoundSource::Status::Playing)
        {
            instance.activeSounds[i].pause();
        }
    }
    instance.sfxMutex.unlock();
}

void Audio::resumeMusic() {
    Audio& instance = Audio::getInstance();
    instance.musicMutex.lock();
    for (int i = 0; i < (int)MusicID::__COUNT; i++) {
        //すべてのMusicで一時停止中のものは再生
        if (instance.musicList[i].getStatus() == sf::SoundSource::Status::Paused) {
            instance.musicList[i].play();
        }
    }
    instance.musicMutex.unlock();
}

void Audio::resumeSFX() {
    Audio& instance = Audio::getInstance();
    instance.sfxMutex.lock();
    for (int i = 0; i < MAX_SOUNDS; i++) {
        //Sound配列にあるsfxのうち、一時停止中のものをすべて再生
        if (instance.activeSounds[i].getStatus() == sf::SoundSource::Status::Paused)
        {
            instance.activeSounds[i].play();
        }
    }
    instance.sfxMutex.unlock();
}

void Audio::stopSFX() {
    Audio& instance = Audio::getInstance();
    instance.sfxMutex.lock();
    //Sound配列にある全てのsfxを停止する
    for (int i = 0; i < MAX_SOUNDS; i++) instance.activeSounds[i].stop();
    instance.sfxMutex.unlock();
}

void Audio::stopMusic() {
    Audio& instance = Audio::getInstance();
    instance.musicMutex.lock();
    for (int i = 0; i < (int)MusicID::__COUNT; i++) {
        //すべてのMusicを停止
        instance.musicList[i].stop();
    }
    instance.musicMutex.unlock();
}

float Audio::logFunc(const float value) {
    //value*0.9としているのはvalue=1のときに0になり、発散するのを防ぐため
    //valueが大きくなるとlogの中身が小さくなり、logは-1.0を目指して小さくなり、
    //最終的に-logのretは0から1.0を目指して大きくなる。
    //valueに応じてretはなだらかに大きくなり、valueが0.8になるころにretが0.5を迎える
    float ret = -log10f(powf(1 - value * 0.9f, VOLUME_LOG_EXP));
    if (ret > 1.0f) {
        ret = 1.0f;
    }
    return ret;
}

void Audio::setVolume(const float musicVolumeRatio, const float sfxVolumeRatio) {
    Audio& instance = Audio::getInstance();
    //sf::Music標準のsetVolumeに0~100の数値を入れることで音量を変更するが、
    //最大100に対して50を代入すれば感覚的に半分の音量に聞こえるわけではない
    //log対数関数のように、はじめは急激に上昇、あとはなだらかな値変化になる、ゆえに
    //ex)最大の半分の音量は最初の10%,20%あたりで到達している
    //線形的な対応関係ではないために、引数で求める%を別途logFunc関数で変換させる
    //50%ならy50%の音量に聞こえるxを探し出してsetVolumeする

    //入力された音量割合をそのまま保存する。0~1
    instance.userMusicValue = musicVolumeRatio;
    instance.userSFXValue = sfxVolumeRatio;
    //対数関数にかけて適切にしたものを補正して、*100して百分率に補正する
    instance.musicVolumePct = logFunc(musicVolumeRatio) * 100.0f * VOLUME_MULTIPLIER;
    instance.sfxVolumePct = logFunc(sfxVolumeRatio) * 100.0f * VOLUME_MULTIPLIER;
    instance.musicMutex.lock();
    for (int i = 0; i < (int)MusicID::__COUNT; i++) {
        //更新したpctをsetVolumeに代入する。
        //sfxのSoundオブジェクトにはplay時に適用する
        instance.musicList[i].setVolume(instance.musicVolumePct);
    }
    instance.musicMutex.unlock();
}

void Audio::setPitch(const SfxID sfx, const float sfxPitch) {
    Audio& instance = Audio::getInstance();
    //ピッチ:音の高さ。その代わり音の長さが変わる時間軸の拡縮
    int i = instance.sfxLastIndex[(int)sfx];
    instance.activeSounds[i].setPitch(sfxPitch);
}

//レース中に各ドライバーのエンジン音を流す関数
//sf::Listnerはプレイヤーに同行し、プレイヤー含めマップ上のドライバーたちの音を
//キャッチし流す。エンジン配列の添え字のうち、プレイヤーのエンジンのオブジェクトは
//特別扱いなので引数に指定。ifで分岐させている
void Audio::playEngines(unsigned int playerIndex, bool raceMode) {
    Audio& instance = Audio::getInstance();
    instance.playerIndex = playerIndex;
    instance.raceMode = raceMode;
    std::string filename = "assets/sfx/engine.ogg";
    for (unsigned int i = 0; i < DRIVER_NUM; i++) {
        auto& engine = instance.sfxEngines[i];
        //この関数はStartRace,Raceで２度呼ばれる。StartRaceの終わりにstopEnginesを使うことなしに
        //raceModeの引数だけ変えてこの関数を再び呼び出しRaceでのエンジンを演出する。
        //直下のifは、enginesPlayingがtrueのまま、つまりRace直前の利用を目的としている。
        if (instance.enginesPlaying && i != playerIndex) {
            continue;
        }
        if (!instance.enginesPlaying) {
            //playEngineはレース直前に1度だけ呼び出す。その際enginesPlayingをtrueにしておいて
            //レース終了時にfalseにする。
            engine.openFromFile(filename);
            engine.play();
            engine.setLoop(true);
            setEngineVolume(i);
        }

        //距離に応じた聞こえ方の設定
        if (raceMode) {
            //レース中の全ドライバー
            //減衰境界を越えると引数の割合で音量が小さくなっていく
            engine.setAttenuation(0.60f);
            //タイル３枚分の距離までは減衰しない
            //ドライバーたちの距離は0~1で正規化されているので、setMinDistanceの引数も正規化する
            engine.setMinDistance(1.0f / MAP_TILE_NUM_WIDTH * 3.0f);
        }else{
            if (i == playerIndex) {
                //RaceStartのプレイヤー
                engine.setAttenuation(0.60f);
                //タイル３枚分の距離までは減衰しない
                //ドライバーたちの距離は0~1で正規化されているので、setMinDistanceの引数も正規化する
                engine.setMinDistance(1.0f / MAP_TILE_NUM_WIDTH * 3.0f);
            }
            else {
                //RaceStartのプレイヤー以外
                //減衰境界でかなり減衰する
                engine.setAttenuation(0.15f);
                //減衰境界がかなり近い
                engine.setMinDistance(1.0f / MAP_TILE_NUM_WIDTH * 0.15f);
            }
        }
        
    }
    //setRelativeListnerは音源とリスナーの位置関係の設定
    //false:音源とリスナーのそれぞれのsetPositionの関係で音の聞こえる大きさが変わる
    //true:音源のpositionはリスナーの位置を原点にした関係になる。
    //true:音源positionを(100,0,0)すれば、リスナーが移動しても常に（100,0,0）の関係が崩れない
    instance.sfxEngines[playerIndex].setRelativeToListener(raceMode);
    if (raceMode) {
        //レース中はリスナーとプレイヤーエンジンの相対距離が一致。常に最大音量で聞こえる
        //プレイヤー以外はfalse,つまりリスナーと絶対座標を共有し、遠近感のある音源になる
        instance.sfxEngines[playerIndex].setPosition(0.0f, 0.0f, 0.0f);
    }
    instance.enginesPlaying = true;
}

void Audio::playEngines(bool playerOnly) {
    Audio& instance = Audio::getInstance();
    //playginesのラッパー版
    playEngines(instance.playerIndex, playerOnly);
}

void Audio::setEngineVolume(unsigned int i, float volume) {
    Audio& instance = Audio::getInstance();
    //各ドライバーのエンジン音の調整。
    //プレイヤーだけ特別扱い、同じ位置にいても少し大きく聞こえるようにする
    auto& engine = instance.sfxEngines[i];
    if (i == instance.playerIndex) {
        //0.75のほうが若干大きい
        engine.setVolume(instance.sfxVolumePct * 0.75f * volume / 100.0f);
    }
    else {
        engine.setVolume(instance.sfxVolumePct / 1.75f * volume / 100.0f);
    }
}

void Audio::setEnginesVolume(float volume) {
    for (int i = 0; i < DRIVER_NUM; i++) {
        setEngineVolume(i, volume);
    }
}

void Audio::updateEngine(unsigned int i, sf::Vector2f position, float height,
    float speedForward, float speedTurn) {
    Audio& instance = Audio::getInstance();
    //sf::Music,sf::Soundが音を流す役、に対してsf::Listnerが音を聞くオブジェクト。
    //ドライバーの人数分エンジン音を流すMusicオブジェクトはドライバーと同じ座標にある。
    //レース中ドライバーの速度、角速度、座標を元にpitchを設定。
    //さらにsf::Listnerはplayerの座標に設定してあるので
    //playerから離れたドライバーのエンジン音は聞こえないようになる。
    //playerの近くにいるドライバーについて、速度が高ければ、そして直進動作に近ければ
    //エンジン音が早く駆動しているような演出を実装している
    float maxLinearSpeed = 0.4992f / 2.0f;//理論上の最大値をハードコーディングしてる？
    float maxSpeedTurn = 3.6f;//これも？
    float pitch = 1.0f;
    pitch += speedForward / maxLinearSpeed;
    pitch -= fabs(speedTurn) / maxSpeedTurn * 0.65;
    pitch = fmin(pitch, 2.0f);//最低2.0fにとどめる
    if ((unsigned int)i != instance.playerIndex || !instance.raceMode) {
        instance.sfxEngines[i].setPosition(position.x, position.y, 0);
    }
    instance.sfxEngines[i].setPitch(pitch);
}

//player専用のupdateEngine
void Audio::updateEngine(sf::Vector2f position, float height,
    float speedForward, float speedTurn) {
    Audio& instance = Audio::getInstance();
    updateEngine(instance.playerIndex, position, height, speedForward, speedTurn);
}

void Audio::updateListener(sf::Vector2f position, float angle, float height) {
    //sf::Music,sf::Soundが音を流してsf::Listnerが音を聞き、sf::Listerがとらえた音が
    //実際にゲームから聞こえる音になる。
    //これはplayerの位置、正面方向、頭上方向設定と同期される
    sf::Listener::setPosition(position.x, position.y, 0);
    sf::Listener::setDirection(-cosf(angle), -sinf(angle), 0.0f);
    sf::Listener::setUpVector(0.0f, 0.0f, 1.0f);
}

void Audio::pauseEngines() {
    Audio& instance = Audio::getInstance();
    for (int i = 0; i < DRIVER_NUM; i++) {
        instance.sfxEngines[i].pause();
    }
}

void Audio::resumeEngines() {
    Audio& instance = Audio::getInstance();
    for (int i = 0; i < DRIVER_NUM; i++) {
        instance.sfxEngines[i].play();
    }
}

void Audio::stopEngines() {
    Audio& instance = Audio::getInstance();
    //全ドライバーのエンジンを止める。enginesPlayingをfalseに
    for (auto& engine : instance.sfxEngines) {
        engine.stop();
    }
    instance.enginesPlaying = false;
}