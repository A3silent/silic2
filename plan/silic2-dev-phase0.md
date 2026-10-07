# Silic2 Phase 0 开发任务：测量、确定性与快速重开

## 1. 阶段目标

在增加玩法内容前，建立一套能回答“玩家为什么死亡、何时掉队、同一局能否复现”的最小基础设施。

Phase 0 完成后必须具备：

- 每局拥有明确且可打印的 64 位 seed。
- Mac 与 Windows 在同一 seed 下产生相同的逻辑随机序列。
- 能统计局时、击杀、受伤、移动、滑铲和房间结果。
- 玩家死亡或主动结束后，3 秒内可以开始一局干净的新游戏。
- 调试构建能实时查看最关键的 8-10 个数值。
- 核心统计与随机逻辑不依赖 OpenGL，可以独立测试。

预计工作量：2-3 个有效开发日。

## 2. 进入条件

- 当前 `macos-port` 可在 macOS 编译并运行。
- 玩家、敌人、子弹命中、房间清理和玩家死亡流程可用。
- 不要求 Phase 1 的敌潮、Shard 或新敌人存在。

## 3. 非目标

- 不做正式结算 UI 美术。
- 不做在线遥测、账号、排行榜或 replay 文件。
- 不调整敌人和武器平衡。
- 不实现完整房间图和局外存档。
- 不引入通用 ECS 或反射系统。

## 4. 架构决策

### 4.1 逻辑时间与真实时间分离

- 游戏统计使用累加的 `deltaTime`，不读取系统时间。
- 暂停状态不增加 run time 和 room time。
- UI 动画可以读取真实时间，但不得影响敌人、掉落和随机结果。
- 单帧 `deltaTime` 继续保持当前 50ms 上限。

### 4.2 随机数只有一个入口

新增 `SeedService`，由 run seed 派生命名子流：

- `encounter`
- `rewards`
- `room_graph`
- `cosmetic`

游戏逻辑禁止直接调用 `std::random_device`、`rand()` 或文件级全局 RNG。子 seed 使用项目内固定的 FNV-1a 或 SplitMix64 混合函数；不要使用 `std::hash<std::string>`，因为跨标准库结果不保证一致。

`cosmetic` 子流不得反向影响逻辑子流。增加粒子数量或随机音高不能改变奖励和敌潮。

### 4.3 统计采用事件入口，存储保持简单

Phase 0 不建立通用 event bus。`RunStats` 提供明确的方法：

```cpp
void beginRun(uint64_t seed);
void beginRoom(const std::string& roomId);
void recordKill(EnemyType type);
void recordDamage(float amount, DamageSource source);
void recordShot(bool hit);
void recordMovement(float distance, PlayerState state, float dt);
void endRoom(RoomResult result);
void endRun(RunEndReason reason);
```

调用者通过 `App` 或具体系统直接调用这些窄接口。等 Phase 2 出现大量 Augment 事件后，再引入专用 gameplay hooks。

### 4.4 第一版调试显示不依赖字体系统

当前项目没有文字渲染器。Phase 0 使用两种低成本输出：

- 每秒一次的结构化控制台摘要。
- Debug 模式下通过 `glfwSetWindowTitle` 显示 seed、FPS、敌人数、局时和玩家 HP。

正式 Debug overlay 和卡牌文字渲染放到 Phase 2 一起解决，避免 Phase 0 被字体资源阻塞。

## 5. 文件规划

新增：

```text
include/game/run_stats.h
include/game/seed_service.h
include/game/run_controller.h
src/game/run_stats.cpp
src/game/seed_service.cpp
src/game/run_controller.cpp
tests/run_stats_test.cpp
tests/seed_service_test.cpp
```

修改：

```text
include/engine/run_state.h
include/engine/app.h
src/engine/app.cpp
include/player/player.h
src/player/player.cpp
include/player/weapon.h
src/player/weapon.cpp
include/enemy/enemy_manager.h
src/enemy/enemy_manager.cpp
Makefile.map
```

## 6. 数据结构

