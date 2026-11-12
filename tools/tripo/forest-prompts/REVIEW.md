# 森林巨晶竞技场：Tripo 建模提示词审阅

版本 `forest-v1`，2026-10-31。状态：**用户已批准四套提示词，ATLAS已提交四件生成**。
本包是四件模型的设计与提交文本；不会自行调用 Tripo、购买额度或生成素材。

以已批准概念图 `artifacts/acceptance/forest-arena/approved-concept.png` 为构图参考：
直径约200米的圆形石灰岩战斗场，中央单根140米巨晶，外围三种松树组成森林。
巨眼、云雾、原有星空、地面图腾及砂石地表由场景渲染实现，不塞进独立模型中。
树木是可重复摆放的**单株**，不是连带地形的一整片森林；晶柱是一件完整主地标。

## 如何阅读与实际提交

- 每件 `brief.zh.txt` 是完整中文设计要求，`brief.en.txt` 是相同意图的详细英文美术说明。
- 实际API只提交 `prompt.en.txt` 和 `negative.en.txt`，下文逐字列出，分别不超过1024/255字符。
  本机 Tripo CLI 的 `dist/knowledge/params.js` 已离线核实这两个限制，不能把长说明直接塞入API。
- 拟用已验证导入流程的 `tripo-p1`（wire版本 `P1-20260311`），`texture=true`、`pbr=false`、
  每件一候选；face_limit分别2000/2500/3000/4000。未授权额外在线重网格、贴图或重复生成。
- 长篇要求用于设计与验收，**不是更多微细节或更多多边形**。本游戏内部320×200渲染，
  优先轮廓、宽大结构和少数清晰色块。不要追求写实针叶、密集裂纹或高频噪声。
- 尺寸、朝向、面数、UV、透明标记均是待检查的交付目标，不能靠提示词保证。
  下载后检查真实网格，统一为米制、+Y向上、根部/基底中心原点；保留原始来源文件。
- 首选一个不透明材质与base-color贴图，树最多两个材质；忽略未使用PBR通道。
  不烘焙日光、方向阴影、光晕或照在地面的光。运行时仅天空和巨晶提供环境照明。
- 实际碰撞另做匹配树干/粗根/晶柱实体的简化代理；不要给整个树冠套实心盒子。
  源模型若标记double-sided，现有导入器可能复制反面，运行时三角形会高于source face_limit。

## 为什么眼睛、云雾和图腾不用 Tripo

北方巨眼是天空中被云层包围的圆形凝视意象，适合在既有星空上绘制方向固定的天空效果，
无需实体眼球堵住世界；云层透明层次与缓慢涡旋也更适合shader。图腾是直接画/刻在平地上，
采用地表材质或贴花能维持平整碰撞与连续的跑动区域。巨晶符文的发光和异变由运行时控制，
模型只提供宽阔晶面和极浅的大笔画导向，不烘焙不可调节的亮斑。

## 导入目标与分工

| 模型 | 目标尺寸 X/Y/Z（米） | Source face_limit | 用途 |
| --- | --- | --- | --- |
| 疏冠高松 | 7 / 22 / 7 | 2000 | 高而透气的森林竖向轮廓 |
| 层叠密冠松 | 8 / 18 / 8 | 2500 | 遮挡边界、形成厚密林带 |
| 扭干阔冠老松 | 12 / 20 / 11 | 3000 | 林缘不规则转折与辨识度 |
| 冲天巨晶 | 16 / 140 / 16 | 4000 | 统一中心地标与局部照明来源 |

树冠宽度为近似最大包络，巨晶基底12–18米可接受；导入以保持轮廓比例为先，不强行逐轴压扁。
预计运行时路径：`res/models/tripo/forest/<pine_sparse|pine_dense|pine_crooked|crystal_spire>/model.import.json`。
在线调用由ATLAS独占，离线导入由FOREST-ASSETS执行，摆放/渲染由NEBULA，公共接入与功能验证由VECTOR。
尺寸、纹理和法线需在实际资源到达后验收；本轮不做性能测试。


