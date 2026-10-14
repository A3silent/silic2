# Silic2 地图编辑器

本次升级保留 `tools/map_editor/build/map_editor` 入口，并与仓库内游戏模块共享同一份 Map / JSON 实现。旧地图、纹理路径、实体属性与未知扩展字段可保留、编辑、另存。

## 构建与打开

完整克隆游戏仓库后，从游戏仓库根目录执行以下命令。

```sh
cmake -S tools/map_editor -B tools/map_editor/build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build tools/map_editor/build -j 4
./tools/map_editor/build/map_editor
```

`SILIC2_ROOT` 默认是相对此目录的仓库根目录 `../..`，可通过 `-DSILIC2_ROOT=/实际游戏目录` 指定。需要 CMake、C++17 编译器、OpenGL、GLFW；构建所需的最小 ImGui 子集已包含，来源见 [DEPENDENCIES.md](DEPENDENCIES.md)，GLM 和地图代码来自游戏。直接从任意工作目录启动绝对路径 `tools/map_editor/build/map_editor` 均可；命令行可带地图路径。

## 常用操作

- **Open**：文件浏览器支持目录路径、文件名、Game maps / Parent / Home。打开失败保留当前工作。
- **Crystal template**：从游戏标准星空竞技场创建未保存文档，另存到自己的路径，不覆盖模板。
- 视口单击与左侧列表共用选中状态；青色轮廓突出选中物体。双击列表或 **F** 聚焦。
- 按住**右键 + WASD** 浏览，**Q/E** 下/上，Shift 加速；松开右键回到编辑。界面始终可见，滚轮沿视线推进。
- Inspector 拖拽或输入 **Position / Size / Rotation**，模型使用局部 **Scale**。拖动时轻量预览，松开后提交一次共享验证与一条撤销记录，状态栏显示提交耗时。启用 Snap 后结束编辑时吸附；Snap to grid 可立即吸附。方向键沿世界 X/Z 移动，Page Up/Down 沿 Y 移动，Shift 为十倍步长。
- **Cmd/Ctrl+D** 复制，Delete/Backspace 删除；Add 菜单添加真立方体、水平平面、五类旧实体及三类灯。几何可按 XYZ 欧拉角旋转；平面局部 Y 尺寸固定为零。新增坡道沿局部 +Z 低端向 -Z 高端上升。旧单平面网格不会凭空变成有厚度的盒子。
- **Cmd/Ctrl+Z** 撤销，**Cmd/Ctrl+Shift+Z / Ctrl+Y** 重做。一次拖动合并为一个命令，最多保留128条；保存点参与脏状态判断。
- **Cmd/Ctrl+S** 保存，**Cmd/Ctrl+Shift+S** 另存。星号表示未保存。新建、打开、模板和退出会提供保存/丢弃/取消；另存覆盖现有文件需确认。
- Map settings 可改重力、环境色、环境预设、异变时间/时长、粒子数量。重力是保存到地图的兼容字段，不会覆盖当前游戏玩家重力；玩家仍使用 PlayerConfig.gravity。竞技场必需的玩家/裂隙/炮台锚点不能通过单次非法编辑删除；错误显示具体字段与原因。
- 文本输入不会触发相机和普通编辑快捷键；失焦会终止浏览。保存使用同目录临时文件和成功后的替换，失败不破坏旧文件。

预览使用简单颜色展示几何、实体、灯与选择框，不模拟游戏最终纹理、光照、晶体特效或战斗。

## 可复现验证

```sh
ctest --test-dir tools/map_editor/build --output-on-failure
./tools/map_editor/build/map_editor --self-test res/maps/textured_room.json /tmp/editor-legacy-check
./tools/map_editor/build/map_editor --self-test res/maps/starry_crystal_arena.json /tmp/editor-arena-check
./tools/map_editor/build/map_editor --smoke res/maps/textured_room.json /tmp/editor-gl-check
```

self-test 覆盖旧网格移动/缩放、复制ID、纹理/扩展字段、撤销保存点、失败载入/保存、v2对象和游戏共享加载器重读。smoke 打开真实 ImGui/GL 窗口，注入合成键鼠事件走共用交互处理，生成 edited.json 与 PPM 截图，并验证 Retina/窗口缩放。它不是人工易用性测试。所有危险操作只针对输出目录副本；GL检查请与游戏顺序运行。更详细接口和验证说明见 README.md。

## 混合建模流程

- **File → Import static OBJ / Cmd+I** 浏览导入模型。使用共享 OBJ 读取器：支持 v/vt/vn、三角形或凸四边形面、负索引；建议导出 Y 向上、米制、三角化、外向绕序模型。每实例单材质/颜色，不支持 MTL 贴图、动画、任意多边形或顶点编辑。
- 勾选 **Use visual mesh for collision** 使用真实视觉网格碰撞；取消后可指定独立碰撞 OBJ，留空则纯装饰。实例支持位置、旋转、非均匀缩放、复制与吸附。**Edit resource paths** 可事务式替换资源；缺文件/坏模型/非法变换不破坏当前文档。
- **View settings → Layer** 切换视觉、碰撞、导航。碰撞来自共享编译后的三角代理；导航节点/连接保留独立高度，显示为可透视检查的覆盖线。
- **Add object → Navigation node** 添加脚底位置节点，Inspector 输入目标 ID 后连接，可选双向；导航面板可删边，删节点时连边一并进入撤销记录。玩家、裂隙和炮台出生标记仍可正常移动。几何变换不会自动移动导航节点。
- 共享诊断显示悬空节点、阻塞边、断连路线与出生问题，可继续编辑修复；游戏会拒绝仍有空间诊断的地图。创建坡道/模型或旋转基础几何会显式启用 v3 三角空间模式；普通旧图操作保留旧模式。
- **Save As** 由共享保存器将模型/代理路径改为相对目标目录。可另存仓库外后重开继续编辑，但资源不复制、不嵌入；原资源位置必须继续可访问。区域、路线和旧探索扩展字段继续保留。

新增 `--hybrid-self-test 输出目录` 运行无 GL 的模型/坡道/导航/外部路径回归；`--measure-edits 地图 输出目录` 测完整图 CPU 提交时间。`--hybrid-smoke 地图 输出目录 OBJ路径` 打开真实窗口，以合成输入走导入、旋转、缩放、复制、撤销重做、新坡道、另存重开和完整地图拖动，输出截图与包含提交/截图峰值的原始帧 CSV。它们均不是人类操作体验结论；GPU 按团队时段串行运行。

`--layout-smoke MAP OUTPUT` checks a selected model’s full-width TRS fields and real rotation drag/undo at 1440×900 and 800×600. It requires the same serial GPU scheduling as other smoke modes.
