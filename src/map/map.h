#pragma once

#include <string>
#include <SFML/Graphics.hpp>
#include"enums.h"
#include<array>
#include<memory>
#include<vector>

class WallObject;
typedef std::shared_ptr<WallObject> WallObjectPtr;

//マップのロードや描画にまつわるクラス
class Map {
private:
	static Map instance;
	std::string courseName;
	//マップ画像は透視変換など処理を加えるためram上でImage型で保持する
	//マップ本体の画像、遠景の奥の空の画像、遠景の岩などのオブジェクトの画像
	//assetEdgesはマップの外側に広がる景色画像
	sf::Image assetCourse, assetSkyBack, assetSkyFront, assetEdges;
	sf::Image assetObjects;
	sf::Image assetMinimap;//ウインドウ下部に描画されるミニマップ画像

	//マップを縦横に埋めるタイルの2次元配列。それぞれ通常エリア、遅延エリアなどの
	//属性を持つ。詳細はenums.h
	MapLandMatrix landTiles;
	std::vector<WallObjectPtr> wallObjects;//障害物インスタンスを管理するポインタの配列

	sf::FloatRect stretchedGoal;  // 道幅いっぱいのゴールラインのrect
	sf::FloatRect centeredGoal;   // ゴールラインオブジェクトがある座標、サイズのrect
	int aiFarVision;

public:
	//courseSelectにおいて選択されたコースファイルをロードする
	static bool loadCourse(const std::string& course);
	//レース中、遠景Imageを加工してtextureに変換する。
	static void skyImageToTextures(const DriverPtr& player, sf::Texture& skyBack,
		sf::Texture& skyFront);
	//レース中、マップImageを加工してtextureに変換する。
	static void minimapImageToTexture(sf::Texture& minimapTexture);
	static void generateMinimap();
	//マップの透視変換処理。
	const sf::Image mode7(const sf::Vector2f& position, const float angle,
		const float fovHalf, const float clipNear,
		const float clipFar, const sf::Vector2u& size,
		const bool perspective);
	//マップ画像のuv座標を引数に、その位置のピクセルの色を返す
	const sf::Color sampleMap(const sf::Vector2f& sample);
	sf::Color sampleAsset(const sf::Image& asset, const sf::Vector2f& sample);
	//レース中、マップImageを加工してtextureに変換する。
	static void courseImageToTexture(DriverPtr player, sf::Texture& courseTexture);

	static void loadAI();//コースごとに決まっているAIの認識タイル深度をロードする
	static std::string getCourseName() { return instance.courseName; }//コース名を取得
	//getDrawablesメソッドに障害物やドライバー達各々のスプライトの位置、スケールを決めたものを
	// スプライトとカメラ深度のペア配列に加えられ、まとめて描画される
	static void getWallDrawables(
		const sf::RenderTarget& window, const DriverPtr& player,
		const float screenScale, std::vector<std::pair<float, sf::Sprite*>>& drawables);
	static void getDriverDrawables(
		const sf::RenderTarget& window, const DriverPtr& player,
		const DriverArray& drivers, const float screenScale,
		std::vector<std::pair<float, sf::Sprite*>>& drawables);
	//マップ正規化座標に変換されているドライバー座標をスクリーン座標に変換する
	static bool mapToScreen(const DriverPtr& player, const sf::Vector2f& mapCoords,
		sf::Vector2f& screenCoords, float& z);
	//マップの正規化座標を受け取って、その位置のタイルの属性を返す
	static inline MapLand getLand(const sf::Vector2f& position) {
		return instance.landTiles[int(position.y * MAP_TILE_NUM_HEIGHT)]
			[int(position.x * MAP_TILE_NUM_WIDTH)];
	}
	//ロードしたAIの認識タイル深度を参照する
	static int getAIFarVision() { return instance.aiFarVision; }
	//マップ座標系に置かれたドライバーの位置をミニマップ座標系に変換する
	static sf::Vector2f mapCoordinates(sf::Vector2f& position);

};