## 疏冠高松 / Tall sparse-crowned pine

### 中文完整说明

制作一株可独立重复摆放的古老高松，作为200米圆形竞技场外围森林的第一种树形。它要直立、修长、树冠通透，远看像一条被少数扁平针叶云团打断的深色竖线。目标高度22米，最大树冠直径约7米；主干地面直径约0.8–1.1米，向上自然收细，稍有弯曲但不倾倒。下部约7米以裸露树干为主，留出可看见林内纵深的空隙；中上部约五至七层错落横枝，不是均匀对称的圣诞树。

每层枝条使用两至四团方向不同的针叶簇，冠部偏向一侧，顶部保留明显尖梢。主枝应有可读的粗细和小幅下垂，再在末端轻微上挑。以大尺度负空间分开各层树冠，少量粗短枯枝形成辨识度。针叶不是一根根针，也不是透明平面：用有厚度的低面数块簇表达，轮廓可有三五处锯齿，但不要几十个细毛边。底部四至五条低矮粗根牢固连接主干，贴近共同的地面平面；不带地形、泥土圆盘或展示底座。

树皮为灰棕、炭黑的宽条纵向裂带，少量灰色斑块即可；针叶为低饱和墨绿、蓝黑色，以大块明暗色变化表达体积。不要烘焙阳光、方向性阴影、蓝紫环境光或高亮边缘。模型本身保持中性材质，游戏中的天空与水晶负责照明。相邻树木随机旋转时应仍可信，不做只有正面好看的舞台布景。

这是320×200内部渲染的低多边形游戏资产，目标source约2000三角形，优先树干与树冠轮廓。希望有完整UV和一个不透明base-color材质，最多两个材质；所有针叶簇从四面可见、有体积，不依赖alpha。静态无骨骼，不需要风动画、Lod组、粒子或细小果实。根部中心位于原点，+Y向上，完整模型不裁切。朝向、实际高度与面数由离线导入再核实。

验收重点：在远处仍明显比另两种松树更细、更透气；高树冠和长裸干能形成森林层次，而不是像细杆插着一整块塑料锥体。不得生成松树林、地面、天空、水晶、道路、人物、文字或任何商标。

### English full brief

Create one original mature tall pine as a reusable static environmental asset for a dark forest surrounding a 200-meter circular combat arena. Its defining character is a slender upright trunk and an airy, discontinuous crown. Target 22 meters tall with a maximum crown spread near 7 meters. The trunk is roughly 0.8-1.1 meters thick at ground level, tapers naturally and has a restrained organic bend. Keep the lower seven meters mostly bare. Above that, arrange five to seven uneven branch tiers, with deliberate gaps showing through the silhouette rather than a continuous cone.

Each tier holds two to four substantial needle masses extending in different directions. Branches sag slightly before lifting at their ends. Offset the upper crown and preserve one unmistakable pointed leader. A few blunt dead branch stubs add age without creating a web of thin twigs. Build foliage as opaque, thick, faceted needle clusters: each mass has a handful of broad serrations, never individual needles, thin hairs or transparent crossed cards. Four or five low chunky roots join the trunk and terminate near a common ground plane. Do not include a soil disc, terrain fragment or presentation plinth.

Use desaturated charcoal-brown bark with broad vertical furrows and a few gray patches. Foliage is muted black-green with blue-gray variation in broad color areas. Texture should communicate material without photographic noise. Do not bake sunlight, directional cast shadows, colored crystal illumination, rim light or a bright overhead gradient. Runtime sky ambience and crystal light must determine the lighting. The model must read from all sides when independently rotated and repeated among the other two pine species.

This is an economical low-poly asset for gameplay rendered internally at 320x200. Spend approximately 2000 source triangles on the primary silhouette, trunk and major branch masses, not microdetail. Supply sensible UVs and preferably one opaque base-color material, at most two. Foliage must possess visible volume from below and behind. Static mesh only, no rig, animation, particles or LOD assembly. Root contact center is the origin, positive Y is up, and the entire uncut tree is included. Final scale, orientation, actual triangle count and collision suitability are inspected offline, rather than assumed from the generation request.

