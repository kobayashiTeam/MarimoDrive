#include"map.h"
#include<fstream>
#include<iostream>
#include"enums.h"
#include"entities/driver.h"
#include"ai/gradientdescent.h"
#include"entities/wakame.h"
#include"entities/sango.h"
#include"entities/hitode.h"
#include"entities/collisionhashmap.h"

Map Map::instance;

bool Map::loadCourse(const std::string& course) {
    instance.courseName = course;

	//マップクラスが持つ変数のそれぞれに指定ファイルからコースの情報を読み込む
    //各コースは固有名のフォルダに分け中身のファイル名は揃えているので、
	// 下記の書き方でそれぞれのフォルダから役割の同じファイルを読み込むことができる
    //std::ifstreamでファイルの存在を確認。
	std::ifstream inCourse(course + "/base.png");
    std::ifstream inSkyBack(course + "/sky_back.png");
    std::ifstream inSkyFront(course + "/sky_front.png");
    std::ifstream inEdges(course + "/edge.png");
    std::ifstream inTileMap(course + "/base.txt");
    std::ifstream inObjects(course + "/objects.txt");
    if (!inCourse.is_open()|| !inSkyBack.is_open()|| !inSkyFront.is_open()||
        !inEdges.is_open()||!inTileMap.is_open()||!inObjects.is_open()) {
        std::cerr << "ERROR: Can't find files for '" << course << "'"
            << std::endl;
        return false;
    }
    //この後ストリーム的に部分的に情報を読み込むinTilemapとinObjects以外は
    //存在確認が済んだら閉じる。改めて直接のファイル指定でデータを読み込む
	inCourse.close();
    inSkyBack.close();
    inSkyFront.close();
	inEdges.close();

    instance.assetCourse.loadFromFile(course + "/base.png");
    instance.assetObjects.loadFromFile(course + "/base.png");
    instance.assetSkyBack.loadFromFile(course + "/sky_back.png");
    instance.assetSkyFront.loadFromFile(course+ "/sky_front.png");
	instance.assetEdges.loadFromFile(course + "/edge.png");

    //タイル情報を格納
    for (int y = 0; y < MAP_TILE_NUM_HEIGHT; y++) {
        char landChar;
        for (int x = 0; x < MAP_TILE_NUM_WIDTH; x++) {
            inTileMap >> landChar;
            instance.landTiles[y][x] = MapLand(landChar - '0');
        }
    }

    // ゴールポジション
    float meta_x, meta_y, meta_w, meta_h;
    inObjects >> meta_x >> meta_y >> meta_w >> meta_h;
    instance.stretchedGoal =
        sf::FloatRect(meta_x / MAP_ASSET_WIDTH, meta_y / MAP_ASSET_HEIGHT,
            meta_w / MAP_ASSET_WIDTH, meta_h / MAP_ASSET_HEIGHT);
    inObjects >> meta_x >> meta_y >> meta_w >> meta_h;
    instance.centeredGoal =
        sf::FloatRect(meta_x / MAP_ASSET_WIDTH, meta_y / MAP_ASSET_HEIGHT,
            meta_w / MAP_ASSET_WIDTH, meta_h / MAP_ASSET_HEIGHT);

	// AI関連:コースごとのAIの視界の深さ
    inObjects >> instance.aiFarVision;

    //ミニマップ
    // スクリーンの上画面でプレイヤーを描画する技術とミニマップの描画技術は
    // 共通のmode7関数を使っている。
    //レース中のプレイヤーの後ろからカメラが追従するような透視変換加工した下マップ視点で、
    //ミニマップの方は巨大な視錐台でマップの端から透視変換無しで固定位置、角度から
    //同じ状態のマップを描画する処理になっている
	generateMinimap();

    // 障害物
	int objectsNum;
    instance.wallObjects.clear();
    inObjects >> objectsNum;
    instance.wallObjects.resize(objectsNum);
    for (int i = 0; i < objectsNum; i++) {
        // タイプ、座標をファイルから読み取る
        int typeId;
        float centerX, centerY;
        inObjects >> typeId >> centerX >> centerY;
        // 障害物生成
        WallObjectPtr ptr;
        sf::Vector2f pos(centerX, centerY);
        switch (WallObjectType(typeId)) {
        case WallObjectType::WAKAME:
            ptr = WallObjectPtr(new Wakame(pos));
            break;
        case WallObjectType::SANGO:
            ptr = WallObjectPtr(new Sango(pos));
            break;
        case WallObjectType::HITODE:
            ptr = WallObjectPtr(new Hitode(pos));
            break;
        default:
            std::cerr << "ERROR: Invalid wall object type (" << typeId
                << ")" << std::endl;
            break;
        }
		// 生成物をマップの障害物配列に追加
        instance.wallObjects[i] = ptr;
    }

    // 衝突設定
    CollisionHashMap::resetStatic();
    CollisionHashMap::resetDynamic();
    for (const WallObjectPtr& wallObject : instance.wallObjects) {
        CollisionHashMap::registerStatic(wallObject);
    }

    //ファイルを閉じる
    inTileMap.close();
    inObjects.close();

	return true;
}

