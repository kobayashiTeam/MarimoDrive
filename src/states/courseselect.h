#pragma once
#include"statebase.h"
#include<memory>
#include"enums.h"

//タイトル画面、コース選択、フェードまでを管理するステート
class StateCourseSelect : public State {
public:

	enum class CourseSelectPhase :unsigned int {
		FADE_IN,
		SELECTING,
		FADE_OUT,
		STATE_CHANGE
	};
	
	
	unsigned int selectedCourse;//CourseOption型をキャストして初期化する。詳細はenums.h
	CourseSelectPhase currentPhase;
	sf::Time timeSincePhaseChange;//このフェーズに入ってからの経過時間
	sf::Time fade_in_time, fade_out_time;//フェードイン、アウトにかかる時間

	static const sf::Time FADE_IN_TIME;
	static const sf::Time FADE_OUT_TIME;
	//スクリーン上でのコーステキストの絶対座標位置
	static const sf::Vector2f ABS_COURSE_TEXT;
	//複数のテキストを間隔をあけて描画する際、１つをABS_COURSE_TEXTとして指定し、
	//それ以外はREL_COURSE_TEXTを加算したものが文章間の間隔とする
	static const sf::Vector2f REL_COURSE_TEXT;

	StateCourseSelect(Game& _game):State(_game) { init(); }
	void init();
	void handleEvent(const sf::Event& event) override;
	bool update(const sf::Time& deltaTime) override;
	bool fixedUpdate(const sf::Time& fixedDeltaTime) override;
	void draw(sf::RenderTarget& target) override;
	std::string string() const override {
		return "RaceManagerState";
	}

	static void loadBackgroundAssets(const std::string& assetName,
		const sf::IntRect& roiBackground);
	static sf::Texture& getBGTexture();
};