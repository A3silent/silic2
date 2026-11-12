# 静态模型离线导入 / Static asset import

`import_static_model.py` 把本地 GLB 或已三角化 OBJ 转成游戏与编辑器共用的静态
OBJ 包。它只在离线工具阶段运行，使用 Python 3.9+ 标准库，无网络请求、不消费
Tripo 额度、不新增运行时 glTF 依赖。真实 Tripo 模型、生成提示词审批和最终摆放
另行验收；当前测试素材是临时合成几何，不能作为生成成果。

## 使用

通过 Tripo Studio 或官方 CLI 下载未压缩、静态、带 base-color 的 GLB 后，保留源文件，输出到
**尚不存在的新目录**：

```sh
python3 tools/assets/import_static_model.py /absolute/downloads/rifle.glb \
  res/models/tripo/orbital_rifle --size 0.85 --pivot center \
  --rotate 0 0 0 --producer tripo-studio --task-id ACTUAL_TASK_ID \
  --prompt tools/tripo/prompts/01_orbital_rifle/prompt.en.txt
```

示例中的源路径、任务 ID、提示词路径须换成实际值；`--producer` 是用户提供的
来源标签，不构成工具对来源或授权的认证。工具只记录源文件名、SHA256、字节数、
提示词哈希和任务 ID，不记录签名下载 URL、Cookie 或凭据。

- `--rotate X Y Z`：角度，按 X→Y→Z 顺序旋转，再计算尺寸和支点。
- `--size N`：最终包围盒最长边为 N 游戏单位。与 `--scale N` 二选一。
- `--pivot floor`（默认）：XZ 居中，最低点 Y=0，适合场景道具。
- `--pivot center`：三轴包围盒中心归零，适合步枪。
- `--pivot origin`：保留源原点；缩放/旋转仍生效。
- `--scene N`：选择 glTF scene；缺省采用默认 scene，多个 scene 无默认时拒绝猜测。

步枪必须在实际预览后确认 **+Y 向上、-Z 为枪口方向**。工具不会从形状猜方向。
先导入一个审阅目录，检查朝向后用另一目录重导；不要反复对已归一化 OBJ 使用
不同隐式单位。场景位置和二次缩放仍由地图实例 TRS 设置。

输出：

```text
model.import.json          # 格式、转换参数、整体边界、统计、来源和警告
parts/part-000.obj          # 每个 primitive/material 独立，变换已烘焙
parts/part-001.obj
textures/basecolor-000.png  # 原始 PNG/JPEG 字节直接提取，保留 hash
```

成功后以目录重命名发布结果；解析/转换失败清理临时目录，不覆盖已有输出。
生成模型和贴图受仓库现有 `.gitignore` 排除；不要 force-add 资产。

## 共享合同 v1

```json
{
  "format": "silic2-static-model",
  "schemaVersion": 1,
  "bounds": {"min": [-0.1, 0, -0.4], "max": [0.1, 0.2, 0.4]},
  "parts": [{
    "obj": "parts/part-000.obj",
    "baseColorTexture": "textures/basecolor-000.png",
    "baseColorFactor": [1, 1, 1, 1]
  }]
}
```

示例仅说明字段，不是可用素材。完整导入结果还包括每部分边界/哈希/三角数、
源 node/mesh、转换来源信息和 warnings。包内资源路径相对 manifest，必须在
包目录内；C++ 消费者拒绝绝对路径、越界 `..` 和指向目录外的符号链接。不同包
之间不要手动链接资源。`loadStaticModelBundle()` 在 CPU 上加载各部分并核对
实际几何与声明边界，一次失败不会返回部分结果。哈希用于离线来源与复核记录，
**运行时不会执行 SHA256 完整性认证**。

`StaticMesh` 的 positions/UV/normals 按角点对齐。GLB 节点的层级矩阵或 TRS、
实例化、负缩放和非均匀缩放会烘焙到 OBJ；法线使用逆转置，负行列式反转面序。
缺少法线时生成面法线。glTF UV 的 V 转为 `1-V`，配合现有引擎加载图片时的
垂直翻转；渲染器不应再次翻 UV。原 OBJ 导入不翻 V。