void Map::minimapImageToTexture(sf::Texture& minimapTexture) {
    //ミニマップの描画はloadCourseで一度つくったものをそれ以上加工する必要が無いので
	//Imageとして作っておいたものをTextureに変換するだけでいい
	minimapTexture.loadFromImage(instance.assetMinimap);
}

void Map::generateMinimap() {
    //mode7の平行投影処理を利用して、マップ画像をミニマップとして変形する
    sf::Vector2u windowSize(SCREEN_WIDTH,SCREEN_HEIGHT);
    //マップ画像からy座標プラス方向に遠く離れた位置から狭い視野角度でマップ画像領域を覆う
    //広大な視錐台をイメージする
    //minはカメラからマップ画像の下端辺
    //maxはカメラからマップ画像の上端辺
    float min = MINIMAP_POS_DISTANCE - 1.0f - 0.05f;
    float max = MINIMAP_POS_DISTANCE + 0.15f;

    instance.assetMinimap = instance.mode7(
        sf::Vector2f(0.5f, MINIMAP_POS_DISTANCE),
        3.0f * M_PI_2,  MINIMAP_FOV_HALF,
        //以下の２行で、カメラから視錐台の上底と下底までの距離を代入する
        //1「カメラ」2「カメラからマップ画像下端辺への垂線」3「カメラからマップ下端辺の左端」
        //でできる三角形を考えたとき、2がmin,3がmin / cosf(MINIMAP_FOV_HALF)
        min / cosf(MINIMAP_FOV_HALF),
        max / cosf(MINIMAP_FOV_HALF), 
        sf::Vector2u(windowSize.x, windowSize.y * MINIMAP_HEIGHT_PCT),
        false);  //透視変換無し

}