Success means a tall open pine visibly distinct from the dense conical species and crooked broad-crowned species at small screen sizes. Avoid toy-like smooth cones, perfect radial symmetry, unnaturally thin poles, isolated floating leaves and dense noisy bark. No surrounding forest, landscape, crystal, sky, vehicles, humans, lettering or logos.

### 实际 API 正向提示词（878/1024字符）

```text
One original isolated tall sparse-crowned pine for a dark forest game rendered at 320x200. Stylized low-poly, 22m tall, crown 7m wide. Slender tapering charcoal-brown trunk, lower 7m mostly bare, slight organic bend. Five to seven uneven horizontal branch tiers with wide visible gaps; asymmetrical crown and pointed leader. Thick branches sag gently then rise at ends. Opaque chunky faceted needle clusters, muted black-green and blue-gray broad color patches, no individual needles or alpha cards. A few stout dead branch stubs; four low thick roots join the trunk on one ground plane. Broad bark grooves, no tiny noise. Readable from every direction, economical roughly 2000 triangles. Static UV-mapped mesh, one opaque base-color material, neutral unlit colors without baked shadows or glow. Complete single tree, ground-root center pivot, +Y up. No scenery or display base.
```

### 实际 API 负向提示词（211/255字符）

```text
forest scene, terrain, pedestal, transparent foliage, alpha cards, individual needles, tiny twigs, high-poly, photoreal noise, plastic cone, symmetry, snow, flowers, ornaments, baked light, glow, rig, text, logo
```


## 层叠密冠松 / Dense layered pine

### 中文完整说明

制作第二种独立可复用松树：树冠厚密、轮廓接近不规则锥形的成熟松树。目标高度18米，最大宽度8米。它比疏冠高松矮而厚重，作为林带内连续阴影与树林边界的主要视觉体量，但仍要像树，而不是绿色实心围墙。主干直径约1米，根部与下方2米左右的树干清楚露出；树冠从约2.5米起逐层向上收窄。

树冠分成七至九个不等高的宽厚枝层，下面两层最长，中部略有左右错位，最高层收成偏心尖梢。每层不是圆盘或完美三角形，而由几块交叠、有厚度的针叶扇团组成，边缘宽大不规则。下垂的枝端要短粗，从下方看也有体积。层间保留少数明显的暗缝，让玩家能读出树枝层次；不要把叶簇全部融成光滑锥体，也不要每一枝都细密分叉。树干和粗枝连接自然，低矮根部可以略向外扩展，但不带土壤底座或散落物场景。

树叶是阴暗低饱和的深绿、灰绿、炭蓝色大块；树皮为冷灰棕，宽裂片为主。不要金色阳光、暖色树梢、白雪、荧光或圣诞装饰。其色值比中央水晶和敌人低，物体的照明由运行时处理，贴图不应画死一侧亮、一侧黑的方向阴影。用少量宽面色彩差异区分上下枝层，避免颗粒状照片细节。

面向320×200游戏画面，目标source约2500三角形。针叶必须以不透明、有厚度的低面数块簇表达，禁止alpha平面、单根松针或毛发。提供完整UV，优先一张base-color贴图和一个不透明材质，最多两个材质。模型为完整单株静态网格，无骨骼、无动画、无粒子；根部接地点中心为原点，+Y向上。实际尺寸、朝向和法线后续导入检查，不承诺生成器自动满足。

验收重点：从远处一眼看出“密而层叠的松树”，比高松冠幅厚重，比老松更直立紧凑；旋转摆放后仍呈现自然枝层。整株必须独立，没有森林、天空、地面、石头、人物、符文、标签、模型底盘。

### English full brief

Create one original dense layered pine, the second reusable tree species for a dark circular arena enclosed by woodland. Target 18 meters in height and about 8 meters maximum crown width. It is shorter, heavier and more continuous in silhouette than the sparse tall pine. Its job is to build overlapping forest depth and a readable woodland boundary, while remaining recognizable as a tree rather than a solid hedge. A substantial trunk around one meter thick is visible at the root and through roughly the lowest two meters; the crown begins around 2.5 meters above the ground.

