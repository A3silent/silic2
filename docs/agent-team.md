# Silic2 六角色团队索引

角色配置是职责的唯一来源；本页记录查找入口、交接和验证状态。
当前里程碑见 [两分钟样片计划](../plan/two-minute-sample.md)。

## 聊天与角色

项目：`silic2`，project ID `d88b2ae3-dee5-4525-9461-d1e3aaab0081`。
专业聊天均使用此项目的本地工作区。ATLAS 保留原聊天和历史，不另建副本。

| 编号 | 聊天标题 / 别名 | 配置 | 聊天 ID | Host |
| --- | --- | --- | --- | --- |
| 1 | Silic2 · 01 ATLAS｜策划统筹 | 主 agent；见 AGENTS.md | `01a111a8-5950-7f32-9fd9-fe2346fd0761` | local |
| 2 | Silic2 · 02 VECTOR｜玩法编程 | `.codex/agents/silic2_gameplay.toml` | `01a11546-c363-7210-bb00-bb94be40552e` | local |
| 3 | Silic2 · 03 PRISM｜界面设计 | `.codex/agents/silic2_ui.toml` | `01a11546-c75d-7793-9abd-0f17f7dd5683` | local |
| 4 | Silic2 · 04 NEBULA｜场景美术 | `.codex/agents/silic2_environment.toml` | `01a11546-cc0a-7502-84f2-d9b174237024` | local |
| 5 | Silic2 · 05 KINETIC｜角色表现 | `.codex/agents/silic2_character.toml` | `01a11548-8699-7362-bdd2-baf5c377fbb8` | local |
| 6 | Silic2 · 06 ECHO｜音频设计 | `.codex/agents/silic2_audio.toml` | `01a11548-8956-7501-9579-5820d4e97efc` | local |

编号、英文别名、中文职责均可用来查找和派单。五份 TOML 省略模型、推理、
权限覆盖，继承实际会话配置；独立聊天读取对应 TOML，不假定标题自动应用角色。

## 双入口交接

独立聊天承接持续专业工作，临时子 agent 承接明确的小任务。它们不自动共享完整
历史。ATLAS 在用户明确交代的开发任务内负责派单、追问和收集；专业聊天可在
同一协调流程中回复 ATLAS。团队初始化只读，不开始实现。

每次派单写清：任务 ID、目标、执行入口、允许写入文件、依赖、验收、交付对象。
同一任务只保留一个执行入口。共享文件先登记负责人再编辑；当前会话临时 agent
容量为 4（含主 agent），未来按实际工具容量调度，不把六角色当成六并发保证。

公共 App、构建和跨模块接口由 VECTOR 集成。其他角色提交需求、资源与接口，
不与 VECTOR 并发编辑同一文件。ATLAS 维护下表；所有角色保持用户已有改动。

| 任务 | 执行入口 | 写入负责人 / 文件 | 状态 |
| --- | --- | --- | --- |
| TEAM-SETUP | 当前 ATLAS 聊天；VECTOR 执行只读原生角色验证 | ATLAS：AGENTS.md、五份角色 TOML、本索引、两分钟样片计划 | 已完成；未派发游戏开发任务 |
| SAMPLE-INTEGRATE | VECTOR 专业聊天 | `include/src/engine`（map_renderer 除外）、`include/src/game`、`include/src/enemy`、`include/src/player`、Makefile.map、现有 effects、玩法与集成 tests | 已集成；Debug/Release、最终三局性能、压力、50 次重开及流程检查通过 |
| SAMPLE-UI | PRISM 专业聊天 | `include/hud`、`src/hud`、HUD 专属 shaders、`tests/hud_*` | 已集成；逻辑、两尺寸 Retina 预览与实际流程检查通过；人类可读性待试玩 |
| SAMPLE-ENV | NEBULA 专业聊天 | `include/environment`、`src/environment`、`include/engine/map_renderer.h`、`src/engine/map_renderer.cpp`、新增场景 shaders、`res/shaders/pixel.frag`、`res/maps/starry_crystal_arena.json` | 已集成；星空、晶体、异变与像素后处理完成，实际 GL 截图及压力检查通过 |
| SAMPLE-CHAR | KINETIC 专业聊天 | `include/character`、`src/character`、新增角色 shaders；既有 enemy/weapon 文件仅提交接口需求 | 已集成；角色、武器与动作反馈完成，真实 GL 预览及整局重置通过 |
| SAMPLE-AUDIO | ECHO 专业聊天 | `include/audio`、`src/audio`、`res/audio`、`tools/audio`、`third_party/miniaudio`、`tests/audio_*` | 已集成；离线、真实设备、整局暂停/重开通过；耳机人测待完成 |
| SAMPLE-ACCEPT | ATLAS 当前聊天 | `docs`、`plan`、README.md、README_CN.md、`.gitignore`、`tools/acceptance` | 已完成开发协调、独立证据复算及交付记录；人工试听与五人试玩待完成 |

