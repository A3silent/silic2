# Silic2 Phase 2 开发任务：构筑闭环

## 1. 阶段目标

把 Phase 1 的固定战斗原型扩展为可形成不同打法的短局：玩家使用三把行为不同的武器，在三次奖励中逐步形成能被描述、能被看见、也有代价的构筑。

Phase 2 完成后必须具备：

- 3 把主、副射击差异明确的武器。
- 12 个至少改变一种可见行为的 Augment。
- 确定性的房间后 3 选 1，以及每局一次 reroll。
- 标签、稀有度、冲突、前置条件和叠加上限验证。
- 至少 4 组可被玩家识别的协同组合。
- 奖励选择期间暂停战斗，选择结束后恢复原有逻辑时间。
- 构筑摘要、升级演出和本局结束统计。
- Augment 逻辑可在无 OpenGL 环境中测试。

预计工作量：5-8 个有效开发日。

## 2. 进入条件

- Phase 1 的 5 分钟 Pressure Arena 可完整运行。
- 三种敌人的职责、预警和伤害来源已经通过测试。
- Shard、Charge、Flow 和双模式 Needler 已有唯一状态入口。
- 同一种子可以复现敌潮与掉落顺序。

如果 Phase 1 的移动和威胁识别仍不成立，先停止本阶段。构筑不能用于遮盖基础战斗问题。

## 3. 非目标

- 不实现房间图、地图切换、Boss 或局外解锁。
- 不做无限层级、任意脚本或通用事件总线。
- 不做复杂物品栏、拖放装备或武器配件槽。
- 不允许 Augment 直接持有 OpenGL、音频或敌人容器。
- 不以纯数值 `+5% damage` 填充奖励池。

## 4. 架构决策

### 4.1 Definition、Runtime 与表现分离

- `WeaponDefinition` 保存静态参数和兼容标签。
- `WeaponRuntime` 保存冷却、Charge、热量和当前射击状态。
- `AugmentDefinition` 保存元数据、条件和效果 ID。
- `AugmentSystem` 保存本局已选项、等级和运行时状态。
- 渲染与音频只消费 `GameplayCue`，不得成为效果是否发生的依据。

第一版效果使用明确的 C++ `effectId` 分派，不在 JSON 中解释公式或执行脚本。这样可以保留数据调参，同时让行为可调试、可测试。

### 4.2 使用窄 gameplay hooks

第一版 hooks：

```cpp
void modifyWeaponStats(WeaponStats& stats, const WeaponContext& ctx) const;
void onShotCreated(ShotContext& ctx, EffectQueue& out);
void onProjectileHit(const HitContext& ctx, EffectQueue& out);
void onEnemyKilled(const KillContext& ctx, EffectQueue& out);
void onShardCollected(const PickupContext& ctx, EffectQueue& out);
void onPlayerStateChanged(const PlayerStateContext& ctx, EffectQueue& out);
```

所有 context 使用只读数据、稳定 entity ID 和当前逻辑 tick。效果通过 `EffectQueue` 延迟执行，禁止在遍历敌人或子弹容器时直接增删实体。

### 4.3 阻止无限触发

每个派生攻击携带：

```cpp
struct EffectTrace {
    uint64_t rootShotId;
    uint8_t generation;
    uint32_t flags;
};
```

约束：

- 默认最大派生深度为 2。
- 冲击波、分裂弹和反弹弹不得重复触发自己的来源效果。
- 每个逻辑 tick 最多执行固定数量派生效果，超过时记录错误并丢弃低优先级表现效果。
- `EffectQueue` 按创建序号稳定执行，不能依赖指针地址或容器哈希顺序。

### 4.4 奖励池保证可用性

候选流程固定为：过滤未解锁项 -> 过滤冲突/满级项 -> 计算兼容权重 -> 确定性洗牌 -> 取三个不同选项。

- 三项中至少一项与当前武器标签匹配。
- 前两次选择至少提供一个构筑核心，而不是三个条件型效果。
- 同一轮不得出现同一 Augment 的不同等级。
- 候选不足三项时使用明确的通用备选，不显示空卡。
- reroll 从新子流抽取，并排除当前三项；重开同 seed 时结果一致。

### 4.5 Phase 2 同时补齐最小字体能力

当前 HUD 没有文字系统。新增固定字形图集和 `BitmapFontRenderer`，仅支持开发所需 ASCII。中文本地化、复杂排版和动态字体留到后续。

## 5. 文件规划