const sf::Image Map::mode7(const sf::Vector2f& position, const float angle,
    const float fovHalf, const float clipNear,const float clipFar, 
    const sf::Vector2u& size,    const bool perspective)
{
	//カメラを上底の中心とした、台形型の視界（視錐台）を定義する
    //カメラから上底二頂点への単位ベクトルを求める
    float fovMin = angle - fovHalf;
    float fovMax = angle + fovHalf;
    sf::Vector2f trigMin = sf::Vector2f(cosf(fovMin), sinf(fovMin));
    sf::Vector2f trigMax = sf::Vector2f(cosf(fovMax), sinf(fovMax));

	// 視界（視錐台）の四頂点の座標を求める
    sf::Vector2f far1 = position + trigMin * clipFar;
    sf::Vector2f far2 = position + trigMax * clipFar;
    sf::Vector2f near1 = position + trigMin * clipNear;
    sf::Vector2f near2 = position + trigMax * clipNear;

    // create image and sample pixels from original course
	//mapImageはウインドウ中のサーキット描画領域のサイズ
    sf::Image mapImage;
    float width = size.x;
    float mapHeight = size.y;
    mapImage.create(width, mapHeight);
    for (int y = 1; y < mapHeight; y++) {
        // yは台形（視錐台）の輪郭と内部にある座標のy座標にあたる
		//ループの対象は視界の下底（台の奥）から上底（台の手前）まで
        float sampleDepth = y / mapHeight;
        sf::Vector2f start, end;
        //透視変換あり。視錐台は双曲線的なスケーリングにあい、
		//マップ画像のうちカメラに近い部分を密に、遠い部分を疎にサンプリングする
        if (perspective) {
            start = near1 + (far1 - near1) / sampleDepth;
            end = near2 + (far2 - near2) / sampleDepth;
        }
		//透視変換なし。視錐台は変形無し、その内側にあるマップ画像の座標をサンプリングする
        else {
            start = near1 + (far1 - near1) * (1.0f - sampleDepth);
            end = near2 + (far2 - near2) * (1.0f - sampleDepth);
        }

        for (int x = 0; x < width; x++) {
			// xは台形（視錐台）の輪郭と内部にある座標のx座標にあたる
			//ループ対象は視界の左端から右端まで
            float sampleWidth = x / width;
            sf::Vector2f sample = start + (end - start) * sampleWidth;

            sf::Color color;
            //視錐台内のの対象座標（sample）がマップ画像の内側に収まっているとき、
			//マップ画像の対応する座標の色をサンプリングする
            if (sample.x >= 0.0f && sample.x <= 1.0f &&
                sample.y >= 0.0f && sample.y <= 1.0f) {
                color = sampleMap(sample);
            }
			//視錐台内の対象座標（sample）がマップ画像の外側にあるとき、
            // マップの外側を担当する「エッジ画像」の対応する座標の色をサンプリングする
            else {
                //外側用のエッジ画像はマップ画像に対してとても小さい。
                //マップ画像の外側にエッジ画像が大量に敷き詰められているものとする
				//その際sampleがエッジ画像のuv座標系でどこにあたるかを求める
                sample.x =
                    fmodf(sample.x, MAP_EDGES_SIZE / (float)MAP_ASSET_WIDTH);
                sample.y =
                    fmodf(sample.y, MAP_EDGES_SIZE / (float)MAP_ASSET_HEIGHT);
				//uv座標の0~1の範囲に収まるようにする
                if (sample.x < 0.0f)
                    sample.x += MAP_EDGES_SIZE / (float)MAP_ASSET_WIDTH;
                if (sample.y < 0.0f)
                    sample.y += MAP_EDGES_SIZE / (float)MAP_ASSET_HEIGHT;
				//マップ画像におけるuv座標にマップ画像サイズとの積をとることで絶対座標に変換する
				//と同時にエッジ画像サイズで割ることでエッジ画像におけるuv座標に変換する
                sample.x *= MAP_ASSET_WIDTH / (float)MAP_EDGES_SIZE;
                sample.y *= MAP_ASSET_HEIGHT / (float)MAP_EDGES_SIZE;
				//エッジ画像からuv座標にある色をサンプリングする
                color = sampleAsset(instance.assetEdges, sample);
            }
            mapImage.setPixel(x, y, color);
        }
    }


    return mapImage;
}


const sf::Color Map::sampleMap(const sf::Vector2f& sample) {
    return sampleAsset(instance.assetObjects, sample);
}


sf::Color Map::sampleAsset(const sf::Image& asset,const sf::Vector2f& sample)
{
    //assetは色情報をサンプリングしたいマップ画像orエッジ画像
	//sampleはマップorエッジ画像のuv座標
    sf::Vector2u size = asset.getSize();
	//(sample.x * size.x)はマップorエッジ画像における絶対ピクセル座標
	//ただし画像がもつ座標の幅は0~(size.x-1)であることに注意。asset.getSize()は含まない
    unsigned int px =
        std::min(asset.getSize().x - 1, (unsigned int)(sample.x * size.x));
    unsigned int py =
        std::min(asset.getSize().y - 1, (unsigned int)(sample.y * size.y));
    return asset.getPixel(px, py);
}