### 6.1 RunState

将现有占位结构扩展为纯局内状态：

```cpp
struct RunState {
    uint64_t seed = 0;
    uint32_t runIndex = 0;
    int act = 1;
    int floor = 1;
    int currency = 0;
    bool active = false;
};
```

`RunState` 不保存计数器细节，也不持有 OpenGL 或实体对象。

### 6.2 RunStatsSnapshot

```cpp
struct RunStatsSnapshot {
    uint64_t seed = 0;
    float runTime = 0.0f;
    float roomTime = 0.0f;
    float distanceTravelled = 0.0f;
    float slidingTime = 0.0f;
    int kills = 0;
    int shotsFired = 0;
    int shotsHit = 0;
    int damageEvents = 0;
    float damageTaken = 0.0f;
    int roomsCleared = 0;
};
```

命中率只在 `shotsFired > 0` 时计算。统计展示层不应保存派生值。

### 6.3 DamageSource

第一版枚举：

```cpp
enum class DamageSource {
    EnemyContact,
    EnemyProjectile,
    Environment,
    SelfDamage,
    Unknown
};
```

来源需要保留敌人类型和可选实体 ID 的扩展空间，但 Phase 0 不要求稳定实体 GUID。

## 7. 任务拆解

### P0-01：建立 game 目录和构建入口

依赖：无。

工作：

- 创建 `include/game`、`src/game` 和 `tests`。
- 将新增 `.cpp` 以完整 `src/...` 路径加入 `GAME_SRCS`，测试源文件单独加入测试目标，继续保留目录化对象文件。
- 增加 `test` 目标，测试程序不链接 GLFW/OpenGL。
- 保持 Windows、macOS 和 Linux 使用相同源列表。

验收：空实现能随主程序编译；`make -f Makefile.map test` 能运行一个最小测试二进制。

### P0-02：实现 SeedService

依赖：P0-01。

工作：

- 支持命令行 `--seed <uint64>`。
- 未传 seed 时，启动阶段生成一次 seed 并打印。
- 使用固定算法从 run seed 和 stream name 派生子 seed。
- 每个命名流持有独立 `std::mt19937_64`。
- 提供闭区间整数、半开区间浮点和容器索引 API。
- 对非法范围使用断言和错误返回，不隐式交换上下界。

验收：同一 seed 的前 1000 个结果在两次进程运行中一致；调用 cosmetic 流不会改变 rewards 流。

### P0-03：实现 RunStats

依赖：P0-01。

工作：

- 实现 run/room 生命周期断言。
- 每帧只通过 `tick(dt)` 累加活动时间。
- 记录击杀、伤害、射击、命中、移动状态和房间结果。
- 每秒输出一行固定字段顺序的调试摘要。
- 结束时输出单行 JSON 或稳定的 key-value 格式，方便后续脚本分析。

验收：暂停 10 秒不改变 run time；重复 `endRun()` 不重复累加或崩溃。

### P0-04：接入玩家移动与伤害

依赖：P0-03。

工作：

- `Player::takeDamage` 接受或回传 `DamageSource`。
- 在应用层记录实际扣除的 HP，而不是传入的原始伤害。
- 每帧使用位置差记录移动距离。
- 根据 `PlayerState::SLIDING` 记录滑铲时间。
- God mode 造成的无伤状态仍记录 incoming event，但 `damageTaken` 为 0，可单独计数。

验收：站立不增加移动距离；撞墙时只记录实际位移；死亡伤害总量与 HP 变化一致。

### P0-05：接入武器和敌人结果

依赖：P0-03。

工作：

- `Weapon::fire` 成功创建子弹时记录射击；冷却期间点击不算射击。
- 子弹第一次命中敌人时记录命中。
- `EnemyManager::checkBulletHit` 返回包含 `hit`、`killed`、`enemyType` 的结果结构，替代单一 bool。
- 击杀只记录一次，移除死亡敌人时不得再次记录。
- 房间清空时调用 `endRoom(Cleared)`。

