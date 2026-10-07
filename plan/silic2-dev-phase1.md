# Silic2 Phase 1 开发任务：Pressure Arena 战斗垂直原型

## 1. 阶段目标

用一个 5 分钟固定竞技场验证游戏最重要的命题：高速移动、主动瞄准、可读敌潮和危险区域拾取是否能在没有完整 roguelite 包装时成立。

Phase 1 完成后必须具备：

- 一条由 JSON 定义且可复现的 5 分钟敌潮。
- Chaser、Spawner、Sentry 三种职责不同的敌人。
- 玩家敌方投射物、预警和清楚的伤害来源。
- 敌人死亡掉落有时限的 Shard。
- Shard 与滑铲产生明确的战斗收益。
- 基础武器具有主动选择明显的主、副射击。
- 有声环境下，玩家可以仅凭声音判断关键威胁的大致方向。

预计工作量：5-7 个有效开发日。

## 2. 进入条件

- Phase 0 的 seed、统计、重开和测试目标全部完成。
- 当前默认地图可稳定容纳至少 30 个活动敌人。
- 玩家死亡、敌人死亡和子弹碰撞都有唯一事件入口。

## 3. 非目标

- 不做随机房间图、商店、局外解锁或 Boss。
- 不实现全部五类敌人。
- 不做 12 个正式 Augment；本阶段只用 3 个固定测试升级。
- 不追求最终敌人模型，美术使用高可读占位几何。
- 不做动态难度补偿。

## 4. 核心玩法规格

### 4.1 Arena 时长

- 总时长：300 秒。
- 0-45 秒：只出现 Chaser，教学移动和基础射击。
- 45-105 秒：加入 Sentry，教学横移和预警。
- 105-180 秒：加入 Spawner，建立目标优先级。
- 180-240 秒：三者组合，增加空间压力。
- 240-300 秒：Final pressure，停止教学留白但不改变既有规则。
- 300 秒时清除新生成命令，击杀剩余敌人后视为完成。

死亡立即结束本局，不保留本局升级。

### 4.2 基础武器 Needler

主射击：

- LMB 持续连射。
- 高弹速、低单发伤害、低击退。
- 负责清理 Chaser 和精确打击 Spawner 核心。

副射击：

- RMB 发射近距离散射爆发。
- 消耗 8 点 `Charge`，Charge 由拾取 Shard 获得。
- 对玩家施加可控反冲，可用于快速后撤或配合跳跃改变轨迹。
- 高击退、短射程，不应替代远距离主射击。

第一版不使用传统弹匣和换弹。需要验证的是瞄准、移动和资源循环，不是弹药管理。

### 4.3 Shard 与 Flow

- 普通 Chaser：1 Shard。
- Sentry：2 Shard。
- Spawner：4 Shard。
- Shard 存活 10 秒，最后 3 秒明显闪烁并提高音调。
- 基础拾取半径 1.2 units。
- 滑铲状态拾取半径提高到 2.4 units。
- 每个 Shard 恢复 1 Charge，并增加 5 Flow。
- Flow 范围 0-100；2 秒未获得 Flow 后每秒衰减 12。
- Flow 只影响副射击冷却和视觉反馈，首版不直接增加伤害。

这套规则要求玩家进入危险区收资源，但不会让未滑铲玩家完全失去升级机会。

## 5. 架构与文件规划

新增：

```text
include/game/encounter_director.h
include/game/encounter_definition.h
include/game/pickup_manager.h
include/game/flow_meter.h
include/enemy/enemy_definition.h
include/enemy/enemy_projectile.h
include/audio/audio_system.h
src/game/encounter_director.cpp
src/game/encounter_definition.cpp
src/game/pickup_manager.cpp
src/game/flow_meter.cpp
src/enemy/enemy_projectile.cpp
src/audio/audio_system.cpp
res/gameplay/encounters/pressure_arena_01.json
res/gameplay/enemies/chaser.json
res/gameplay/enemies/spawner.json
res/gameplay/enemies/sentry.json
res/audio/README.md
```

修改：

