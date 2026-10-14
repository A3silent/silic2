#pragma once
#include <memory>
#include <fstream>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include "engine/app_options.h"
#include "engine/run_state.h"
#include "game/gameplay_types.h"
struct GLFWwindow;
namespace silic2 {
class Map; class MapRenderer; class PixelRenderer; class Camera; class Player;
class Weapon; class GroundParticleSystem; class EnemyManager; class EnemyProjectileSystem;
class HudRenderer; class RunController; class EncounterDirector; class AudioSystem;
class CharacterPresentation; class Shader; class ExplorationController;
struct HudViewModel;
class App {
public:
    explicit App(uint64_t runSeed);
    explicit App(const AppOptions& options);
    ~App();
    void run();
    bool loadMap(const std::string& mapFile);
private:
    AppOptions options;
    GLFWwindow* window = nullptr;
    std::unique_ptr<Map> currentMap;
    std::unique_ptr<MapRenderer> mapRenderer;
    std::unique_ptr<PixelRenderer> pixelRenderer;
    std::unique_ptr<Camera> camera;
    std::unique_ptr<Player> player;
    std::unique_ptr<Weapon> weapon;
    std::unique_ptr<GroundParticleSystem> groundParticles;
    std::unique_ptr<EnemyManager> enemyManager;
    std::unique_ptr<EnemyProjectileSystem> hostileProjectiles;
    std::unique_ptr<HudRenderer> hudRenderer;
    std::unique_ptr<RunController> runController;
    std::unique_ptr<EncounterDirector> director;
    std::unique_ptr<ExplorationController> exploration;
    std::unique_ptr<AudioSystem> audio;
    std::unique_ptr<CharacterPresentation> characters;
    std::unique_ptr<Shader> projectileShader;
    unsigned int projectileVAO=0, projectileVBO=0;
    std::ofstream runLog, acceptanceLog;
    std::string currentMapFile;
    bool arena=false;
    GameState gameState=GameState::READY;
    float stateTimer=0, deltaTime=0;
    bool escWasPressed=false, rWasPressed=false, leftMouseWasPressed=false, enterWasPressed=false;
    bool contactDamageActive=false, transformationPlayed=false, wasSliding=false;
    bool refocused=false;
    float hitAge=1000,killAge=1000,damageAge=1000,damageDirection=0;
    glm::vec3 previousPlayerPosition{0};
    int acceptanceRuns=0, acceptanceFrames=0, screenshotStage=0;
    double modeStarted=0, frameStarted=0, flowPlaybackSeconds=0;
    std::vector<double> frameTimes;
    bool acceptanceFailed=false;
    size_t journeyNode=0;
    float journeyHold=0;
    int journeyPhase=-1;
    int combatLap=0;
    float noVisibleSeconds=0,longestNoVisible=0,firstFireSeconds=-1;
    int combatLastShots=0;
    bool combatShiftBefore=false,combatShiftAfter=false,combatLowerCaptured=false;
    float generationGap=0,longestGenerationGap=0,cleanupGap=0,longestCleanupGap=0,cleanupRefresh=0;
    glm::vec3 cleanupWaypoint{0};
    std::string pendingScreenshot;
    bool frameCaptured=false;
    int flowStep=0;
    float flowFrozenTime=0;
    uint64_t flowAudioFrames=0;
    glm::vec3 flowFrozenPosition{0};
    bool initWindow();
    void cleanup();
    void processInput();
    void update(float dt);
    void updatePlaying(float dt);
    void render();
    void setState(GameState next);
    void finishCurrentRun(RoomResult result, RunEndReason reason);
    void restartCurrentRun();
    void resetRuntimeState();
    glm::vec3 getPlayerStartPosition() const;
    HudViewModel makeHud(const glm::mat4& view,const glm::mat4& projection) const;
    bool visible(const glm::vec3& from,const glm::vec3& to) const;
    void applyDamage(float amount,DamageSource source,const glm::vec3& origin,bool newEvent=true);
    void firePrimary();
    void updateAcceptance(float dt);
    void updateExplorationAcceptance(float dt);
    void updateSpatialCombatAcceptance(float dt);
    void recordPerformance();
    void captureScreenshot(const std::string& name,int width=0,int height=0);
    void logReset();
    void renderProjectiles(const glm::mat4& view,const glm::mat4& projection);
    static void framebufferSizeCallback(GLFWwindow*,int,int);
    static void mouseCallback(GLFWwindow*,double,double);
    static void scrollCallback(GLFWwindow*,double,double);
    static void focusCallback(GLFWwindow*,int);
};
}