VECTOR 内部的短任务细分（父任务仍为唯一对外执行入口）：

| 短任务 | 唯一写入范围 | 集成负责人 |
| --- | --- | --- |
| SAMPLE-DIRECTOR | `include/game/arena_definition.h`、`src/game/arena_definition.cpp`、`include/game/encounter_director.h`、`src/game/encounter_director.cpp`、`tests/encounter_director_test.cpp` | VECTOR |
| SAMPLE-ENEMIES | `include/enemy`、`src/enemy`（含 enemy_projectile_system）、`tests/enemy_combat_test.cpp` | VECTOR |
| SAMPLE-STATS | `include/game/gameplay_types.h`、`include/game/run_stats.h`、`src/game/run_stats.cpp`、`tests/run_stats_test.cpp` | VECTOR |

上述短任务执行期间，VECTOR 本体只读取其文件并负责 App/main/Makefile/player 接入；
任务交付后再接管集成修改，避免父子同时写入。

2026-10-07 用户已授权完整实施两分钟样片，日常取舍交 ATLAS。以上实施任务覆盖
初始化只读限制，但不扩大样片内容范围。专业角色只编辑自己的文件；共享接口先
向 VECTOR 报送，由 VECTOR 集成。需要额外文件归属时先交 ATLAS 协调。

## 验证记录

2026-10-07 完成以下检查：

- 配置解析：Python `tomllib` 成功解析五份配置，均只有 `name`、
  `description`、`developer_instructions` 三个非空字段；角色名与文件名一致，
  指向的项目规则、团队索引和样片计划均存在。
- 聊天创建：五个专业聊天均通过已有 silic2 项目的本地环境创建，未覆盖模型
  或推理设置。上表 ID 来自创建结果；读取聊天确认名称、工作区及接入完成。
  ATLAS 仅重命名，聊天 ID 和历史保留。
- 专业聊天只读自检：五个聊天全部完成，正确说明角色职责、VECTOR 的共享代码
  集成责任、一文件一写入者规则，以及两分钟样片的范围和延后项。
- 新会话发现与调用：五个专业聊天的原生子 agent 工具均列出全部 `silic2_*`
  角色。VECTOR 再按 2＋2＋1 分批调用原生 `agent_type`，使用空历史启动，
  不在任务提示中粘贴角色职责，也不覆盖模型和推理设置。五个临时 agent
  均报告在读取文件前已收到角色 developer instructions，并核对与对应 TOML
  一致。五份项目级角色配置已验证生效。
- 原有文件完整性：配置前记录的 553 个已有文件 SHA-256 全部一致；本次只新增
  团队配置及说明，未修改游戏代码、启动素材制作或覆盖原有改动。

原生调用证据位于 VECTOR 聊天的 TEAM-SETUP 验证回合；以下临时标识以该聊天
为作用域，不作为跨聊天地址使用：

| 原生 agent_type | 返回的临时 agent 标识 | 加载身份 | 结果 |
| --- | --- | --- | --- |
| `silic2_gameplay` | `/root/verify_gameplay` | 02 VECTOR | 通过 |
| `silic2_ui` | `/root/verify_ui` | 03 PRISM | 通过 |
| `silic2_environment` | `/root/verify_environment` | 04 NEBULA | 通过 |
| `silic2_character` | `/root/verify_character` | 05 KINETIC | 通过 |
| `silic2_audio` | `/root/verify_audio` | 06 ECHO | 通过 |

配置被解析、角色被自动发现、专业聊天实际读取角色说明是分别验证的事项。
当前已开启的主会话不假定能够热重载新角色；需要时由新会话加载配置。

## 样片交付状态