void Map::courseImageToTexture(DriverPtr player,sf::Texture& courseTexture) {

    //プレイヤーもカメラもマップ画像の上にいる
	//その座標はマップ画像サイズ縦横を1.0fとしたときの割合で表される
        sf::Vector2f cameraPosition;
        cameraPosition.x =
            player->position.x -
            cosf(player->posAngle) * (CAM_TO_PLAYER_DST / MAP_ASSET_WIDTH);
        cameraPosition.y =
            player->position.y -
            sinf(player->posAngle) * (CAM_TO_PLAYER_DST / MAP_ASSET_HEIGHT);

		sf::Vector2u windowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
        
		//ゲームウインドウに対するコース描画領域のサイズを求める
        sf::Vector2u circuitSize =
            sf::Vector2u(windowSize.x, windowSize.y * CIRCUIT_HEIGHT_PCT);
        //mode7関数で、mapクラスに登録されたマップ画像をもとに透視変換を施したものを
        //変換済みコース画像を取得する。ピクセル加工はRAM上で行うので
		//sf::Image型で受け取る。
        sf::Image mapImage =
            instance.mode7(cameraPosition, player->posAngle, MODE7_FOV_HALF,
                MODE7_CLIP_NEAR, MODE7_CLIP_FAR, circuitSize, true);

        courseTexture.create(circuitSize.x, circuitSize.y);
        courseTexture.loadFromImage(mapImage);
}

void Map::loadAI() {
    AIGradientDescent::loadOrCreateGradient(instance.landTiles,
        instance.stretchedGoal);
}


void Map::getWallDrawables(
    const sf::RenderTarget& window, const DriverPtr& player,const float screenScale,
    std::vector<std::pair<float, sf::Sprite*>>& drawables)
{
    sf::Vector2u windowSize = window.getSize();
    for (const WallObjectPtr& object : instance.wallObjects) {
		//radious:長さはオブジェクトの視覚半径、方向はプレイヤーからオブジェクトへ
        //プレイヤーから見るオブジェクトの位置はオブジェクトの中心座標ではなく
		//オブジェクトの半径分プレイヤーに近い位置にある
        sf::Vector2f radius =
            sf::Vector2f(cosf(player->posAngle), sinf(player->posAngle)) * object->visualRadius;
        sf::Vector2f screen;
        //zは各オブジェクトが持つ描画深度。カメラとの位置関係から求め、小さい数値程
		//プレイヤーに近い位置にあり、手前に描画される。
        float z;
		//オブジェクトの位置はマップ画像の左上から右下を0.0f~1.0fの範囲で表した座標系で表されている。
        //そしてマップ画像が描画されるスクリーンも独自の座標系を持つ。
        //マップ画像座標（mapCoords）で表された座標をスクリーン座標（screenCoords）に変換する。
        if (Map::mapToScreen(player, object->position - radius, screen, z)) {
            sf::Sprite& sprite = object->getSprite();
            //マップ画像に用いたサイズはウインドウサイズと同じであるが、
            //マップ描画範囲はそのうちの一部（CIRCUIT_HEIGHT_PCT）なので、その分縮小する。
            sprite.setScale(CIRCUIT_HEIGHT_PCT, CIRCUIT_HEIGHT_PCT);
            //求めた正規化スクリーンuv座標を絶対座標に変換する
            screen.x *= windowSize.x;
            screen.y *= windowSize.y * CIRCUIT_HEIGHT_PCT;
            screen.y += windowSize.y * SKY_HEIGHT_PCT;
			//奥行きzに応じてスケーリングする。線形的ではなく、双曲線的なスケーリングをすることで、
            // 遠くのオブジェクトが小さくなりすぎないようにする。単純なy=1/xでは急激になる
            //勾配の緩いlogとアーティスティック関数を用いる
            float scale = 1.0f / (3.6f * logf(1.02f + 0.8f * z));
            sprite.scale(scale * screenScale, scale * screenScale);
            sprite.setPosition(screen);
            drawables.push_back(std::make_pair(z, &sprite));
        }
    }
}