新增：

```text
include/game/gameplay_event.h
include/game/effect_queue.h
include/game/augment.h
include/game/augment_system.h
include/game/reward_draft.h
include/game/weapon_definition.h
include/hud/bitmap_font_renderer.h
include/hud/reward_screen.h
src/game/effect_queue.cpp
src/game/augment.cpp
src/game/augment_system.cpp
src/game/reward_draft.cpp
src/game/weapon_definition.cpp
src/hud/bitmap_font_renderer.cpp
src/hud/reward_screen.cpp
res/gameplay/weapons/needler.json
res/gameplay/weapons/scatter_cannon.json
res/gameplay/weapons/rail_lance.json
res/gameplay/augments/*.json
res/fonts/debug_8x8.png
tests/effect_queue_test.cpp
tests/augment_system_test.cpp
tests/reward_draft_test.cpp
```

修改：

```text
include/player/weapon.h
src/player/weapon.cpp
include/enemy/enemy_manager.h
src/enemy/enemy_manager.cpp
include/game/run_controller.h
src/game/run_controller.cpp
include/engine/app.h
src/engine/app.cpp
include/hud/hud_renderer.h
src/hud/hud_renderer.cpp
Makefile.map
```

## 6. 数据规格

### 6.1 WeaponDefinition

```json
{
  "id": "rail_lance",
  "displayName": "Rail Lance",
  "tags": ["Projectile", "Impact"],
  "primary": {
    "type": "charged_hitscan",
    "damage": 18,
    "chargeTime": 0.65,
    "cooldown": 0.25,
    "pierce": 3
  },
  "secondary": {
    "type": "mobility_burst",
    "chargeCost": 10,
    "cooldown": 1.2
  }
}
```

加载时验证字段范围、射击类型、资源消耗和 tag。未知字段在 Debug 构建给出警告，未知 `type` 或缺少必填字段直接失败。

### 6.2 AugmentDefinition

```json
{
  "id": "ricochet",
  "displayName": "Ricochet",
  "rarity": "common",
  "tags": ["Projectile"],
  "maxLevel": 1,
  "requiresAny": ["Projectile"],
  "conflicts": [],
  "effectId": "ricochet_v1",
  "weight": 100
}
```

所有引用 ID 在启动时统一验证。冲突必须按双向处理，即使数据只声明一侧。

## 7. 三把武器规格

### 7.1 Needler

- 主射击保持高弹速持续连射，强调追踪瞄准。
- 副射击为消耗 Charge 的近距散射和反冲。
- 兼容标签：`Projectile`、`Movement`、`Heat`。
- 作为稳定基线，不应在任何单项指标上全面优于另外两把。

### 7.2 Scatter Cannon

- 主射击为低射速、多弹丸、近距高击退。
- 副射击压缩散布并提高穿透，但有短暂蓄力。
- 兼容标签：`Impact`、`Movement`、`Risk`。
- 目标打法是主动接近、滑铲穿群并争取危险拾取。

### 7.3 Rail Lance

- 主射击蓄力后发射可穿透的高精度射线或高速弹。
- 蓄力时移动略受限制，取消蓄力不消耗资源。
- 副射击消耗 Charge 产生侧向机动脉冲，不直接造成主要伤害。
- 兼容标签：`Projectile`、`Impact`、`Void`。
- 目标打法是预判排列、处理高优先级目标和保持射线角度。

## 8. 首批 Augment 实现规格

| ID | 实现边界 | 首版上限 |
|---|---|---:|
| Ricochet | 第一次撞静态墙后反射；前方锥内有目标时轻微修正方向 | 1 |
| Split Core | 穿透后的第一个目标生成两枚 45% 伤害弱弹 | 1 |
| Heavy Chamber | 射速乘 0.72，伤害乘 1.55，击退乘 1.8 | 2 |
| Aftershock | 单次达到伤害阈值时生成小范围冲击波 | 2 |
| Slide Loader | 滑铲拾取累计 5 Shard，恢复副射击冷却 | 1 |
| Airborne Circuit | 滞空击杀提供一次有限时长的额外跳跃刷新 | 1 |
| Redline | 连射提高射速与散布；满热量继续开火产生可预警自伤 | 1 |
| Cold Start | 停火 1.5 秒后的第一发获得强击退或短减速 | 1 |
| Blood Pickup | Shard 拾取半径缩小，满计数后恢复少量生命 | 2 |
| Last Chamber | 低生命下每个射击周期的最后一发获得穿透 | 1 |
| Void Mark | 首次命中标记；击杀标记目标时产生短距牵引 | 1 |
| Eclipse | 处于定义的黑暗区域时提高伤害并关闭小地图提示 | 1 |