Construct seven to nine uneven, thick branch tiers tapering upward. The bottom two tiers spread widest; the middle tiers are slightly offset from side to side; the upper leader is pointed and mildly off-center. Each tier consists of a few overlapping thick fan-shaped needle masses, with broad irregular edges. Avoid perfect horizontal discs, smooth cone shells or mathematical radial symmetry. Branch tips can droop but remain stout. Preserve a handful of conspicuous dark gaps between tiers to reveal the layered structure. The underside must have real volume and coherent branch connections, not invisible planes. Roots flare gently from the trunk and stay low without a separate dirt mound or scenic base.

Use muted deep green, gray-green and charcoal-blue foliage areas and cold gray-brown bark divided into broad plates. No bright seasonal colors, snow, flowers, fruit, ornaments, fluorescence or sunlit yellow tips. Lighting belongs to the runtime sky and central crystal. Base color must not contain directional cast shadows, fixed highlights, bright rim lighting or colored environmental glow. Prefer sparse broad tonal patches instead of photographic leaf texture or granular noise.

The target is a readable stylized low-poly model for a 320x200 internal render, approximately 2500 source triangles. Opaque volumetric foliage clusters are required; do not use alpha cards, individual needles, hair-like branches or complex material stacks. Supply UV mapping with one opaque base-color material preferred and two maximum. Entire single static tree, root-ground contact center at the origin, positive Y up, with no rig, animation, particles or LOD collection. Offline import will verify dimensions, actual polygon count, material flags and normals.

At distance this tree must read as a dense stack of irregular conifer boughs, clearly broader than the tall sparse species and more upright and compact than the crooked old species. Renderable from every side for repeated rotated placement. Exclude surrounding forest, sky, ground, stones, buildings, characters, runes, letters, labels and display stands.

### 实际 API 正向提示词（903/1024字符）

```text
One original isolated dense layered pine for a dark forest game at 320x200. Stylized low-poly, 18m tall and 8m wide. Stout tapering gray-brown trunk, roots and lower 2m visible. Seven to nine uneven thick bough tiers narrow upward to an off-center pointed leader. Wide bottom boughs, staggered middle layers, short drooping tips. Each tier has a few opaque volumetric faceted needle masses with broad irregular edges; visible gaps between tiers, never a smooth plastic cone. Muted deep green, gray-green and charcoal-blue color blocks, broad bark plates. Low thick roots at one ground plane. About 2500 source triangles, strong readable silhouette from all directions; no individual needles, fine twigs or alpha planes. Single complete static UV-mapped tree, one opaque base-color material, neutral colors with no baked lighting or glow. Root center pivot, +Y up, no surrounding ground or display stand.
```

### 实际 API 负向提示词（228/255字符）

```text
forest scene, terrain base, smooth cone, perfect symmetry, alpha planes, transparency, individual needles, fine twigs, high-poly, photographic noise, snow, Christmas ornaments, flowers, glowing tips, baked light, rig, text, logo
```


## 扭干阔冠老松 / Crooked ancient pine

### 中文完整说明

制作第三种独立松树：树龄很高、主干弯折、树冠横向展开的老松。目标高度20米，冠幅约12×11米，明显比另外两种更宽、更不对称。主干地面直径约1.4–1.8米，先稳固向上约5米，再出现一处大弯和一处较小回折，使重心看起来仍受粗根支撑；不要折成触手、螺旋弹簧或倒伏枯木。上部两条主要枝轴分叉成偏向一侧的宽冠，另一侧有短一些的回平衡枝。

树冠像四至六片高低不一、带厚度的破碎伞状针叶簇，彼此之间留出很大的缺口，露出结实的弯曲主枝。每个叶团是粗大的多面块簇，末端有宽钝锯齿，不能做单根针叶或透明贴片。最多两至三个明显断枝残桩，可以有一块宽的浅色剥皮疤痕，表达老龄与暴露，但不要遍布小洞、须根、寄生物。根部三至五条粗根贴地伸展，彼此仍属于完整一株树，不附带泥土台座。