武器通过 `loadStaticModelBundle()` 消费多材质包，独立 fitting 配置控制持枪
位置与视觉枪口。实际射击方向、射速、伤害和碰撞不在本工具职责内。

场景每个 part 对应一个 `geometry.models` 实例，使用既有 `color` 保存
`baseColorFactor.rgb`，新增可选 `texture` 保存贴图相对地图的路径：

```json
{
  "id": 4001,
  "name": "imported decorative part",
  "asset": "../models/tripo/example/parts/part-000.obj",
  "texture": "../models/tripo/example/textures/basecolor-000.png",
  "collisionAsset": "",
  "material": "imported_static",
  "color": [1, 1, 1],
  "position": [0, 0, 0],
  "rotation": [0, 0, 0],
  "scale": [1, 1, 1]
}
```

这是接口示例，**不能在真实资产不存在时写入正式地图**。所有部分使用相同
实例变换，保持组合形体。Map 编译保持 UV，把 texture 解析到 `Brush.texture`；
Save As 同时重定位 asset/collisionAsset/texture 路径。贴图缺失时场景渲染器
沿用既有颜色回退；武器包资源缺失则整体返回旧武器。视觉高模不会自动成为
碰撞网格，需另做低复杂度代理并验证通行、射线和导航。

## 支持范围与主动拒绝

依据 [Khronos glTF 2.0 specification](https://registry.khronos.org/glTF/specs/2.0/glTF-2.0.html)。

支持：GLB 2、一个内嵌 BIN、TRIANGLES、索引/无索引、交错 buffer stride、
float positions/normals、float 或 normalized unsigned UV、UV0/UV1、节点层级、
重复 mesh 实例、内嵌 PNG/JPEG、base-color factor、KHR_texture_transform、
opaque double-sided（离线反向面复制，统计包括双面）。

拒绝：Draco/Meshopt/BasisU、其他必须支持的未知扩展、skin/animation/morph、
sparse accessor、条带/扇形/线、顶点色（请先烘焙到贴图）、透明/裁切材质、
非重复纹理 wrapping、外部 GLB buffers/images、非法引用、非有限值、零面积面、
退化变换。源 OBJ 必须已三角化，仅保留几何/UV/法线，MTL 材质不导入且明确警告；
有材质需求应使用 GLB。OBJ 不支持颜色扩展及齐次坐标。

保留 base-color 的图像和因子，使用 Silic2 自身光照，不是完整 glTF PBR 重现。
normal/metallic/roughness/AO/emission map 不转交，并在 manifest 留下警告。
nearest/mipmap 采样由现有引擎决定。图像提取核对 MIME 和 PNG/JPEG 头，CPU 测试
不等于实际解码/视觉验收；实际导入后仍需运行时解码与贴图方向检查。

上限：源 GLB 512 MiB、每图 64 MiB、PNG 每边 8192、每包 128 part / 100000
三角形、每个输出 OBJ 32 MiB / 200000 展开角点。部件因双面或展开超过预算时
直接拒绝，应在 DCC 简化/拆分；工具不进行可能破坏轮廓的自动减面。武器消费者
还有更紧的 16 part / 20000 三角形限制。场景平滑法线当前仍由既有平面渲染决定，
导入器保留的 authored normals 已可供武器消费者使用。

## 验证

```sh
make -f Makefile.map asset-import-test
make -f Makefile.map test
```

18 个导入测试现场生成 GLB、转换后调用**真实 C++ loader**读取 OBJ/manifest，
覆盖矩阵/层级/镜像/法线、交错与 normalized UV、纹理原字节、多实例、双面、
OBJ roundtrip、压缩/skin/sparse/alpha 拒绝、范围错误、退化面、输出不覆盖、
路径与符号链接越界、错误边界等。普通 CPU 测试额外覆盖 model.texture 的加载、
编译、Save As 和重开。测试不打开 GL，不调用 Tripo；实际模型质量、贴图解码、
武器 fitting、碰撞代理、场景摆放与性能仍等待真实生成资源后验收。