所有数值为首轮测试起点，不视为最终平衡。`Eclipse` 在 Phase 2 使用竞技场内预设 dark zone，Phase 4 再接入异常规则。

## 9. 任务拆解

### P2-01：建立稳定 gameplay context 与 EffectQueue

依赖：Phase 1。

工作：

- 为 run、enemy、shot、pickup 分配单调递增的本局 ID。
- 定义六类窄 context 和 `EffectCommand` variant。
- 延迟执行增删实体、伤害、击退、资源恢复和 cue。
- 加入 generation、来源 flags、每 tick 预算和稳定排序。

验收：同一输入事件序列产生字节等价的 command 摘要；容器扩容不改变效果顺序。

### P2-02：数据化 WeaponDefinition

依赖：P2-01。

工作：

- 将 Needler 的射速、伤害、散布、弹速、Charge 消耗迁移到 JSON。
- 分离基础数值、经 Augment 修正后的快照和运行时冷却。
- 热重载仅限 Debug，重载失败时保留上一份合法定义。
- 增加无 OpenGL 的定义验证测试。

验收：修改数据并重启可改变武器；重开后不会保留前一局热量或冷却。

### P2-03：实现 Scatter Cannon 与 Rail Lance

依赖：P2-02。

工作：

- 实现三把武器统一输入接口与切换生命周期。
- 定义散射确定性、蓄力取消、穿透命中顺序和反冲。
- 为每种攻击输出不同 muzzle、impact 和 audio cue ID。
- 在调试摘要记录武器使用时间和命中率。

验收：三把武器在无 Augment 时已有不同的目标距离和移动策略；相同 seed 下散射方向一致。

### P2-04：加载并验证 AugmentDefinition

依赖：P2-02。

工作：

- 加载 12 个定义，建立 ID registry。
- 验证 tag、稀有度、max level、requires 和 conflicts。
- 检测未知引用、重复 ID、无可用武器和循环前置条件。
- 输出每把武器的兼容奖励清单。

验收：任何错误定义都能定位到文件和字段；三把武器各有至少 6 个兼容选项。

### P2-05：实现 AugmentSystem

依赖：P2-01、P2-04。

工作：

- 保存已选 Augment、等级与每项运行时计数器。
- 按固定顺序执行 stat modifier 和 gameplay hook。
- 每次武器、Augment 或玩家条件改变时重建 stat snapshot。
- 重开时清空所有状态；结果页保留只读 snapshot。

验收：获取顺序不会让可交换的 stat modifier 产生不同结果；不兼容效果不能被直接注入。

### P2-06：实现前六个基础 Augment

依赖：P2-05。

范围：Ricochet、Split Core、Heavy Chamber、Aftershock、Slide Loader、Airborne Circuit。

工作：

- 每个效果先写单元测试，再接视觉和音频 cue。
- 对墙反弹、穿透、范围命中和额外跳跃定义唯一触发点。
- 增加派生攻击计数、每秒效果数和伤害贡献统计。

验收：Ricochet + Split Core + Aftershock 不会无限递归；六项都能从画面行为中辨认。

### P2-07：实现 RewardDraft

依赖：P2-04。

工作：

- 使用 `rewards` RNG 子流生成候选。
- 实现兼容权重、防重复、冲突、满级排除和保底规则。
- 每局一次 reroll，结果和消耗写入 RunStats。
- 提供固定 seed 的候选序列快照测试。

验收：对每把武器运行 10,000 次模拟，没有少于三项、全不兼容或冲突候选。

### P2-08：实现 BitmapFontRenderer 与 RewardScreen

依赖：P2-07。

工作：

- 渲染 ASCII 标题、名称、最多三行说明、稀有度和标签。
- 奖励状态暂停逻辑更新，但 UI 输入和动画继续。
- 支持鼠标与 `1/2/3` 选择、`R` reroll、`Esc` 返回。
- 在 framebuffer 坐标下布局，验证 Retina 和 16:9/16:10。
- 卡片选中后先提交状态，再播放短演出，避免重复点击。

验收：最长文本不越界；窗口缩放后点击区域与显示一致；一次输入只选择一次。

### P2-09：实现剩余六个风险/状态 Augment