```text
include/enemy/enemy.h
include/enemy/enemy_manager.h
src/enemy/enemy.cpp
src/enemy/enemy_manager.cpp
include/player/weapon.h
src/player/weapon.cpp
include/player/player.h
src/player/player.cpp
include/engine/map.h
src/engine/map.cpp
include/engine/app.h
src/engine/app.cpp
include/hud/hud_renderer.h
src/hud/hud_renderer.cpp
```

## 6. 数据规格

### 6.1 EnemyDefinition

```json
{
  "id": "sentry",
  "maxHp": 4,
  "moveSpeed": 0.8,
  "contactDamagePerSecond": 0,
  "attack": {
    "type": "aimed_projectile",
    "cooldown": 2.4,
    "telegraph": 0.8,
    "projectileSpeed": 12,
    "damage": 18
  },
  "shardDrop": 2,
  "threat": {
    "attention": 3,
    "space": 2,
    "damage": 3,
    "priority": 2
  }
}
```

定义加载失败必须终止开发构建启动，并输出文件名和字段；Release 构建可回退到内置默认值，但不能静默使用 0。

### 6.2 EncounterDefinition

```json
{
  "id": "pressure_arena_01",
  "duration": 300,
  "maxAlive": 40,
  "events": [
    { "time": 0, "enemy": "chaser", "count": 4, "zone": "edge_front" },
    { "time": 12, "enemy": "chaser", "count": 6, "zone": "edge_any" },
    { "time": 48, "enemy": "sentry", "count": 1, "zone": "edge_far" }
  ]
}
```

事件按 `time` 稳定排序。同一时刻保持 JSON 原顺序。达到 `maxAlive` 时事件进入 pending queue，不丢弃、不重新随机。

### 6.3 SpawnZone

地图新增 `spawn_zone` 实体：

```json
{
  "type": "spawn_zone",
  "name": "edge_front",
  "position": [0, 0, -8],
  "scale": [6, 1, 2]
}
```

生成点从 zone 的 XZ 范围确定性采样，并执行：

- 距玩家至少 6 units。
- 不与静态 brush 相交。
- 不在玩家当前视锥中心 20 度内突然出现，除非事件标记允许。
- 最多尝试 16 次，失败时记录错误并使用 zone 中心的验证点。

## 7. 敌人行为规格

### 7.1 Chaser

- 保留当前水平追击和接触伤害基础。
- 进入攻击距离前不瞬时转向，保持可绕行轨迹。
- 同类之间使用简单分离力，避免完全重叠。
- 轮廓：矮宽或明显前倾，不能与 Sentry 共用相同比例盒子。
- 声音：连续低频运动声；接近 3 units 时增加一层节拍。

### 7.2 Sentry

- 生成后寻找可见位置，停止移动并充能。
- 充能 0.8 秒：瞄准线从暗到亮，方向音逐渐升高。
- 充能结束锁定方向，不继续追踪玩家，再延迟 0.1 秒发射。
- 投射物可以被玩家看见并躲避；不得 hitscan。
- 受重击退或副射击命中时取消本次充能。

### 7.3 Spawner

- 缓慢向房间中心移动，每 5 秒产生 2 个弱 Chaser。
- 首次生成前有 1.5 秒脉冲预警。
- 同时存活的子体有上限，Spawner 死亡后停止生成。
- 核心在生成前发亮，提供集中火力窗口。
- 掉落更多 Shard，形成风险与奖励的一致优先级。

## 8. 任务拆解

### P1-01：数据化现有 Enemy

依赖：Phase 0。

工作：

- 增加 `EnemyType` 和 `EnemyDefinition`。
- 将 HP、速度、接触伤害、尺寸和 Shard 数量从硬编码迁移。
- `Enemy` 保存 definition ID 或不可变引用，不拥有资源。
- `EnemyManager` 按 type 创建敌人，并继续集中管理生命周期。
- 三种行为先使用 type switch，不建立继承树。

验收：现有 Chaser 行为不回归；修改 JSON 后重启即可看到参数变化。

### P1-02：实现 SpawnZone 与 EncounterDefinition 加载

依赖：P1-01。

工作：