bool Map::mapToScreen(const DriverPtr& player, const sf::Vector2f& mapCoords,
    sf::Vector2f& screenCoords, float& z) {
    //カメラはプレイヤーの後方、posAngleの反対側に追従する。
	//カメラ座標をマップ画像サイズ縦横を1.0fとしたときの割合で表す
    sf::Vector2f cameraPosition;
    cameraPosition.x =
        player->position.x -
        cosf(player->posAngle) * (CAM_TO_PLAYER_DST / MAP_ASSET_WIDTH);
    cameraPosition.y =
        player->position.y -
        sinf(player->posAngle) * (CAM_TO_PLAYER_DST / MAP_ASSET_HEIGHT);

	// relPointはカメラから見たオブジェクトの位置を表すベクトル
	//ここでいうオブジェクトの位置は、オブジェクトの中心座標ではなく、
    // オブジェクトの半径分プレイヤーに近い位置、つまり実際に見える表面の座標。
    sf::Vector2f relPoint = mapCoords - cameraPosition;
    //relPointMod:relPointの長さ。
    float relPointMod =
        sqrtf(relPoint.x * relPoint.x + relPoint.y * relPoint.y);
    // forward:プレイヤーの向きの単位ベクトル
    sf::Vector2f forward =
        sf::Vector2f(cosf(player->posAngle), sinf(player->posAngle));
    // カメラから見たときのオブジェクトの距離z
	//そしてスクリーン描画時におけるオブジェクトの座標x,yを求めている
    float cosFov = cosf(MODE7_FOV_HALF);
    float sinFov = sqrtf(1.0f - cosFov * cosFov);
    //z:オブジェクトの座標をカメラの視線方向に射影した上での大きさの値。
    z = (forward.x * relPoint.x + forward.y * relPoint.y);
    //カメラ、オブジェクト、オブジェクトからカメラの視線方向へ卸した垂線の交点による三角形を考える
    //cos:カメラからオブジェクトを斜辺とした場合の余弦。
    float cos = z / relPointMod;
    // sin:この正弦はスクリーンにおいてオブジェクトがx座標にどれだけずれて描画されるかに関係する。
    //外積により求められる。
    float sin = (forward.x * relPoint.y - forward.y * relPoint.x) / relPointMod;
    
    float y = MODE7_CLIP_FAR * cosf(MODE7_FOV_HALF) / (relPointMod * cos);
    //スクリーン座標は左端から右端まで0~1で表される。
    //視界のx成分はsinFov*2,そしてオブジェクトの位置はsinであり、その割合で表すことができる
	//さらに視界の中心が0.5なので、sinFovを足している。 オブジェクトが視界の左側にあるとsinは
	//負の値になり、右側にあると正の値になる
    float x = (sin + sinFov) / (2.0f * sinFov);

    screenCoords = sf::Vector2f(x, y);
    return x >= -0.1f && x < 1.1f && y >= 0.0f && y <= 2.0f;//2.0f
}


void Map::getDriverDrawables(
    const sf::RenderTarget& window, const DriverPtr& player,
    const DriverArray& drivers, const float screenScale,
    std::vector<std::pair<float, sf::Sprite*>>& drawables) {
    sf::Vector2u windowSize = window.getSize();
    for (const DriverPtr& object : drivers) {
        if (object == player) {//ドライバーでもプレイヤーは別の処理で描画するので省く
            continue;
        }
        sf::Vector2f radius =
            sf::Vector2f(cosf(player->posAngle), sinf(player->posAngle)) *
            object->visualRadius;
        sf::Vector2f screen;
        float z;
        //マップ内正規化座標におけるドライバーたちの座標をスクリーン座標に変換。
		//wallObjectの描画と同様、ドライバーの描画位置もオブジェクトの半径分プレイヤーに近くになる。
        if (Map::mapToScreen(player, object->position - radius, screen, z)) {
            float driverAngle = player->posAngle - object->posAngle;
            object->animator.setViewSprite(driverAngle);
            sf::Sprite& sprite = object->getSprite();
            screen.x *= windowSize.x;
            screen.y *= windowSize.y * CIRCUIT_HEIGHT_PCT;
            screen.y += windowSize.y * SKY_HEIGHT_PCT;
			//運転中は走行演出でy座標が上下に揺れる。詳しくはdriveranimator.cpp
            screen.y -= object->animator.spriteMovementSpeed;
            float scale = 1.0f / (3.6f * logf(1.02f + 0.8f * z));


            sprite.scale(scale * screenScale, scale * screenScale);
            sprite.setPosition(screen);
            drawables.push_back(std::make_pair(z, &sprite));
        }
    }
}