树皮为深灰棕与石墨色，大块扭转裂纹沿干身方向；剥皮区域是暗淡灰褐，避免高亮白色吸引注意。树冠为墨绿、灰蓝绿的宽色块，整体暗而不死黑。不要自发光、发光蘑菇、蜘蛛网、血肉、人脸或拟人眼睛，恐怖感由不对称古老轮廓和场景照明产生。环境天空和水晶负责光照，不要烘焙日光或投影。

用于320×200低分辨率第一人称游戏，目标source约3000三角形。把预算留给弯干和主要开口，舍弃微小树皮与松针。单株从四面可读，完整UV，优先一个不透明base-color材质，最多两个。树冠是有厚度的实体低面网格，不依赖alpha。+Y向上，地面根中心为原点，静态无骨架无动画。实际重心、高度和朝向需离线检查；树冠碰撞不能简单套大盒，接地粗干与根部另制代理。

验收重点：远景中仍能识别一棵“歪而稳、冠很宽”的老松，不能与疏冠高松混淆；独立旋转和适度统一缩放后都能融入林缘。只生成这棵树，不要生成森林、地表、场景灯、巨眼、水晶、怪物、文字或商标。

### English full brief

Create one original ancient crooked pine, a third reusable species with a broad asymmetric crown for the forest edge of a cosmic-horror arena. Target height 20 meters, crown spread approximately 12 by 11 meters. The trunk is substantial, roughly 1.4-1.8 meters wide at the ground. It rises securely for about five meters, then makes one large bend and one smaller correcting bend. The tree must still look structurally rooted and balanced, not like a tentacle, a spring or a fallen log. Higher up, two primary branch axes carry most of the crown toward one side, with a shorter counterbalancing branch on the other.

The crown is four to six broken umbrella-like masses at different heights. Maintain large negative spaces between them to show the thick bent limbs. Foliage is built from opaque, volumetric, faceted needle clusters with a few wide blunt serrations. Never model individual needles or rely on transparent cards. Include no more than two or three obvious broken branch stubs. One broad old bark scar may show age; avoid a dense field of tiny holes, fibers, fungus or parasitic detail. Three to five thick low roots radiate from the same rooted trunk and meet a common ground plane without a soil plinth.

Bark uses dark gray-brown and graphite color areas, with broad twisting furrows following the trunk. The scar is dull gray-tan, not a bright white beacon. Foliage uses muted black-green and gray blue-green patches. Keep the material dark but distinguishable from pure black. Do not add glow, luminous fungi, webs, flesh, faces or eye-like features. Unease comes from the old asymmetric silhouette and runtime lighting. Do not bake sunlight, cast shadows, colored environment highlights or crystal glow into base color.

Designed for a first-person game with a 320x200 internal image. Aim for approximately 3000 source triangles, prioritizing trunk bends, principal limbs and crown openings over surface microdetail. The complete single tree must be readable from every direction, UV-mapped, preferably one opaque base-color material with two maximum. Positive Y up, root-ground center at origin, static geometry without skeleton or animation. Final size, grounding, normals and topology will be checked during offline import; simplified collision will match the actual thick trunk and low roots rather than enveloping the whole canopy.

The result must immediately read as a wide, crooked but stable old pine, distinct from the slender sparse pine and compact layered pine. It should tolerate rotation and modest uniform scale variation when repeated at the forest margin. Exclude other trees, terrain, lighting props, eye, crystals, creatures, lettering, signs and logos.

### 实际 API 正向提示词（877/1024字符）