2026-10-07，六角色实施任务均已交付并集成，本轮文件写入任务关闭。
ATLAS 独立复算最终性能 CSV、重开 JSONL 和二进制 hash；开发与自动化工程
验收通过，细节及剩余人工门槛见 [验收记录](sample-acceptance.md)。
保留既有工作区改动，未自动提交 Git。后续修改重新登记文件所有权；人工试玩
完成前不将整个里程碑标记为全部验收通过，也不提前扩大到武器奖励阶段。

## 地图与编辑器升级（2026-10-07）

用户授权系统修改地图模块，并同步改进相邻 `silic2_map_editor` 的操作。
现有加载成功行为、旧地图与游戏射击/碰撞规则作为兼容基线。

| 任务 | 执行入口 | 本轮唯一写入范围 | 状态 |
| --- | --- | --- | --- |
| MAP-CORE | VECTOR | 游戏地图/JSON 共享数据模块、game/arena/director、enemy/player/App/HUD 数据接入、Makefile 和相关测试；`res/maps` 的格式迁移 | 已交付；最终 Debug/Release、流程、两程序往返及三局性能回归通过 |
| MAP-EDITOR | PRISM | `/Users/ritzsun/Desktop/game/opengl/silic2_map_editor` 全部项目代码、CMake、测试及使用说明；共享游戏数据模块只读 | 已交付；3/3 CTest、旧图/v2 实际 GL 合成输入与两尺寸界面检查通过 |
| MAP-ENV | NEBULA | 游戏 environment、map_renderer 及专属环境 shader；不编辑 res/maps 和公共数据定义 | 已交付；CPU、shader与真实GL验证通过 |
| MAP-ACCEPT | ATLAS | 游戏 docs/plan、README、tools/acceptance；跨程序证据与验收 | 已完成独立核对、逐帧性能复算及证据归档；见地图升级记录 |

同一共享地图模块同时供游戏与编辑器构建使用，不复制第二套格式实现。
仅允许各角色写入上表范围，跨文件需求先交 ATLAS 或对应负责人。

MAP-CORE 内部短任务（公共外部入口仍是 VECTOR）：

| 短任务 | 唯一写入文件 | 集成 |
| --- | --- | --- |
| MAP-JSON | `include/engine/simple_json.h`、`src/engine/simple_json.cpp`、`tests/simple_json_test.cpp` | VECTOR，交付后接管 |
| MAP-NAV | `include/src/game/arena_definition`、`include/src/enemy/enemy` 与 `enemy_manager`、`tests/map_navigation_test.cpp` | VECTOR，交付后接管 |

VECTOR 本体同时只写 `map.h/map.cpp`、地图迁移、App、Makefile/test_main 和地图测试；
子任务完成前不修改它们的文件。

用户短暂停止后因网络恢复要求继续；MAP-CORE 与 MAP-EDITOR 恢复原范围。
NEBULA 已交付部分保持不动。旧失败 flow/编辑器 smoke 仅作诊断，最终验收须使用修复后证据。

MAP-CORE 的 HUD 集成范围包括 `include/hud`、`src/hud` 及 `tests/hud_*` 中
模式和异变提示所需的改动：legacy 显示清场/探索目标，竞技场保留 90 秒敌潮，
异变提示读取地图的开始/时长及环境启用状态。PRISM 本轮只编辑独立编辑器。

本轮已交付，写入任务关闭。[升级记录](map-editor-upgrade.md) 包含操作入口、
验收矩阵、最终二进制与输入地图校验、截图和三局性能数据。人工键鼠易用性与
跨平台运行尚未验证；原样片的人工试玩门槛仍保留。

## GitHub 交付（2026-10-07）

用户要求上传当前成果。MAP-PUBLISH-EDITOR 由 PRISM 唯一负责
`tools/map_editor/**`，将相邻编辑器的构建所需源码和 ImGui 依赖纳入本仓库，
调整默认共享模块位置并验证新目录构建与 CTest。ATLAS 负责主文档链接、
忽略规则、证据归档、提交与推送。相邻旧入口保留，不上传本地构建产物或嵌套 Git 历史。

源码导入已完成，仓库内编辑器构建及 3/3 CTest 通过；交互代码与已验收的相邻
版本相同。本次交付使用现有 `macos-port` 分支。