void Map::skyImageToTextures(const DriverPtr& player, sf::Texture& skyBack, sf::Texture& skyFront) {
    sf::Vector2u windowSize(SCREEN_WIDTH, SCREEN_HEIGHT);

    // 1. 画像内での「使用する高さ」を直接決定 (下側62.5%を使用)
    sf::Vector2u skyBackSize = instance.assetSkyBack.getSize();
    skyBackSize.x /= 2; // ループの1周期分
    float targetImgHeight = skyBackSize.y * 0.625f;
    int backCutY = skyBackSize.y - (int)targetImgHeight;

    sf::Vector2u skyFrontSize = instance.assetSkyFront.getSize();
    skyFrontSize.x /= 2; // ループの1周期分
    int frontCutY = skyFrontSize.y - (int)targetImgHeight;

    // 2. 画面の横幅(windowSize.x)が、画像内の何ピクセル分に相当するかを計算
    // 画面の空の高さ(windowSize.y * SKY_HEIGHT_PCT)が、targetImgHeight になる比率を使う
    float screenToImageRatio = targetImgHeight / (windowSize.y * SKY_HEIGHT_PCT);
    float imgSampleWidth = windowSize.x * screenToImageRatio;

    // 3. プレイヤーの向きから切り出し開始位置(X)を計算
    float modAngle = fmodf(player->posAngle, 2.0f * M_PI);
    if (modAngle < 0.0f) modAngle += 2.0f * M_PI;

    // skyBack: 360度(2*PI)で1周期
    int sampleBackX = (int)(modAngle * skyBackSize.x / (2.0f * M_PI));
    // skyFront: 180度(PI)で1周期（演出上のリスペクト）
    int sampleFrontX = (int)(fmodf(modAngle, M_PI) * skyFrontSize.x / M_PI);

    // 4. テクスチャの生成と切り出し
    // imgSampleWidth 分を切り出すことで、1枚のスプライトで画面横一杯をカバーする
    skyBack.create((unsigned int)imgSampleWidth, (unsigned int)targetImgHeight);
    skyFront.create((unsigned int)imgSampleWidth, (unsigned int)targetImgHeight);

    skyBack.loadFromImage(instance.assetSkyBack,
        sf::IntRect(sampleBackX, backCutY, (int)imgSampleWidth, (int)targetImgHeight));

    skyFront.loadFromImage(instance.assetSkyFront,
        sf::IntRect(sampleFrontX, frontCutY, (int)imgSampleWidth, (int)targetImgHeight));
}


sf::Vector2f Map::mapCoordinates(sf::Vector2f& position) {
    //矩形であるマップ画像上の正規化座標を持つドライバーを
    // 台形のミニマップ上での座標に変換する
    //左上(57.0f / 512.0f, 252.0f / 448.0f),左下(20.0f / 512.0f, 438.0f / 448.0f)
    // //巨大な視錐台によるサンプリングの結果、カメラより遠くにあるものは中央にすぼむ 
    //ゆえに下底に対して上底のほうが辺の小さい台形を想定している。
    //ゆえにドライバーの位置も
    sf::Vector2f bottom(20.0f / 512.0f, 438.0f / 448.0f);
    sf::Vector2f top(57.0f / 512.0f, 252.0f / 448.0f);
    //左上から左下の輪郭線の長さを0~1としたときのpositionの高さ。線形的に求められる
    sf::Vector2f middleLeft = top + (bottom - top) * position.y;
    //台形のうちpositionのある高さを通る平行線分を考えたとき
    //平行線分の右側の座標
    sf::Vector2f middleRight(1.0f - middleLeft.x, middleLeft.y);
    //台形の左上から右下を(0,0),(1,1)とした時のドライバーの正規化座標
    return middleLeft + (middleRight - middleLeft) * position.x;
}