```text
One original isolated ancient crooked pine for a dark 320x200 forest game. Low-poly silhouette, 20m tall, broad 12m crown. Thick gray-brown trunk rises 5m then bends strongly and bends back slightly, firmly rooted and balanced. Two stout upper limbs carry four to six separated umbrella-like crowns, asymmetric with broad gaps and a short balancing limb. Opaque chunky faceted needle clusters in muted black-green and blue-green, no thin needles or alpha cards. Two blunt broken branch stubs and one broad dull bark scar; broad twisting bark grooves. Three to five thick low roots at ground level. Believable old conifer, not a tentacle or humanoid. About 3000 source triangles, readable from all sides. Complete static UV-mapped tree, one opaque base-color material, neutral unlit colors without baked shadows or glow. Root-ground center pivot, +Y up. No soil base or scenery.
```

### 实际 API 负向提示词（231/255字符）

```text
forest scene, ground base, alpha cards, transparent leaves, tiny needles, fine roots, high-poly, photoreal noise, face, eyes, flesh, tentacles, spiral spring, glowing fungus, web, flowers, ornaments, baked lighting, rig, text, logo
```


## 冲天异文巨晶 / Runic crystal spire

### 中文完整说明

制作单根冲天的巨大晶柱，作为超大型圆形森林竞技场的唯一中央主地标。目标高度140米，基底最大宽度约16米（12–18米可接受），高宽比约9:1，整体远高于周围18–22米松林。不是小型水晶簇、短胖宝石或祭坛雕像：主体是一根向极高尖端收束的狭长矿物山峰，底部有少量与主体连生的宽厚碎面，其高度不得接近主尖端，不能变成许多平行同高的针柱。

轮廓由五至七条不规则纵向棱脊组织，主轴轻微扭转但仍接近垂直。最宽处位于地面附近，下部约四分之一高度缓慢收窄，上部逐渐变尖；顶尖略偏心。大晶面应有长而不均匀的梯形/三角形断面，局部三至五处宽大崩口和纵向裂缝，形成古老不自然的形态。保留几片足够宽、连续的正侧面，供游戏运行时投射/绘制扭曲符文。底部直接接触共同地面，不要台座、岩盘、地形裙边或祭坛台阶；单根晶柱自身就是碰撞遮挡物。

表面以接近黑色的靛蓝、冷紫与石墨蓝为主体，少量较浅的蓝紫矿物区暗示内部能量，但模型保持完全不透明。绝不生成玻璃透明、折射材质或镜面铬；晶体的亮边、核心发光、地面照明与符文脉动交给运行时。base-color中不要烘焙光晕、粒子、光柱、白色过曝裂缝或照射到地面的光。大晶面的材质变化应稀疏，避免整面亮蓝噪声。

在两至三片大晶面上，只需要少数暗色、极浅的符文笔画导向：扭曲的断环、弯钩、岔开的竖线、眼状椭圆与不完整交叉。它们构成疏密不均的纵向带，略微顺着晶柱的扭势，不要铺满成可读段落。每个符号应在现实尺度约3–6米高，线宽至少0.3–0.6米；可以用宽浅刻痕或base-color暗纹表示，不能用极细立体浮雕。它们完全虚构，不用拉丁字母、数字、真实宗教符号或任何现成游戏的文字。动态发光版本由shader覆盖或加强，生成模型不必把完整符文系统雕得很精密。

这是一件用于320×200游戏画面的低面数地标，目标source约4000三角形。细节多指设计清楚，不是高密度建模：预算集中在外轮廓、大晶面和几个崩口。要求完整封闭主体、合理法线、UV和一个不透明base-color材质，无动画、无粒子、无透明叠层。+Y向上，底部中心为原点，完整顶尖不得裁切。导入时统一高度并检查实际宽度和符文可读性，再单独制作贴合实体的低面碰撞代理；不以提示词承诺网格水密性。

验收重点：从竞技场任意位置都能读出狭长冲天晶柱；接近时表面保留大面积清晰色面，而非细碎像素噪声；整个模型只有一根主峰，根部不附带挡路的大片模型地面。不要生成森林、圆形场地、天空、眼球、云、地面图腾、人物、枪械、建筑、文字标签或底座。

### English full brief