- 扩展地图实体枚举、解析和保存。
- 验证 zone 名称唯一、scale 为正、位置在地图边界内。
- 实现 encounter JSON 解析、字段错误定位和稳定排序。
- 增加无 OpenGL 的解析测试。

验收：错误 enemy ID、zone ID、负时间、乱序事件和超出 duration 都有明确失败。

### P1-03：实现 EncounterDirector

依赖：P1-02。

工作：

- `start(definition, seedStream)`、`update(dt)`、`stop()`、`reset()`。
- director 只发出 `SpawnCommand`，不直接访问渲染器。
- App 将命令交给 EnemyManager。
- 处理 pending queue、maxAlive 和 encounter completion。
- 把当前 phase、下一事件时间和 pending 数写入调试摘要。

验收：不同帧率步进到 300 秒时发出的命令序列一致。

### P1-04：实现敌方投射物

依赖：P1-01。

工作：

- 新建独立于玩家 Bullet 的 `EnemyProjectile`。
- 使用 swept segment 对玩家 AABB 检测，避免低帧率穿透。
- 命中、超时和撞墙后移除。
- 清晰区分敌弹颜色、轮和 glow；低分辨率下至少占 2 个内部像素。
- 伤害记录为 `EnemyProjectile`，带来源 type。

验收：20 FPS 和 144 FPS 下命中结果一致；穿墙射击不可伤害玩家。

### P1-05：实现 Sentry

依赖：P1-04。

工作：

- 增加 Idle、Charging、Recovering 状态。
- 充能开始时锁定攻击上下文，发射前锁定最终方向。
- 视觉预警和音频预警共享同一剩余时间参数。
- 副射击击退可以打断 Charging。

验收：玩家第一次遇见时能在看到/听到预警后躲开；不存在无预警发射。

### P1-06：实现 Spawner

依赖：P1-01、P1-03。

工作：

- 增加 SpawnWarmup 和 Recovering 状态。
- 子体计数使用稳定 parent ID；父体删除时安全解除关联。
- 全局和单体子体上限同时生效。
- Spawner 生成行为写入 RunStats。

验收：多个 Spawner 不会指数失控；达到 maxAlive 后不继续生成但 cooldown 行为明确。

### P1-07：实现 PickupManager 与 Shard

依赖：P1-01、Phase 0 统计。

工作：

- 敌人死亡事件产生 Shard，不从 renderer 回调生成。
- Shard 包含 position、value、ttl 和稳定 ID。
- 更新、拾取和过期在 PickupManager 中完成。
- 用简单 billboard/点精灵渲染，闪烁由 ttl 驱动。
- 统计 spawned、collected、expired 和 collection distance。

验收：敌人死亡与 Shard 数量一致；重开后没有旧 Shard；暂停时 ttl 不减少。

### P1-08：实现 FlowMeter 和滑铲拾取联动

依赖：P1-07。

工作：

- FlowMeter 独立于 Player 物理 momentum。
- App 根据 PlayerState 和 pickup result 更新 Flow。
- HUD 增加无文字 Flow 条，明确 0、可用副射击阈值和满值。
- 平衡参数进入 gameplay config，不写死在 HUD。

验收：站立拾取和滑铲拾取都有收益，但滑铲更有效；Flow 暂停时不衰减。

### P1-09：实现 Needler 双模式

依赖：P1-07、P1-08。

工作：

- 将输入拆成 primary/secondary 边沿与持续状态。
- 主射击保留现有连续子弹。
- 副射击生成固定散布的多枚弹丸，消耗 Charge。
- 反冲通过 Player 公共方法施加，不能直接修改私有 velocity。
- 所有弹丸共享 shot ID，统计一次射击、多次命中。

验收：无 Charge 时提供明确失败反馈；副射击反冲不能把玩家推进墙内；同 seed 不依赖随机散布。

### P1-10：接入最小音频层

依赖：P1-05、P1-06、P1-07。

工作：

- 建立 `AudioSystem` 后端接口和 RAII 生命周期。
- 首选 vendored 单文件跨平台后端；依赖版本在实施时固定并记录许可证。
- 支持 one-shot、loop、3D position、gain、pitch 和 group。
- 首批事件：Sentry charge/fire、Spawner pulse/spawn、Chaser near、Shard expiring/pickup、player hit/death。
- 同类高频声音设置并发上限和最短重触发间隔。

