# Silic2 六角色团队索引

角色配置是职责的唯一来源；本页记录查找入口、交接和验证状态。
当前里程碑见 [两分钟样片计划](../plan/two-minute-sample.md)。

## 聊天与角色

项目：`silic2`，project ID `d88b2ae3-dee5-4525-9461-d1e3aaab0081`。
专业聊天均使用此项目的本地工作区。2026-11-12用户指定当前聊天接任ATLAS；
原“Silic2 · 01 ATLAS｜策划统筹”聊天及历史保留，ID为
`01a111a8-5950-7f32-9fd9-fe2346fd0761`。后续新任务交付使用下表当前入口。

| 编号 | 聊天标题 / 别名 | 配置 | 聊天 ID | Host |
| --- | --- | --- | --- | --- |
| 1 | 接替 ATLAS 任务（当前01 ATLAS／策划统筹） | 主 agent；见 AGENTS.md | `01a1d063-069e-7c42-93b4-186792653d6f` | local |
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

## 太空恐怖表现改造（2026-10-08）

用户要求更黑暗的太空恐怖与不可名状的怪物；本轮以
[美术方向](../plan/cosmic-horror-art-pass.md) 为共同规范。

| 任务 | 唯一执行入口 | 写入范围 | 状态 |
| --- | --- | --- | --- |
| HORROR-ENV | NEBULA | include/src/environment、map_renderer.h/.cpp、环境 shader、pixel.frag；地图 JSON 只读 | 已交付；两轮实际GL及几何/地图/重置检查通过 |
| HORROR-CHAR | KINETIC | include/src/character、character shader；玩法源码只读 | 已交付；第二版炮台、10,240组几何与最终六姿势GL通过 |
| HORROR-AUDIO | ECHO | include/src/audio、res/audio、tools/audio、tests/audio_* | 已交付；离线混音、生成一致性和预警定位检查通过；听感待人测 |
| HORROR-UI | PRISM | include/src/hud、HUD shader、tests/hud_* | 已交付；CPU与两尺寸8状态GL预览通过 |
| HORROR-INTEGRATE | VECTOR | App/main、Makefile、集成测试、res/maps 内容身份；其他角色交付后必要集成 | 已交付；最终Debug/Release、14张整合GL、实际流程10/10、旧图与三局性能通过 |
| HORROR-ACCEPT | ATLAS | plan、docs、README、AGENTS.md 和角色配置；跨角色验收 | 已完成视觉复核、逐帧性能独立复算及证据归档；人工试玩待完成 |

前三专业角色与主 agent 同时工作；后两角色随后执行，避免超过四个活动入口。
真实 GL/音频设备运行由 ATLAS 串行分配，性能测量独占运行窗口。

本轮文件写入任务已关闭，GPU时段释放；交付与验证边界见
[太空恐怖改造记录](cosmic-horror-acceptance.md)。此版尚未人工试玩与耳机评估，
不将自动化检查等同于主观恐怖感验收；未自动提交或推送本轮修改。

## 双竞技场（2026-10-08）

用户追加要求：保留太空恐怖地图及全部敌人改动，另提供原先水晶竞技场。
本轮将环境按地图预设选择；敌人、武器、音频和玩法维持当前版本。
`hollow_orbit_arena.json` 保存太空恐怖地图并继续作为默认入口；
`starry_crystal_arena.json` 恢复原水晶地图内容与表现。

| 任务 | 唯一执行入口 | 写入范围 | 状态 |
| --- | --- | --- | --- |
| DUAL-ENV | NEBULA | include/src/environment、map_renderer.h/.cpp、环境 shaders、环境验证程序与说明 | 已交付；两图各7张对各自基线逐像素一致，来回切换/reset通过 |
| DUAL-INTEGRATE | VECTOR | map.h/.cpp、res/maps、App/main、Makefile、HUD地图名称数据/标签、地图与集成测试、仓库内编辑器必要预设兼容 | 已交付；Debug/Release、两图各10项流程及各14张整合图、旧图、编辑器4/4通过 |
| DUAL-ACCEPT | ATLAS | docs、plan、README、AGENTS.md 方向说明及证据 | 已完成独立字节/像素/hash核对及双地图交付归档 |

两专业角色仅修改上表范围；环境与公共数据接口约定使用新增
`EnvironmentPreset::HollowOrbit`，已有 `StarryCrystal` 恢复原义。
实际 GL 运行先交 NEBULA，交付后由 VECTOR 集成运行，避免相互干扰。

