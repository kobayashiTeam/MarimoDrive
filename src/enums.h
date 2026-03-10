#pragma once
#include <SFML/Graphics.hpp>
#define M_PI       3.14159265358979323846
#define M_PI_2     1.57079632679489661923
#define M_PI_4     0.78539816339744830962


//共通データ管理クラス
static constexpr float SCREEN_WIDTH = 1024.0f;
static constexpr float SCREEN_HEIGHT = 648.0f;
static constexpr int FRAMERATE = 60;
//
static constexpr int MAP_ASSET_WIDTH = 1024;
static constexpr int MAP_ASSET_HEIGHT = 1024;
static constexpr int MAP_EDGES_SIZE = 8;
static constexpr int MAP_TILE_SIZE = 8;
static constexpr int MAP_TILE_NUM_WIDTH = MAP_ASSET_WIDTH / MAP_TILE_SIZE;
static constexpr int MAP_TILE_NUM_HEIGHT = MAP_ASSET_HEIGHT / MAP_TILE_SIZE;

//マップ上の各タイル
enum class MapLand : int {
    TRACK,            // 通常エリア
    BLOCK,            // 侵入不可エリア
    SLOW,             // 遅延エリア
    OUTER,            // マップ外
};

typedef std::array<std::array<MapLand, MAP_TILE_NUM_WIDTH>, MAP_TILE_NUM_HEIGHT>
MapLandMatrix;

static constexpr float MINIMAP_POS_DISTANCE = 6.5f;
static constexpr float MINIMAP_FOV_HALF = M_PI / 32.0f;
static constexpr float SKY_HEIGHT_PCT = 2.0f / 22.4f;
static constexpr float CIRCUIT_HEIGHT_PCT = 9.2f / 22.4f;
static constexpr float MINIMAP_HEIGHT_PCT = 11.2f / 22.4f;

static constexpr float CAM_TO_PLAYER_DST = 46.0f;
static constexpr float MODE7_FOV_HALF = 0.5f;
static constexpr float MODE7_CLIP_NEAR = 0.0005f;
static constexpr float MODE7_CLIP_FAR = 0.045f;

static constexpr int PLAYER_DRIVER_INDEX = 0;
static constexpr int DRIVER_NUM=4;
static constexpr int NUM_LAPS_IN_CIRCUIT = 2;
class Driver;
typedef std::shared_ptr<Driver> DriverPtr;
typedef std::array<DriverPtr, DRIVER_NUM> DriverArray;
typedef std::array<Driver*, DRIVER_NUM> RaceRankingArray;

static constexpr float HITBOXRADIOUS = 1.5f;
static constexpr float VISUALRADIOUS = 1.0f;


static constexpr float PLAYER_START_X = 180.0f;
static constexpr float PLAYER_START_Y = 620.0f;
static constexpr float ENEMY1_START_X = 160.0f;
static constexpr float ENEMY1_START_Y = 660.0f;
static constexpr float ENEMY2_START_X = 160.0f;
static constexpr float ENEMY2_START_Y = 640.0f;
static constexpr float ENEMY3_START_X = 160.0f;
static constexpr float ENEMY3_START_Y = 620.0f;



enum class WallObjectType : int {
	WAKAME = 0,
    SANGO=1,
    HITODE=2
};

//Drive setting
static constexpr float MOTOR_ACCELERATION = 0.15f;
static constexpr float MOTOR_TURNING_ACCELERATION = 0.1f;
static constexpr float MAX_NORMALLAND_LINEARSPEED = 0.13f;
static constexpr float MAX_SLOWLAND_LINEARSPEED = 0.05f;
static constexpr float SLOWLAND_LOSE_ACCELERATION = -0.06f;
static constexpr float MAX_TURNING_ANGULARSPEED = 0.8f;
static constexpr float FRICTION_LINEAR_ACELERATION = -0.03f;
static constexpr const float BREAK_ACELERATION = -0.2f;
static constexpr float POSITION_ACCELERATION_BONUS_PCT = 0.0085f;

enum class RacePhase : int {
    RACING,
    RESULT,
    DONE,
};

enum class CourseOption : unsigned int {
    COURSE1,
    COURSE2,
    COURSE3,
    __COUNT,
};