验收：一发穿透或分裂机制尚未存在时，一枚子弹最多记录一次命中；3 HP 敌人只产生一个 kill。

### P0-06：实现 RunController 和干净重开

依赖：P0-02、P0-03。

工作：

- `RunController` 负责 `startRun`、`endRun`、`restartRun`。
- 给 `Weapon` 增加 `reset()`，清空子弹、impact light 和冷却。
- 给 `Player` 的 respawn 明确重置 HP、位置、速度、滑铲、相机偏移和输入边沿状态。
- 给 `EnemyManager` 使用 `clear()` 后重新从当前地图生成。
- 重置 `GameState`、room timer、run stats 和所有逻辑 RNG 流。
- 死亡后显示 1 秒结果状态，按 `R` 或 LMB 重开；开发模式可自动重开。

验收：连续重开 50 次，实体数量、活动子弹和统计都不增长；同 seed 重开产生相同逻辑结果。

### P0-07：添加调试摘要

依赖：P0-03、P0-06。

工作：

- Debug 构建每秒更新窗口标题。
- 标题字段：FPS、seed、run time、enemy count、HP、shots/hits。
- Release 构建不改变正式窗口标题，也不进行每秒控制台刷屏。
- 增加 `F3` 切换调试摘要，使用边沿检测。

验收：开启时更新频率不超过 2Hz；关闭后没有额外字符串构造热点。

### P0-08：测试与基线记录

依赖：P0-01 至 P0-07。

工作：

- 编写 SeedService 与 RunStats 单元测试。
- 手动完成死亡、房间清理、暂停、重开和指定 seed 流程。
- 保存一份 `tests/baselines/phase0-run.txt`，记录默认地图的一次基线数据。
- 在 Mac 与 Windows 各运行一次相同 seed，比较逻辑随机序列。

验收：所有测试命令返回 0；差异仅允许出现在 FPS、真实运行耗时和渲染信息中。

## 8. 自动测试清单

- `SeedService.same_seed_same_sequence`
- `SeedService.named_streams_are_independent`
- `SeedService.invalid_range_rejected`
- `RunStats.pause_does_not_tick`
- `RunStats.damage_uses_actual_hp_loss`
- `RunStats.kill_is_counted_once`
- `RunStats.room_lifecycle_is_validated`
- `RunController.restart_resets_runtime_state`

测试应使用项目自带轻量断言即可，Phase 0 不必为了测试框架引入包管理器。

## 9. 人工测试脚本

1. 使用 `--seed 42` 启动，记录标题和控制台 seed。
2. 静止 5 秒，确认移动距离接近 0。
3. 走、跑、滑铲各 5 秒，确认状态时间合理。
4. 连续射击敌人，确认 shots、hits、kills 变化。
5. 暂停 10 秒，确认 run time 不增加。
6. 被敌人击杀，确认死亡来源为 contact。
7. 按 `R` 重开，确认 HP、敌人、子弹和计时归零。
8. 再次使用 seed 42，确认随机序列摘要一致。
9. 连续重开 50 次，观察内存和实体数量。

## 10. 建议提交边界

1. `Add deterministic run seed service`
2. `Add run statistics and gameplay instrumentation`
3. `Add clean run restart flow`
4. `Add phase zero tests and debug summary`

每个提交都必须独立编译。不要把格式化整个代码库混入这些提交。

## 11. 完成定义

- [x] 主程序和测试在 macOS 编译通过。
- [ ] Windows 使用相同 seed 得到相同逻辑随机序列。
- [x] 所有核心统计有唯一写入点。
- [x] 暂停、死亡和重开生命周期无重复事件。
- [x] 死亡后等待 1 秒即可由玩家重新开始。
- [ ] 连续 50 次重开无明显状态泄漏。
- [x] Phase 0 自动测试全部通过。
- [x] `docs/architecture.md` 更新新增 game 层和 seed 规则。

## 12. 阶段出口

Phase 0 完成后才能进入 Phase 1。若无法稳定复现同一 seed，禁止开始随机敌潮和奖励系统；否则后续平衡问题将无法定位。