依赖：P2-05、P2-08。

范围：Redline、Cold Start、Blood Pickup、Last Chamber、Void Mark、Eclipse。

工作：

- 明确热量、自伤、低生命、标记和 dark zone 状态机。
- 自伤必须有至少 0.4 秒视觉/声音预警，且不能绕过死亡统计。
- Void 牵引只影响活着的敌人，不穿越不可通行墙体。
- HUD 显示 Redline、Cold Start 和 Void Mark 的必要状态。

验收：条件效果开始和结束均可读；Eclipse 关闭小地图时不移除敌人伤害预警。

### P2-10：完成构筑反馈与结果摘要

依赖：P2-06、P2-09。

工作：

- 选择后显示名称、主要行为变化和对应 cue。
- HUD 用图标显示已选 Augment 与等级，不常驻长文本。
- 结果摘要列出武器、标签、各效果伤害/触发数和死亡来源。
- 增加一次标签组合触发的“中场进化”占位演出；首版只改变表现，不新增第 13 个效果。

验收：测试者无需查表即可说出本局至少一个核心联动。

### P2-11：平衡模拟与内部测试

依赖：P2-03、P2-10。

工作：

- 为三把武器记录单目标击杀时间、3 秒群伤和 Shard 回收率。
- 固定三个奖励时点，例如 75、165、255 秒。
- 为四组目标协同建立实验卡。
- 每把武器至少完成 5 局人工测试，记录失败来源和选择时间。

目标协同：

1. Needler + Ricochet + Split Core：持续弹幕与空间利用。
2. Scatter Cannon + Aftershock + Slide Loader：贴身冲击与移动回收。
3. Rail Lance + Heavy Chamber + Void Mark：高价值穿透与聚怪。
4. Needler + Redline + Blood Pickup：高风险持续输出和危险回血。

验收：四组都能形成不同操作节奏，且没有一组同时消除瞄准、走位和资源风险。

## 10. 自动测试

`make -f Makefile.map test` 至少覆盖：

- 武器和 Augment JSON 的合法/非法样例。
- stat modifier 的顺序、等级和重置。
- EffectQueue 稳定顺序、递归深度和预算。
- Ricochet、Split、Aftershock 的组合边界。
- RewardDraft 在固定 seed 下的快照、兼容保底和 reroll。
- 暂停奖励界面时逻辑时间、Shard ttl 和敌人状态不前进。

建议额外提供 `reward_sim`：按武器和 seed 批量输出候选分布、无效候选率和稀有度分布。

## 11. 人工测试脚本

每轮保持同一地图和敌潮：

1. 三把武器各无升级运行 2 分钟，描述其目标距离和移动倾向。
2. 为四组目标协同各运行一局，确认第三次选择前形成方向。
3. 故意触发满热 Redline、低生命 Last Chamber 和 Blood Pickup，检查风险是否提前可知。
4. 在奖励出现前持续按键，确认不会误选或多次选择。
5. 在 Retina、1920x1080、1280x720 下检查文本、卡片和 HUD。
6. 连续重开 30 次，检查效果计数器、派生子弹和奖励状态泄漏。

记录：首次选择时刻、选择用时、构筑描述、效果伤害占比、未理解伤害来源、主动重开意愿。

## 12. 建议提交边界

1. `Add deterministic gameplay effect pipeline`
2. `Data-drive weapons and add three weapon archetypes`
3. `Add augment definitions and runtime hooks`
4. `Add deterministic reward draft and reward UI`
5. `Implement twelve gameplay augments`
6. `Add build summary and phase two tests`

每个提交都应能编译；不要把 12 个效果和奖励 UI 压进同一个提交。

## 13. 完成定义

- 3 把武器、12 个 Augment 全部可通过正式流程获得。
- 三选一永远提供至少一个兼容项，reroll 可复现。
- 至少 4 组协同通过人工测试，能改变操作而非仅改变 DPS。
- 派生效果不存在无限循环或跨局状态泄漏。
- 奖励界面在 macOS Retina 与 Windows 常用分辨率下可读、可点。
- 自动测试和 10,000 次奖励模拟通过。
- RunStats 能区分武器和各 Augment 的伤害与触发贡献。

## 14. 阶段退出门

只有在测试者能于第三次选择前描述构筑，并且 Pressure Arena 的基础可读性没有因效果数量明显下降时，才进入 Phase 3。若奖励很丰富但战斗选择趋同，应先删减或重做效果，不增加房间内容。