验收：左右方向变化正确；40 个敌人不会产生不可控叠音；关闭音频不影响逻辑。

### P1-11：制作 5 分钟敌潮

依赖：P1-03、P1-05、P1-06、P1-09。

工作：

- 先用表格规划每 15 秒的 attention/space/damage 预算。
- 每次只引入一种新规则，之后再组合。
- 在 60、150、240 秒设置三个明显压力峰。
- 峰值后提供 5-10 秒回收窗口，但不完全停止敌人。
- 使用至少三个固定 seed 做回归。

验收：没有超过 8 秒的无目标时间；没有单一敌种长期占活动实体 80% 以上。

### P1-12：测试与决策门

依赖：全部 Phase 1 任务。

工作：

- 至少 5 名测试者，每人连续 3 局，不提前解释敌人答案。
- 记录第一局和第二局的存活、得分、伤害来源与重开时间。
- 访谈问题只问观察：“你为什么死”“哪个声音最重要”“什么时候想停”。
- 将结论写入 `plan/phase1-playtest-results.md`。

进入 Phase 2 的条件：至少 3/5 测试者第二局明显改善，且至少 3/5 在没有解锁奖励的情况下主动开始第三局。

## 9. 自动测试清单

- `EncounterDefinition.rejects_invalid_events`
- `EncounterDirector.same_seed_same_commands`
- `EncounterDirector.frame_step_independent`
- `EncounterDirector.respects_alive_cap`
- `SpawnZone.avoids_player_and_geometry`
- `EnemyProjectile.swept_hit_is_stable`
- `Spawner.child_cap_is_enforced`
- `PickupManager.pause_freezes_ttl`
- `PickupManager.reset_clears_all`
- `FlowMeter.decay_and_clamp`
- `Weapon.secondary_consumes_charge_once`

## 10. 性能预算

- 40 个活动敌人、80 枚敌弹、150 个 Shard、2000 个环境粒子下保持目标帧率。
- Enemy update 总耗时目标低于 CPU frame 的 20%。
- 音频并发 voice 上限建议 32，具体以实际后端测试为准。
- 同类敌人的渲染继续批处理；不要为每个实例创建 VAO/VBO。
- 调试构建每 10 秒记录一次 subsystem frame time。

## 11. 人工测试脚本

1. 使用固定 seed 完整存活或死亡三次，确认事件顺序一致。
2. 在 45 秒前只观察 Chaser 行为和分离。
3. 遮住 Sentry 视觉方向，仅凭声音尝试闪避。
4. 故意不杀 Spawner，确认压力增长但不无限失控。
5. 测试站立拾取、跑步拾取、滑铲拾取和过期。
6. 连续使用副射击反冲撞墙、上坡、空中和近敌。
7. 暂停 10 秒，确认敌潮、子弹、Shard 和 Flow 全部冻结。
8. 死亡后重开，确认音频 loop、投射物和 parent 关系清空。

## 12. 建议提交边界

1. `Data-drive enemy definitions and spawn zones`
2. `Add deterministic encounter director`
3. `Add sentry projectiles and spawner behavior`
4. `Add shard pickup and flow systems`
5. `Add Needler secondary fire and recoil movement`
6. `Add positional combat audio cues`
7. `Add pressure arena encounter and tests`

## 13. 完成定义

- [ ] 5 分钟敌潮可从 JSON 完整运行。
- [ ] 三种敌人的职责、轮廓和声音可区分。
- [ ] 同 seed、不同帧率产生相同生成命令。
- [ ] Shard 回收率可测量，滑铲产生明确收益。
- [ ] 主、副射击都有不可替代的使用场景。
- [ ] 重开清理敌人、敌弹、Shard、音频和武器状态。
- [ ] 自动测试和 Mac/Windows 构建通过。
- [ ] 完成第一轮 5 人测试并通过决策门。

## 14. 阶段出口

若玩家无法解释死亡、第二局没有学习改善，先修改敌人预警和敌潮，不进入 Augment 开发。构筑只能放大成立的战斗，不能替代战斗本身。