Create one original colossal jagged crystal spire, the unique central landmark of an enormous circular limestone arena surrounded by pine forest. Target height 140 meters with a roughly 16-meter basal spread, acceptable 12-18 meters, giving an approximate nine-to-one height-to-width ratio. It must tower far above 18-22-meter trees. This is one continuous soaring mineral peak, not a small crystal cluster, a squat jewel or an altar statue. A few low broad intergrown cleavage masses may reinforce the base, but never compete with the dominant apex or produce a bouquet of parallel needles.

Organize the outline around five to seven uneven longitudinal ridges. The main axis twists subtly while remaining nearly upright. The lower quarter tapers gradually from the widest region near the ground; above it, long facets converge toward one slightly offset acute tip. Large trapezoidal and triangular cleavage faces have uneven proportions. Three to five broad chips and deep but sparse vertical fractures give the landmark an ancient unnatural character. Retain several wide uninterrupted front and side facets suitable for runtime runic overlays. The base contacts a shared flat ground plane directly: no plinth, rock disc, altar steps, attached terrain skirt or separate scenic foundation.

Use near-black indigo, cold violet and graphite-blue mineral color areas, with a few quieter lighter blue-violet inclusions suggesting an inner core. The mesh must remain entirely opaque. No glass transmission, transparency, refraction, chrome or mirror material. Runtime rendering will create luminous edges, core energy, ground illumination and rune pulses. Base color must not bake in bloom, particles, light rays, overexposed white cracks, cast light pools or fixed directional lighting. Keep facet variation sparse and broad, never a carpet of brilliant blue noise.

On two or three principal faces, include only a few shallow dark fictional glyph guides: warped incomplete rings, hooks, forked vertical strokes, eye-like ovals and interrupted crossings. They form loose vertical bands following the subtle twist, with substantial blank facets between them. Each glyph is roughly 3-6 meters tall at final scale, using strokes around 0.3-0.6 meters thick. Suggest them through shallow broad grooves or dark painted mineral marks, not fine embossed calligraphy. These are invented nonverbal shapes, without Latin letters, numbers, real religious symbols or copied game alphabets. Their animated luminous expression belongs to a runtime shader, so generation does not need to solve a dense precisely carved writing system.

This asset is intentionally low-poly for a 320x200 game image. Target approximately 4000 source triangles. Detailed instructions define clear design decisions, not microscopic geometry: spend topology on the dominant outline, broad facets and a handful of fractures. Request a coherent closed main solid, sensible outward normals, UVs and one opaque base-color material. Static model only, no skeleton, particles, transparent shells or animation. Positive Y is up and the contact base center is the origin. Include the entire tip. Offline import will normalize height, inspect proportions and rune readability, and build a separate fitted collision proxy; text instructions alone cannot guarantee watertight output.

Success is a singular narrow sky-piercing landmark visible from anywhere in the arena, with broad legible surfaces when approached and no attached ground that blocks traversal. Exclude forest, circular arena, sky, eye, clouds, floor totem, characters, weapons, buildings, captions, lettering and display base.

### 实际 API 正向提示词（894/1024字符）

```text
One original colossal runic crystal spire for a dark 320x200 game. Single soaring peak, 140m high and 16m wide at base, not a cluster. Five to seven uneven vertical ridges twist subtly upward; broad angular facets taper to one offset sharp apex. Few low intergrown basal cleavage masses, sparse large chips and deep vertical fractures. Wide uninterrupted faces carry sparse shallow dark fictional glyphs: warped broken rings, hooks and forked strokes in loose vertical bands, bold not tiny. Opaque near-black indigo and cold violet mineral with broad muted blue core patches. No glass or baked glow: runtime shaders add light and rune emission. Low-poly roughly 4000 triangles; silhouette and large facets over microdetail. One complete coherent static UV-mapped solid, one opaque base-color material, +Y up, flat base center at origin. No pedestal, terrain, scene, eye, sky or floating shards.
```

### 实际 API 负向提示词（239/255字符）

```text
crystal bouquet, short gem, glass, transparency, chrome, neon, bloom, baked lighting, particles, floating shards, tiny cracks, dense writing, letters, numbers, religious symbols, high-poly, pedestal, terrain, forest, eye, clouds, rig, logo
```