本轮写入任务已关闭。保留34个敌人/武器/音频文件原样；一次首帧READY缺字
未在同程序重检复现，原图与重检均归档，未声称修复未知原因。
证据与边界见[双地图追加验证](cosmic-horror-acceptance.md#双地图追加验证)。

## Crystal 场景探索化（2026-10-13）

用户要求只修改Crystal Arena，增加主题化、多变空间，并允许超出纯射击框架。
依据[参考研究](crystal-world-references.md)，实施范围见
[失焦天文馆](../plan/crystal-observatory.md)。本轮保留全部既有未提交工作。

| 任务 | 唯一入口 | 写入范围 | 状态 |
| --- | --- | --- | --- |
| CRYSTAL-REFERENCE | 临时研究 agent crystal_scene_references | docs/crystal-world-references.md | 已交付；明确jdh指定片段不可访问 |
| CRYSTAL-WORLD | VECTOR | 地图公共数据、res/maps/starry_crystal_arena.json、探索流程、App/build、必要HUD数据与提示、tests、编辑器兼容 | 已交付；真实109m闭环、Debug/Release、两图流程各10/10与编辑器5/5通过 |
| CRYSTAL-ART | NEBULA | include/src/environment、map_renderer.h/.cpp、starry_crystal shaders、环境验证程序 | 已交付；两轮构图复核、碎弧投影与reset通过，Hollow画面一致 |
| CRYSTAL-ACCEPT | ATLAS | plan/docs/README/AGENTS、证据归档与视觉审阅 | 已完成；独立核对截图、原始性能CSV、保护文件及源码/二进制哈希 |

VECTOR唯一编辑地图JSON；NEBULA先提供空间与锚点需求。两者共享数据接口先对齐。
角色/武器/音频文件只读。GL先由NEBULA预览、完成后交VECTOR正式整合与性能测量。

ATLAS追加允许VECTOR对 `include/player/player.h`、`src/player/player.cpp` 提取
值式按键输入入口，使GLFW与合成WASD走同一移动/碰撞分支。仅输入接入为保护
基线例外，移动参数与规则保持，Player内部不加入验收专用玩法或传送分支。

CRYSTAL本轮任务已关闭，GPU已释放。最终记录见[探索验收](crystal-observatory-acceptance.md)。
性能门槛通过但保留326ms单帧峰值，原因未定位；初次探索与可读性仍待人类试玩。
本轮未提交或推送，不修改Hollow默认入口与既有敌人/武器/音频。

## Crystal 立体战斗地图（2026-10-13，访谈确认后）

用户已确认[完整实施范围](../plan/crystal-combat-world.md)：五区持续战斗、双层坡道，
战术与空间探索同等重要；90秒敌潮＋清场、小地图原样、纯视觉异变、混合建模流程。
取代上一轮Crystal解谜推进。保留全部已有改动，基线位于artifacts/acceptance/crystal-combat。

| 任务 | 唯一入口 | 写入范围 | 状态 |
| --- | --- | --- | --- |
| CRYSTAL-SPACE | VECTOR | Map/资产/空间查询/碰撞/导航、player/enemy/weapon必要接入、App/HUD数据/Makefile/tests；Hollow表现只读 | 已交付：真实三角空间、普通输入巡路/实战、Debug/Release及两图回归通过 |
| CRYSTAL-WORLD-ART | NEBULA | Crystal地图JSON、res/models/crystal_combat与生成工具、environment/map_renderer、Crystal shaders与预览 | 已交付：五区地形、7类OBJ、实际GL构图/异变与Hollow一致性通过 |
| CRYSTAL-EDITOR | PRISM | tools/map_editor/**、编辑器测试及其文档 | 已交付：混合编辑、7/7两构建测试、实际UI及两尺寸布局通过 |
| CRYSTAL-COMBAT-ACCEPT | ATLAS | plan/docs/README/AGENTS、审阅和证据归档 | 已完成：独立图像/CSV/hash复核，证据归档 |

本轮地图JSON唯一作者改为NEBULA，VECTOR通过约定交接请求地图调整。
VECTOR可以为新几何修改移动/敌人/射线查询实现，参数与战斗手感目标保持。
角色/武器美术、音频、Hollow地图/shaders、全局pixel后处理只读。
三专业聊天＋ATLAS为四个活动入口；先CPU/资产工作，GPU时段由ATLAS串行分配。

任务均已关闭，GPU已释放。工程验收见[交付记录](crystal-combat-acceptance.md)。
保留两轮约400ms孤立长帧，原因未定位；人工认路/暗色怪物可读性/战斗趣味待试玩。
本轮不提交或推送，旧工作区改动全部保留。

## Crystal 材质贴图（2026-10-13）

用户认可立体地图，授权ATLAS评估贴图替换并直接交NEBULA生成。

| 任务 | 唯一入口 | 写入范围 | 状态 |
| --- | --- | --- | --- |
| CRYSTAL-TEXTURES | NEBULA | 新增res/texture/crystal_combat/**与素材说明/提示词；Crystal shaders；map_renderer.h/.cpp必要贴图绑定；环境验证程序及纹理验证说明 | 已交付：四类imagegen贴图、真实采样/回退/异变/保护文件及构建验证通过 |
| CRYSTAL-TEXTURE-REVIEW | ATLAS | 团队记录、材质取舍、交付审阅 | 已完成：原图/重复预览/实际角色画面及哈希复核 |

四类表面：地板/坡道金属板、浅色矿化陶瓷巨构、深色侵蚀外壳、低对比晶体裂理。
NEBULA使用内置imagegen生成原创位图（用户本轮授权覆盖旧角色配置仅程序化资产偏好），
保留原图/提示词与可供游戏加载的版本。保持几何、碰撞、导航、地图布局、敌人/武器/
音频和Hollow不变。当前仅NEBULA持有GPU验证时段；其他专业任务已关闭。

贴图任务已关闭，GPU释放；素材与提示词见res/texture/crystal_combat，
[取舍与验收](crystal-texture-pass.md)。本轮未修改地图几何/玩法，未提交或推送。

## Tripo 静态资产接入（2026-10-17）

用户授权使用已有免费额度生成宇宙步枪、坠毁飞船、生物骨架、水晶簇和碎石装饰，
接入模型工具、Crystal 摆放及武器外观替换；射击规则保持。资产仍只保留本地，
本轮不提交或推送。现有未跟踪 interview-project-summary.md 与 codex-tripo-research.md 保留。

| 任务 | 唯一入口 | 写入范围 | 状态 |
| --- | --- | --- | --- |
| TRIPO-IMPORT | VECTOR 临时子 agent | tools/assets/**、共享 mesh_asset / map 所需最小接口、对应 tests、Makefile | 已交付：18项离线导入测试及完整 Debug CPU 测试通过 |
| TRIPO-ART | NEBULA 临时子 agent | tools/tripo/prompts/**、Crystal JSON、场景专属渲染及四件环境资产 | 已交付：船体3块、骨架残段、墙冠水晶3处、浅裂片3处；GL及Hollow环境基线通过 |
| TRIPO-WEAPON | KINETIC 临时子 agent | include/src/character/**、character shaders、角色预览与测试、武器本地配置 | 已交付：真实步枪拟合启用，六姿态/CPU/GL验证通过 |
| TRIPO-ACCEPT | ATLAS 当前聊天 | 额度核实、Tripo调用/下载与asset-plan状态、docs、基线与最终验收 | 五件各40credits，总200、剩800；独立图像/CSV/hash复核完成，保留异常记录 |

三位执行者并行时只做 CPU/文件工作，实际 GL 时段由 ATLAS 串行安排。共享接口
由 VECTOR 发布；NEBULA 和 KINETIC 消费同一导入结果。没有实际生成资产前，
不得将占位模型标为 Tripo 成果，也不把缺失资源写入正式地图。

用户追加要求：全部提示词完成后先供其确认，再启动任何生成。生成调用、消费额度、
真实资产摆放和武器启用均保持待办。提示词详细描述造型意图，但模型适配320×200
渲染，以轮廓、大中尺度结构和明暗分区为主，不追求微细节或高面数。

本地接入 Debug / Release 构建通过；导入和武器 CPU/合成 GL 检查通过。
场景 authored UV 采样验证通过，30组原材质结果与 HEAD 逐像素一致。
19个玩法/敌人/音频/Hollow保护文件及 Crystal 地图哈希不变。
这些结果仅验证接入能力，不代表真实 Tripo 模型、摆放、握持和性能验收已完成。

2026-10-17 用户以“开始”确认低分辨率版五组提示词。ATLAS核对全部文本哈希，
记录批准；实际 balance/whoami 均返回国际站 default 账号余额0、冻结0。
当前仅等待领取额度所对应的账号/站点澄清，提示词无须再次批准，未提交生成任务。

随后用户要求复查，API 已到账1000 credits、冻结0。ATLAS启动已批准五件资产，
唯一负责所有在线生成/额度调用及 tools/tripo/asset-plan.json 的任务状态。
本轮续作唯一写入分工：KINETIC负责步枪本地导入包、武器配置及 character 代码；
NEBULA负责其余四件本地导入包、Crystal JSON及 environment/场景渲染；
VECTOR负责导入工具修复、共享代码/构建与测试和最终性能检查。所有角色保留现有改动，
不得自行追加在线生成；场景摆放基于真实资源，GL时段仍由ATLAS串行授权。

VECTOR追加独占 tools/map_editor/src/map_editor_final.cpp、tests/editor_self_test.cpp
（编辑器目录下）、README.md/README_CN.md（编辑器目录下）：仅补充模型贴图路径字段与
保留/SaveAs测试，不扩展编辑器渲染器。ATLAS维护.gitignore以排除本地Tripo会话上下文。

本轮已交付并关闭所有执行任务，GPU已释放。最终Debug/Release、41点巡路、编辑器7项
测试及91.465秒实战通过；p95 6.185ms/p99 8.302ms，孤立218.146ms长帧保留。
Hollow首次流程失败后同版本三次通过，用户补充同期打开map editor并认为无实际游戏
问题，记录为用户报告的外部窗口干扰。详见[Tripo交付记录](tripo-asset-pass.md)。
素材保持本地，未提交/推送，人工手感和可读性反馈仍待试玩。

## Arena 内部模型掩体（2026-10-17）

用户明确追加：部分飞船、骨架和水晶放入竞技场内部，兼作掩体与装饰，并有实际碰撞。
该要求取代上一轮仅场外/不可达装饰的限制；复用已生成资源，不增加Tripo消费。
保留现有未提交工作、外部地标和新步枪，保持射击/移动数值、敌人、音频、Hollow不变。

| 任务 | 唯一入口 | 独占写入范围 | 验收 |
| --- | --- | --- | --- |
| INTERIOR-ART | NEBULA 临时子agent tripo_art | Crystal JSON（含导航建议的实际写入）、res/models/tripo内新增内部道具派生/简化碰撞代理与生成脚本、场景预览 | 船体/骨架/水晶均有内部可读实例，视觉/碰撞吻合，主要环路和坡道畅通 |
| INTERIOR-COLLISION | VECTOR 临时子agent tripo_import | 必要共享空间代码/构建/测试及编辑器兼容；地图修改通过NEBULA交接 | 玩家/敌人移动、射击/敌弹/LOS一致碰撞；绕行/出生点/功能验证（用户取消本轮性能测试） |
| INTERIOR-ACCEPT | ATLAS | 本记录、设计约束、资产清单状态和验收文档 | 审阅实际图片/变换/回归证据；资源仍仅本地，不提交推送 |

先协调约4–5处内部掩体的尺寸/位置及代理形状，再改正式地图。可替换局部旧掩体避免
无意义堆叠；不得封闭骨架明显开口或以整个包围盒制造空气墙。主通道尽量保持4米，
坡口/桥下净空/出生点和Sentry射击空间受保护。需要改导航/巡路时由VECTOR给出具体
节点与连接建议，NEBULA作为地图唯一作者落地。实际GL由ATLAS串行分配。

内部掩体本轮已交付：31船体、32骨肋及4400/4401两簇水晶正式落图，带局部闭合
碰撞代理；85导航点/167连接。NEBULA与VECTOR写入已冻结。Debug/Release及CPU通过，
内部61项检查、41点普通巡路、编辑器7项均通过，短时GL图片已复核。用户明确取消
本轮性能测试，未执行性能基准。旧低掩体的站顶Chaser停追问题对照复现并记录，
未扩展AI修改。详见[内部掩体交付](crystal-interior-cover.md)。无新增Tripo消费、无提交推送。

## 巨型模型墙体修订（2026-10-17，当前任务）

用户澄清上一轮尺度理解有误：参考截图白色巨骨，将飞船/骨架/晶体作为巨型地图墙体和
空间结构，不是地面小掩体。本轮以此取代四处低矮模型的摆放方案；复用真实源模型，
使模型主体形成竞技场内可接近、可碰撞的墙面、转角与通道轮廓。建议8–16m高、
12–24m跨度，以实际玩家视角而非绝对数值验收。不得仅把它们放到墙外做远景。
允许替换/融合局部旧墙，保留主环路、上下坡与出生点；不改变武器/敌人/音频/Hollow。
不做性能测试，不调用Tripo，不提交推送。

| 任务 | 唯一入口 | 文件所有权 | 验收/交付 |
| --- | --- | --- | --- |
| MEGA-ART | NEBULA 临时 tripo_art | Crystal JSON、res/models/tripo/巨型场景派生/碰撞资源、场景预览 | 先出玩家高度实景候选，再定稿巨大船壳/骨墙/晶壁，交ATLAS |
| MEGA-FUNCTION | VECTOR 临时 tripo_import | 必要共享代码/测试/Makefile；导航改动交NEBULA写 | 移动/射线/通路/编辑器功能，禁止性能测试，交ATLAS |
| MEGA-ACCEPT | ATLAS | 本索引、docs、asset-plan及验收 | 审查尺度/实际可接近性/证据 |

同一文件唯一作者；保留既有未提交工作。GPU仍串行，先NEBULA预览，后VECTOR短功能。

巨型墙体修订已交付：西船墙地上10m/跨度28m、东南骨墙11m、东北晶壁13m，
上一轮4小掩体移除。NEBULA资源/地图冻结；VECTOR正式90项功能、41点巡路、editor7项
通过，Debug/Release构建和短GL验证完成。ATLAS复核玩家高度图、模型哈希与保护文件。
不做性能测试；没有新增Tripo消费、提交或推送。详见[巨型结构交付](crystal-megastructure.md)。

## 摄像机侧移倾斜（2026-10-31，当前任务）

用户要求左右移动时镜头微倾，急刹有动态反馈。ATLAS将本轮理解为轻微侧移roll与
停步的短促惯性回摆，中心瞄准方向/玩家物理不受表现倾斜改变。不做性能基准。
CAMERA-LEAN唯一实现入口为VECTOR临时tripo_import：独占Camera、App相机接入、
必要GameConfig字段、对应测试/Makefile；ATLAS维护本记录与交付说明。若需要
修改角色viewmodel代码先交接所有权。现有src/player/weapon.cpp中子弹速度195
为本轮前已有改动，必须保留；所有地图/资源/敌人/音频改动保持，不提交推送。
验收左右方向、平滑回摆、帧率适应、暂停/重启复位与中心射线一致性，短GL确认。

CAMERA-LEAN已完成：Camera视图独立roll，实际侧移±3°、停步轻微回摆，暂停冻结/重启归零。
VECTOR实现与构建冻结；Debug/Release完整测试和短GL5项通过，ATLAS审阅左右图并确认9份
保护文件哈希保持。实际调参入口为CameraConfig默认字段，不宣称JSON自动加载。
详见[摄像机侧移倾斜](camera-movement-lean.md)。未做性能测试、未提交推送。

## 森林巨晶竞技场（2026-10-31，当前任务）

已批准概念图实施，范围见plan/forest-crystal-arena.md；取代Crystal旧布局。旧图保留独立文件。
| 任务 | 唯一执行入口 | 独占文件/职责 |
| --- | --- | --- |
| FOREST-ENV | NEBULA tripo_art | Crystal地图与旧图备份、forest场景生成器、environment代码/着色器、map_renderer场景渲染、场景预览；消费已交付Tripo模型 |
| FOREST-CORE | VECTOR tripo_import | 共享Map/配置/App/空间接口/导入工具、Makefile/tests、必要editor兼容；地图导航/出生建议交NEBULA写 |
| FOREST-ASSETS | NEBULA bounded forest_assets | tools/tripo/forest-prompts/**；批准生成后res/models/tripo/forest/**模型离线导入、尺寸/碰撞/来源记录 |
| FOREST-ATLAS | ATLAS | plan/docs、asset-plan、Tripo在线调用/余额/下载、审核与最终验收 |

先提供接口契约、模型尺寸/方向，再并行实现。只有ATLAS可发Tripo请求；保留用户先审
全部prompt的要求。前三类树+中央晶柱首批，云眼/图腾按渲染表达，不盲目增加收费建模。
不运行性能基准。现有未提交工作和子弹速度195、camera lean保留。GPU时段由ATLAS串行分配。

FOREST已完成并冻结：四件已批准Tripo模型真实生成/导入，共160 credits，余额640；
48株三类松树、140米巨晶、森林边界、云眼与图腾接入。最终96项功能、编辑器往返、
App流程10项与实际GL检查通过；修复可见根丛斜肩导致的跳跃越界，未改玩家物理。
ATLAS审阅正式游戏与环境截图、核对原图/保护文件与提示词哈希。详情见
[森林巨晶竞技场交付](forest-crystal-arena.md)。所有写入任务关闭、GPU已释放。
不做性能测试，未提交推送；树根站立覆盖与大场地战斗压力保留人工试玩边界。

## 森林外围加密（2026-10-31）

FOREST-OUTER-RINGS唯一实现入口NEBULA临时tripo_art：独占forest场景generator、
Crystal地图JSON、layout-state/该资源README，必要environment远林布局与专属预览。
在已验收林缘之外添加三圈错落松树林，复用现有三件真实模型；保留现有61个模型、
碰撞边界、导航、出生、天空、巨晶和玩法。不调用Tripo、不做性能测试、不提交推送。
ATLAS维护本索引/交付文档并审图；NEBULA验证外圈纯装饰、原碰撞数据不变及实际视角。

FOREST-OUTER-RINGS已完成：新增三圈216株真实松树，总264株；原61模型及碰撞/
导航/出生与其他map字段保持。生成器重复一致、实际加载和前后三视角GL通过，
ATLAS审图并独立核对原记录与保护资源哈希。无需重编，无API消费、性能测试或推送。
NEBULA写入结束，GPU释放。

## 森林材质表现修订（2026-10-31）

用户要求星云渐变巨眼、半透明自发光及内部散射感水晶、扭曲像素符文，允许扩充引擎。
FOREST-MATERIAL-ENV：NEBULA tripo_art独占forest_sky.frag、forest_map.frag（地面符文）、
环境专属美术预览；不得并发改map_renderer。
FOREST-MATERIAL-CRYSTAL：NEBULA forest_assets独占新增forest_crystal透明材质shader；
与VECTOR约定uniform/pass契约，只做材质/内部发光表现。
FOREST-MATERIAL-ENGINE：VECTOR tripo_import独占map_renderer.h/.cpp、App/必要PixelRenderer、
Makefile及针对性透明遮挡/状态/切图检查；公共渲染接入由其集成。
ATLAS维护范围/证据/文档及串行GPU分配。地图、模型、碰撞、外围三圈森林和玩法保留。
不调用Tripo、不做性能测试、不提交推送。所有角色保留现有未提交工作，一文件一作者。

FOREST-MATERIAL已完成：NEBULA交付星云渐变云眼、扭曲像素符文及半透明自发光晶体；
VECTOR集成最近表面透明pass与独立发光。ATLAS审阅最终中近景及实际游戏截图，
接受蓝色内核和柔和散射层次。Debug/Release构建、14项渲染检查、10项App流程通过，
51份保护文件保持。所有写入任务关闭，GPU已释放。详见docs/forest-crystal-arena.md
及artifacts/acceptance/forest-material-pass/final-summary.json。未做性能测试、API生成或提交推送。

## 640×400与森林绘制优化（2026-10-31）

用户要求将离屏buffer改为640×400，并针对树木增加后卡顿优化外围森林。
FOREST-RENDER-OPT唯一实现入口VECTOR临时tripo_import，独占引擎渲染/配置、必要shader、
Makefile和针对性诊断/回归工具；先取得固定场景短时基线，再决定共享网格/实例化/裁剪方案。
ATLAS负责范围、证据和视觉验收；地图布局、模型源文件、碰撞、玩法和现有材质表现保留。
允许本次卡顿诊断所需短时测量，不运行完整压力测试。GPU独占交VECTOR。
不调用Tripo、不提交推送；保留全部既有未提交修改。

FOREST-RENDER-OPT已完成：VECTOR统一主游戏640×400，并对264棵松树共享三份GPU网格、
实例化绘制及保守视锥裁剪。固定场景索引draw279→18、buffer申请140→4.12MB（不含贴图
或总显存）；Debug/Release构建、资源14项和透明切图14项通过。ATLAS已审阅实际截图、
代码与原始测量，独立确认62份保护文件未变。短测不等同整局FPS；详见森林交付文档。
任务关闭，GPU释放，无完整压力测试、API消费或提交推送。

## 离屏分辨率微调为480×300（2026-10-31）

RESOLUTION-480唯一执行入口VECTOR临时tripo_import：仅调整RenderConfig与PixelRenderer
默认尺寸为480×300，重建Debug/Release并确认App读取实际尺寸。保留森林实例化和所有
现有改动；不运行性能测试、不提交推送。ATLAS负责交付记录。

RESOLUTION-480已完成：两个默认入口和说明注释已同步；App沿用实际尺寸接口。
Debug/Release增量构建成功，diff检查通过。未追加GL或性能测试，任务关闭。

## 水晶不透明度与红点开镜（2026-11-12）

用户要求晶体更实、暂时隐藏手部、放大武器并加入红点开镜。默认按住右键开镜，
松开平滑退出；可移动开镜，不额外改移动速度、射速或伤害。480×300和森林优化保留。

| 任务 | 唯一入口 | 独占文件/职责 |
| --- | --- | --- |
| CRYSTAL-DENSITY | NEBULA forest_assets | forest_crystal.frag及其材质说明；提高不透明度，保留内发光与有限透射 |
| RIFLE-ADS-ART | KINETIC 临时子agent | include/src/character、character shaders、weapon_presentation.json；无手、大枪、红点镜体与对中动画 |
| RIFLE-ADS-CORE | VECTOR tripo_import | App/输入/瞄准状态、必要Camera/Weapon、HUD最小集成、Makefile/tests；开镜FOV/灵敏度、生命周期与射击对中 |
| ADS-ACCEPT | ATLAS | 文档、基线、接口与GPU协调、实际画面验收 |

角色接口：FirstPersonView新增aimBlend（0..1），KINETIC唯一作者；VECTOR产生并传入。
KINETIC拥有镜体和红点表现，VECTOR只在ADS期间淡出普通准星。暂停冻结，重开/切图清空，
死亡解除；焦点恢复读取实际右键状态。共享文件由VECTOR集成，一文件一作者。
先CPU并行，GPU先KINETIC短预览，再VECTOR正式App；不做性能测试、Tripo调用或提交推送。

## Crimson Carapace程序步枪（2026-11-12）

用户批准最终黑红概念图，含顶部开放的\\_/镜框及中央悬浮红点，开始程序实现。
设计依据保存至artifacts/acceptance/procedural-rifle/approved-concept.png。
本轮替换默认第一人称步枪：分片黑甲、红色能量缝、曲弹匣、尖锐枪托/枪口、开放镜架，
无手；网格与材质全部由代码生成，默认路径不得依赖Tripo模型或贴图。

| 任务 | 唯一入口 | 独占写入范围 |
| --- | --- | --- |
| CRIMSON-ART | KINETIC rifle_ads_art | include/src/character、character shaders、weapon_presentation.json；新增程序网格、发光、必要局部bloom与持枪/ADS拟合 |
| CRIMSON-INTEGRATE | VECTOR tripo_import | Makefile/tests、必要App共享接入；Debug/Release和实际App验收，不改角色专属文件 |
| CRIMSON-ACCEPT | ATLAS | 文档、概念图/保护基线、视觉审核与GPU时段 |

保留480×300、森林优化、水晶不透明度、现有ADS控制/射击/移动/敌人/音频。
不做性能测试，不调用任何建模API，不提交推送。CPU可并行，GPU先KINETIC预览、
ATLAS审图后再VECTOR实际游戏。新模块接口先交接，避免重复实现与同文件多作者。

CRIMSON已完成：默认与缺配置路径均为纯代码新枪，1222tri/8材质/117312静态VBO字节，
程序黑甲、亮红芯/局部halo、开放U框及中央4像素红点均接入。ATLAS审初版并要求一轮
脊面叠甲/倒角/能量芯强化，最终专属图和正式App持枪/ADS/连射图均接受。
Debug/Release、实际builder/selector与ADS CPU通过，正式隐藏App合成路线17/17通过；
新材质160×100→480×300私有target resize/depth/state也通过。ATLAS独立核对128份保护
文件保持。详情见docs/procedural-crimson-rifle.md。代码任务关闭、GPU释放；没有性能测试、
建模服务调用或提交推送。

## 缩小瞄具与连续能量线（2026-11-12）

用户依据两张实机标注要求：整套瞄准镜尺寸缩小40%（保留60%），绿色圈内断续红槽
连成连续、自发光的能量线。红点保持相机中心和低分辨率可读性，枪身主体大小不变。
CRIMSON-REFINE-ART唯一入口KINETIC rifle_ads_art：独占character程序几何/瞄具拟合、
必要shader和专属预览；先复现标注断线，再修补连通与表面遮挡。
CRIMSON-REFINE-VERIFY唯一入口VECTOR tripo_import：独占tests/Makefile及共享集成，
验证新瞄具比例、红点中心、能量线连续性和最终App。ATLAS维护证据、审图与本记录。
保留现有所有改动；不改枪身比例、射击/移动/敌人/地图/音频，480×300不变。
先KINETIC专属GPU复现与预览，后VECTOR正式App，不做性能测试/API/提交推送。

CRIMSON-REFINE完成：原瞄具主体缩.6、窄支撑接枪、红点不漂；实际框宽57.65→34.03px。
修正独立端点和甲片遮挡，hip/ADS亮线8连通检查从19/63px缺口变为0，材质/bloom/depth
均未改。当前1288tri/8材质/123648静态VBO字节，枪身护甲三角集合保持。
模型对照5/5、程序枪/ADS CPU与Debug/Release、正式App17/17通过；ATLAS审正式图并独立
确认128保护文件保持。初次灰边分类误报已按真实scope几何排除并归档，不改生产代码迎合
测试。任务关闭，GPU释放，无性能测试/API/提交推送；详见程序步枪文档及本轮验收目录。

## 瞄具横向加宽35%与版本提交（2026-11-12）

用户要求当前瞄具宽度增加35%，测试后提交一个版本。本轮由接任ATLAS的当前聊天
`01a1d063-069e-7c42-93b4-186792653d6f`负责统筹，原ATLAS聊天和历史保留。
以当前已缩小的瞄具为基线，只将横向尺寸乘1.35，保留高度、纵深、红点形状/中心、
枪身及连续能量线、ADS控制和全部既有成果。用户本轮明确授权本地Git提交。

| 任务 | 唯一执行入口 | 独占写入范围 / 验收 |
| --- | --- | --- |
| SIGHT-WIDTH-ART | KINETIC临时sight_width_art | src/character/procedural_rifle.cpp；必要专属preview代码；对称横向加宽与实际hip/ADS图 |
| SIGHT-WIDTH-VERIFY | VECTOR临时sight_width_verify | tests/Makefile必要验证改动及本轮artifacts；另负责forest_map.vert提交前尾随空格清理；几何比例、红点、Debug/Release、现有CPU与实际App流程 |
| SIGHT-WIDTH-ACCEPT | 当前ATLAS | docs/plan、保护基线、审图与Git提交；沿用源码交付规则，模型/贴图/音频和构建产物留本地 |

KINETIC先运行专属GL，交付后VECTOR独占正式App检查；CPU可并行。不得覆盖现有未提交
成果，不新增API消费或性能基准。证据目录：artifacts/acceptance/crimson-sight-width/。

SIGHT-WIDTH已完成并关闭写入：瞄具投影宽34.03395→45.94584px（×1.35），高度/纵深/
红点不变，枪身3570顶点及连续能量线保持。几何8/8、Debug/Release构建与完整测试、
程序枪/ADS CPU及正式App17/17通过；ATLAS审阅专属对照和正式hip/ADS/移动连射图。
基线1049文件中仅7个已分派源码/文档有变化，其余1042个保持。GPU释放。
本次本地版本收录当前森林/巨构/导入、camera lean、ADS和程序步枪源码及验收记录；
模型/贴图/音频/构建产物留本地，未推送远端。无关面试笔记保持未跟踪。
精简证据：docs/acceptance/sight-width-2026-11-12.json；人工试玩边界继续保留。

## 瞄具边框加厚（2026-11-12）

用户以实机圈图要求框条更厚；ATLAS按侧边/底边框条厚度约增加30%处理，红色嵌线
随框条同比加厚，保留上轮横向加宽35%的框架中线与红点。此次先交付工作区可审阅修改。

| 任务 | 唯一执行入口 | 独占文件 / 验收 |
| --- | --- | --- |
| SIGHT-THICK-ART | KINETIC临时sight_width_art续作 | 本轮artifacts候选源码/补丁；加厚框条，实际hip/ADS/recoil图与现有GL检查；正式源码由ATLAS应用补丁以在当前聊天展示 |
| SIGHT-THICK-VERIFY | VECTOR临时sight_width_verify续作 | 只写本轮artifacts验证工具/记录；Debug/Release、CPU与正式App验证，不改生产文件 |
| SIGHT-THICK-ACCEPT | ATLAS | src/character/procedural_rifle.cpp仅应用KINETIC补丁、docs与保护基线；审图、完整源码diff展示 |

CPU可并行，实际GL先KINETIC、再VECTOR串行。保留全部既有工作；不运行性能基准或API。
证据目录：artifacts/acceptance/crimson-frame-thickness/。只运行与几何改动相关的既有检查。

SIGHT-THICK完成，源码和GPU冻结/释放：只调整三处宽度参数；几何7/7、Debug/Release
构建与程序枪/ADS CPU检查、正式App17/17通过。ATLAS审阅加厚前后细节及正式ADS图。
本轮1050文件基线只有程序枪源码和两份交付文档变化；当前工作区完整diff另存
artifacts/acceptance/crimson-frame-thickness/changes.patch，并请求在当前聊天显示审阅面板。
本轮修改保留未提交，既有eadd26b提交完整；不把聊天的文件编辑摘要当作完整Git变更列表。

## 红色长弹体（2026-11-13）

用户要求玩家子弹改为红色，可见弹体长度约为当前2.5倍。当前基础网格长0.3m，
Bullet.length原字段尚未驱动渲染；本轮将默认可见长度设为0.75m并接入渲染Z缩放。
弹体/现有光晕使用红色；不调整速度195、射速、伤害、碰撞、生命周期或敌方弹体。
保留上一轮尚未提交的加厚瞄具和所有现有修改。

| 任务 | 唯一执行入口 | 独占文件 / 验收 |
| --- | --- | --- |
| RED-BULLET-INTEGRATE | VECTOR临时sight_width_verify续作 | 本轮artifacts候选补丁/验证工具；共享Weapon默认颜色/视觉长度和渲染接入方案；实际渲染与ADS行为检查 |
| RED-BULLET-ACCEPT | ATLAS | include/player/weapon.h、src/player/weapon.cpp仅应用VECTOR补丁；团队记录与保护基线、实际图和完整diff |

VECTOR提供候选后ATLAS在当前聊天应用，以展示全部源码变化。GPU独占交VECTOR；
只做相关CPU/GL及Debug/Release检查，不重复完整性能基准、不调用API、不提交推送。
证据目录：artifacts/acceptance/red-long-bullets/。

RED-BULLET完成，GPU释放：实际Weapon渲染16/16、Debug/Release构建与ADS CPU、
正式App17/17通过。弹体本地轴长×2.5、横向粗细不变；侧视投影75→187.5px。
颜色/现有光晕为红色，底层网格和光晕大小不变。ATLAS审阅实际侧视/斜视对照，
核对1050文件基线只有Weapon两文件和本团队记录变化，上一轮瞄具加厚完整保留。
本轮未提交、未推送；无需追加依赖或素材。

## 射速翻倍（2026-11-13）

用户要求射速提升一倍。本轮将玩家主武器射击间隔0.06s减为0.03s；保留红色长弹体、
加厚瞄具及已有未提交修改。FIRE-RATE由VECTOR临时sight_width_verify负责一行候选补丁
及本轮artifacts验证；ATLAS唯一写src/player/weapon.cpp应用补丁、维护本记录。
验证Debug/Release、既有ADS CPU、实际Weapon连续hip/ADS发射间隔和reset；不重复整套
视觉/性能检查，不提交推送。证据目录：artifacts/acceptance/double-fire-rate/。

FIRE-RATE完成：源码唯一变化为射击间隔0.06→0.03s。Debug/Release构建、ADS CPU、
实际Weapon腰射/开镜的0.6s半开区间对照均由10发变20发，边界/同帧拦截/reset检查通过。
保留既有逐帧冷却方式，不声明任意帧率下都精确翻倍；1050文件基线其余1048文件不变。

## 上传当前版本（2026-11-13）

用户要求上传一版，授权提交并推送origin/macos-port。收录上次本地eadd26b及当前
加厚瞄具、红色0.75m弹体和0.03s射击间隔。检测到瞄具之后还有本地参数调整：
框条0.0163、红嵌线0.00432、亮芯0.00145，原样保留，按本轮源码重新验证。
PUBLISH-VERIFY由VECTOR临时sight_width_verify唯一负责构建/CPU/实际App检查，只写
artifacts/acceptance/publish-2026-11-13/；ATLAS负责文档、文件筛选、提交和普通推送。
沿用源码交付规则，素材/构建产物和无关面试笔记不纳入本次提交。

PUBLISH验收完成：最终源码Debug/Release构建、程序枪/ADS CPU通过；正式App同版本
复验17/17通过。首次仅11项便正常退出的未完成记录保留，不计入通过，原因未确定。
609份源码/配置在验证期间哈希保持；ATLAS复核最终ADS图、源码与二进制hash。
正式精简记录：docs/acceptance/weapon-tuning-2026-11-13.json。提交仅包含本轮5份已有
源码/文档及新验收记录；上次本地eadd26b随同普通推送发布，不改写历史。

## 黑暗水晶与玩家自发光（2026-11-13）

范围按plan/forest-crystal-arena.md最新用户追加：移除地面全部黑色符文，压暗森林地图、
突出中央巨晶；枪/弹体/玩家近身光与上升粒子统一浅蓝水晶色。保留大范围微弱可见度，
不做仅一小圈可见的硬遮罩。用户当前未要求新手部模型、改玩法或上传版本。

为完整展示当前聊天的源码改动，本轮各专业角色仅在独占artifacts目录写候选源码/
apply_patch格式补丁，由ATLAS应用到下列生产文件；实现和接口设计仍各由专业角色负责。

| 任务 | 唯一执行入口 | 候选文件/责任，生产补丁由ATLAS应用 |
| --- | --- | --- |
| DARK-ENV | NEBULA临时dark_crystal_environment | res/shaders/forest_*.frag、include/src/environment；必要Crystal JSON/worldSettings与forest_arena/generate.py仅照明内容；去地面符文、暗场与巨晶表现 |
| DARK-CHAR | KINETIC临时sight_width_art续作 | include/src/character、character shaders与专属preview；蓝色流光/反光/halo/瞄点，保持当前枪形和瞄具参数 |
| DARK-CORE | VECTOR临时sight_width_verify续作 | App/必要MapRenderer公共接入、include/src/effects、include/player/weapon.h、相关tests/Makefile；蓝弹、玩家持续光、近身上升粒子、暂停/重开/切图与最终集成 |
| DARK-ACCEPT | ATLAS | plan/docs、保护基线、应用专业补丁、GPU协调与正式画面验收 |

共享色彩目标：流光/玩家输出为浅水晶蓝约(0.28,0.72,1)，亮芯约(0.75,0.94,1)，
各角色按自己的发光强度换算底色，避免RGB夹断成整片纯白。敌方预警维持暖色。
NEBULA拥有地图和生成器，VECTOR不编辑它们；KINETIC不编辑Weapon/App。公共接口先交接。
三角色CPU可并行；只有VECTOR可写公共build/bin，其他preview构建限定各自artifacts。
GPU先NEBULA、再KINETIC、最终VECTOR；NEBULA/KINETIC释放后才进入下一阶段。
证据目录：artifacts/acceptance/dark-crystal-pass/；保留两份现有未跟踪研究/面试文档，
不追加素材/API消费，不运行性能基准，不提交推送。实际GL和人类试玩区分记录。

DARK已完成，三专业候选全部由ATLAS在当前聊天应用，写入冻结、GPU释放。
地面图腾代码移除，森林压暗并突出中央晶蓝；枪/弹/近身光与上升粒子统一晶蓝。
实际玩家光0.9/18m，粒子18/s、3s、上限96；最终一次微调提高粒子可见度并重新验证。
场景GL/角色GL与几何6/6、Debug/Release、相关CPU、最终App aura12/12通过；ADS17/17
和蓝弹三姿态结果在仅粒子可见度参数微调后复用，明确保存版本边界。ATLAS已审最终
idle/俯视/侧向、场景和武器对照及蓝弹；654份最终源码/配置与二进制哈希独立核对。
1052份初始文件中24份分派文件有改动，其余1028份保持；新增粒子测试及文字验收记录。
两份原有未跟踪文档、Hollow/归档地图与武器运动/伤害源码保持。本轮未提交或推送。
精简验收：docs/acceptance/dark-crystal-2026-11-13.json；完整diff在本轮artifacts/changes.patch。

## 明亮天眼、开放地表与新纹理（2026-11-13）

当前 Crystal 地图保留巨晶、暗色调、玩家晶蓝光和敌潮，提亮天眼和星星，
移除树木及附属植被、相关碰撞和生成入口，并接入可平铺的矿物地面纹理。
保留已修改工作区和其他地图。本轮候选补丁由 ATLAS 统一应用，验收集中运行 GL。

| 任务 | 负责人 | 独占候选范围 |
| --- | --- | --- |
| OPEN-SKY | NEBULA | forest_sky.frag；眼睛和星点的曝光与可见度 |
| OPEN-LAYOUT | VECTOR | starry_crystal_arena.json、forest_arena/generate.py、layout-state.json、forest_arena_probe.cpp；移除植被与其物理占用、保留竞技场 |
| OPEN-GROUND | 地表 agent | 新地面图像及素材生成记录、forest_map.frag；地面采样方案 |
| OPEN-INTEGRATE | ATLAS | map_renderer.h/.cpp 纹理接入、environment_scene.cpp 和 environment_renderer.cpp 远景树影移除、候选集成、文档和实际画面验收 |

OPEN 已集成，写入关闭，GPU 已释放。移除 264 棵树、12 个根丛区块和 384 个
远景树影；矿岩边沿与碰撞共用同一几何。内置 imagegen 板岩已由实际 App 加载，
缺图时保留程序化地面。Debug/Release、完整 CPU 套件、竞技场 94/94、实际 App
17/17、纹理缺失和资源释放各 14/14 通过。证据见
`artifacts/acceptance/open-crystal/atlas-verification.json`，纹理提示词见
`res/models/forest_arena/slate-texture.md`。未提交或推送。

## 动态水晶天空（2026-11-17）

| 任务 | 负责人 | 独占范围 |
| --- | --- | --- |
| SKY-MOTION | NEBULA 临时 open_sky 续作 | forest_sky.frag 候选：眼睛略缩小、有机颤动、星点独立闪烁 |
| SKY-MOTION-QA | ATLAS | 生产补丁、实际 GL 与暂停/重开检查、文档 |

SKY-MOTION 已集成，写入关闭，GPU 已释放。GLSL 链接及实际 App 8/8 通过；
72 帧像素后处理输出确认瞳孔约缩小 15%、轻颤和星点不同步闪烁。
动态预览在 `artifacts/acceptance/sky-motion/crystal-sky.gif`。
SKY-TUNE 由 NEBULA 提供单文件候选，ATLAS 集成并复验：眼睛动效速率和幅度
均降至初版 20%，初始画面和测试星区逐像素一致，实际 App 8/8 通过。

## 水晶蓝黑洞（2026-11-18）

NEBULA（BLACK-HOLE-SKY）负责 `res/shaders/forest_sky.frag` 候选，按第二版概念图制作黑洞与流动吸积盘。
VECTOR（BLACK-HOLE-QA）负责本地游戏实拍与动效检查。ATLAS 集成生产补丁、复核画面并维护设计记录。

已完成。最终 shader 通过 GLSL 链接和 16 项实际 App 检查。6 秒固定镜头实拍确认
盘带纹理沿右上方向移动，暗影中心保持稳定。实际画面与像素记录见
`artifacts/acceptance/black-hole-sky/qa/r3/`。

## 倾斜吸积盘与独立天体绘制（2026-11-19）

| 任务 | 唯一写入范围 | 负责人及验收 |
| --- | --- | --- |
| BH-DISK | `res/shaders/forest_black_hole.frag` | NEBULA，圆形暗影、倾斜亮盘、绕行气流与连续光环 |
| BH-SKY | `include/environment/environment_renderer.h`、`src/environment/environment_renderer.cpp`、`res/shaders/forest_black_hole.vert`、`res/shaders/forest_sky.frag` | VECTOR，独立天体绘制、森林限定、锚点与渲染状态 |
| BH-CAPTURE | `artifacts/acceptance/tilted-black-hole/` | 验证 agent，同机位实拍、运动、遮挡及生命周期检查 |

ATLAS 协调接口并验收画面。保留其余工作区修改。

已完成。ATLAS 审阅两轮同机位实拍，第二轮收紧远侧光环并提亮近侧气流。
Debug/Release 构建和测试通过，最终版通过 17 项实际 OpenGL 检查，包含暂停、
重开、反向视角和切换 Hollow。画面对照与量化结果见 `artifacts/acceptance/tilted-black-hole/r2/`。

### 黑洞像素颗粒与盘面动态（2026-11-19）

RETRO-DISK 由 NEBULA（disk_art）独占 `res/shaders/forest_black_hole.frag`，加强气流的位移、颗粒和局部错位。
RETRO-QA 由 sky_capture 独占 `artifacts/acceptance/retro-black-hole/`，负责同机位动图、盘面动态与暂停复位检查。ATLAS 复核实拍。

已完成。480×300 同机位 6 秒实拍中，近侧亮带的典型摆幅从 0.33 增至 6.15 像素，
右翼颗粒的局部明暗对比约为原来的 5.2 倍。最终材质通过 17 项实际 OpenGL 检查。

RETRO-MID 由 disk_art 调整 `res/shaders/forest_black_hole.frag` 的颗粒强度，保留盘面动画；
sky_capture 在 `artifacts/acceptance/retro-black-hole/midpoint/` 同机位复测并输出三版对照，ATLAS 验收。

RETRO-MID 已完成。同机位六秒实拍中，右翼颗粒相对局部亮度的对比从平滑档 0.089、
强档 0.526 调到 0.307。17 项实机检查通过。

RETRO-SOFT 由 disk_art 独占黑洞片元材质中的颗粒系数，sky_capture 在
`artifacts/acceptance/retro-black-hole/soft/` 验证平滑档与上轮中档之间的新取值，ATLAS 对照实拍。
已完成。最终版实测相对颗粒对比为 0.197，处于两档之间的 49.8%；盘带摆幅约 6.6 个原生像素。

## 恢复并缩小天眼（2026-11-19）

| 任务 | 唯一写入范围 | 验收 |
| --- | --- | --- |
| EYE-RESTORE | NEBULA / disk_art，`res/shaders/forest_sky.frag` | 取回发布版眼球、沿原锚点整体缩小 15% |
| EYE-RENDER | VECTOR / sky_render，环境 renderer 头文件与实现、两个 `forest_black_hole` shader | 恢复单次星空绘制，归档黑洞试作材质 |
| EYE-QA | sky_capture，`artifacts/acceptance/restored-eye/` 和构建产物 | 实机同机位比较原眼和当前眼、大小、动效与时钟 |

ATLAS 同步当前地图说明并复核实机画面。

已完成。渲染器恢复到 `82c815f` 的单次天空绘制，天眼着色器相对此提交仅增加整体缩放。
Debug、Release 构建及现有测试通过，同镜头实拍与 15 项 OpenGL 检查见 `artifacts/acceptance/restored-eye/final/`。

## 蓝色天眼与鱼眼视野（2026-11-19）

| 任务 | 负责人和独占写入范围 | 验收 |
| --- | --- | --- |
| IRIS-BLUE | NEBULA / disk_art，`res/shaders/forest_sky.frag` | 调整虹膜与眼周云雾的色彩 |
| PLAYER-LENS | VECTOR / sky_render，`res/shaders/pixel.frag`、PixelRenderer、App、第一人称武器投影与必要的投影模块 | 第一人称桶形畸变、准星与敌人标记对齐 |
| LENS-QA | sky_capture，`artifacts/acceptance/blue-eye-fisheye/` 和构建产物 | 同机位对照、腰射与开镜、画面边缘与 HUD |
| LENS-AIM | 镜头投影测试 agent，`tests/aim_state_test.cpp` | 回归检查准星中心、偏轴投影和开镜过渡 |

ATLAS 复核原生画面与投影映射。

已完成。480×300 实机标定中，同一条直线在腰射、开镜过渡中点和完全开镜时分别弯曲
6、4、1 像素，三个状态下敌人血条锚点与画面位置相差均小于 0.5 像素。
Debug/Release 构建、测试及开镜回归通过。对照画面与测量见 `artifacts/acceptance/blue-eye-fisheye/final/`。

HIP-LENS 由 VECTOR / sky_render 调整 `include/engine/view_lens.h` 的腰射强度；
sky_capture 在 `artifacts/acceptance/stronger-hip-fisheye/` 实拍腰射并逐像素比对开镜画面，ATLAS 验收。
已完成。480×300 标定线的腰射弯曲量从 6 增至 8.5 像素；完全开镜时的 960×600 实机画面逐像素一致。

## 游戏内鱼眼调节（2026-11-19）

| 任务 | 独占写入范围 | 验收 |
| --- | --- | --- |
| LENS-CONTROL | VECTOR / sky_render，App、镜头参数、偏好读写及关联测试 | 热键调整、独立总开关、设置保存与恢复 |
| LENS-HUD | PRISM，`include/hud`、`src/hud` 及必要的 HUD 测试 | 暂停页提示、调整时的即时数值 |
| LENS-VERIFY | sky_capture，`artifacts/acceptance/fisheye-controls/` 及构建产物 | 开关生效、数值微调、跨启动保持、开镜投影与界面实测 |

ATLAS 维护玩家操作说明及本地设置文件的忽略规则。
已完成。Debug、Release 构建与测试通过。43 项实机检查覆盖热键、总开关、强度上下限和重启恢复，
关闭时腰射、开镜均与直线投影逐像素一致。暂停页与测量见 `artifacts/acceptance/fisheye-controls/r1/`。

## 双倍场景分辨率与鱼眼默认值（2026-11-19）

HIRES-LENS 由 VECTOR / sky_render 独占渲染配置、PixelRenderer 默认值、鱼眼设置及相应测试；
PRISM / lens_hud_controls 对齐 HUD 的缺省读数。sky_capture 独占 `artifacts/acceptance/hires-fisheye/`
及构建产物，实测缓冲区尺寸与调节按键。ATLAS 同步本地选值和当前运行说明。
已完成。实机读回的场景和枪械颜色、深度缓冲区以及后处理采样尺寸均为 960×600。
默认腰射 0.33、完全开镜 0.08、V 与方括号热键及已存自定义值均通过检查。

## 通用本地配置（2026-11-19）

GENERAL-CONFIG 由 VECTOR / sky_render 负责引擎配置读写与 App 接入。
lens_aim_regression 负责 `tests/aim_state_test.cpp` 的迁移与字段保留回归。
sky_capture 负责构建、测试和隔离目录中的启动验证。ATLAS 负责本地文件迁移、忽略规则及中英文使用说明。
已完成。Debug 和 Release 构建与测试通过。6 次实机启动通过 39 项检查，覆盖旧配置迁移、
新旧文件并存时的优先级，以及使用热键后保留手动添加的字段。记录见 `artifacts/acceptance/general-config/final/results.json`。
