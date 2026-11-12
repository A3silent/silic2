# Silic2 · Tripo 五资产生成前审阅

状态：**低分辨率造型修订版，已于2026-10-17获用户确认。五件资产已全部生成并下载，合计消耗200 credits；已完成实际场景接入与工程检查，详细结果见 docs/tripo-asset-pass.md。** 免费额度按API实际余额与单次报价核对；不会购买额度或自动生成额外资产。

**详细提示词用于约束美术，不代表追求精细模型。** 以320×200的游戏画面为准：大轮廓、大中尺度形体、粗壮结构、简洁哑光色块与克制纹理噪声。省略密集螺丝、刻线、微划痕、微孔和微碎石；宇宙恐怖气氛通过比例、缺损、空腔和材质对照建立。

中文段落是完整美术与验收说明；每项英文正向和负向文本是拟实际提交的生产提示词，分别不超过1024和255字符。三角形、尺寸与朝向是导入目标，生成器不能靠文字保证。硬上限20000是防护线，并非每个模型都要用满；优先按较低目标生成、导入后统计。

## 1. 轨道遗留宇宙步枪

目标 **6,000 三角形**，硬上限 **20,000**；包络宽×高×长 **0.2 × 0.3 × 0.85 米**。拟请求seed `1701701`，不保证确定性复现。

轨道遗留步枪：完整美术与制作说明

低分辨率优先（最新用户要求）：提示词丰富用于明确造型取舍，不要求模型精细写实。最终游戏以320×200显示，优先大轮廓、大中尺度形体、粗壮结构和简洁哑光明暗色块；保留远看能辨认的断口与镂空。主动省略微小螺丝、密集刻线、微划痕、细丝、微孔和砂砾地毯；材质噪声克制。采用风格化低面数表现，减少在画面中无法辨认的几何，不用高面数补偿不清楚的轮廓。

用途：替换现有第一人称武器外观，维持单枪管、每秒约五次射击和当前单一命中逻辑。它是已经组装好的单体静态步枪，不是广告图、武器陈列架或新武器系统。画面中从右下方进入，枪口朝瞄准方向，枪身上方保持低矮，避免遮挡中央敌人与准星。

造型层级：首先读出短而坚实的机匣和一根直枪管；其次辨认短镂空枪托、独立握把、完整扳机护圈和前握持区域；最后才看见一两处宽冷却槽和简洁装甲接缝；不生成密集螺丝。原型意象为被矿化侵蚀的轨道采掘器械，但仍具有可信的机械功能。整体偏工业、磨损、冷硬，不是现代制式武器复刻，也不需要夸张尖角。

细部要求：机匣由宽面和错位护甲构成，前段有少量纵向凹槽。枪管最前端为有厚度、清楚凹入的单个枪口，不得封成球或水晶尖。枪托与机匣有结实连接，不能浮空。握把能容纳人手但不生成手；护圈与扳机之间留真实空隙。顶部只有低矮机械瞄具，不放大镜筒、长天线、大刀刃或额外炮管。左右适度不对称，但功能中心线明确。

材质：主体哑光炭黑金属，凹槽更暗；边缘有宽而克制的冷灰擦痕。几块旧象牙色陶瓷板帮助识别结构，面积低于主体。暗蓝矿化只沉积在接缝与保护角落。不要高亮能量条、镜面镀铬、透明管路、彩色霓虹或鲜红外壳。表面纹理以中大尺度磨损为主，避免密集细噪声在320×200画面中闪烁。

动态边界：模型静止、无骨骼、无动画、无绑定、无手臂；后坐、摆动与枪口火由现有游戏表现驱动。不得烘焙火焰、光束、烟雾或正在出膛的弹丸。模型只表达连续单发射击的结构，不能暗示转管、双发、霰弹、蓄力或追踪功能。

制作合同：目标6000三角形，硬上限20000；包络约宽0.20米、高0.30米、长0.85米，最终按真实下载网格校正。导入约定Y向上，枪口朝-Z，枪托朝+Z；不假定生成结果自动遵守。KINETIC必须在实物上标定枪口坐标与持枪姿态，不能凭提示词写死。保留UV与不透明base-color，尽量少材质分区，不需要PBR全套贴图才能工作。全枪完整入镜，无底座、背景、文字、编号或品牌。

使用方式：本说明用于完整设计和验收；提交API时使用同目录prompt.en.txt及negative.en.txt。长说明不是要直接塞入受长度限制的API字段。

### 拟提交英文正向提示词

893 / 1024 字符。

```text
One original rigid sci-fi horror rifle for a fast first-person arena shooter rendered at 320x200. Bold low-poly silhouette, chunky useful shapes, simple matte color blocks; deliberately stylized rather than detailed photorealism. A short orbital salvage carbine with charcoal receiver, one straight barrel and clearly recessed single muzzle, short skeletal stock, sturdy grip, enclosed trigger guard, compact foregrip and low sight rail. Asymmetric broad armor planes, a few large ivory ceramic inserts, one or two deep cooling slots. Ancient industrial equipment altered by alien minerals. Sparse dark blue seams and broad edge wear only; no tiny hardware or dense surface noise. Credible repeated single shots at five per second. Entire isolated assembled rifle, opaque UV-mapped static mesh, economical topology. No hands, flash, beam, huge scope, rig, text, logos, scenery or display base.
```

### 拟提交负向提示词

227 / 255 字符。

```text
hands, people, text, logos, dual barrels, huge scope, muzzle flash, beam, rig, animation, floating parts, glass, neon, photorealism, high-poly detail, tiny screws, fine engraving, micro scratches, dense noise, scenery, pedestal
```

### 摆放／接入意图

由KINETIC在真实模型上标定枪口与持枪变换，只替换共享第一人称外观；射速、伤害和射线保持。两张地图均使用新的共享武器。

## 2. 大型坠毁宇宙飞船

目标 **12,000 三角形**，硬上限 **20,000**；包络宽×高×长 **38 × 18 × 16 米**。拟请求seed `1701702`，不保证确定性复现。

坠毁轨道巨舰：完整美术与制作说明

低分辨率优先（最新用户要求）：提示词丰富用于明确造型取舍，不要求模型精细写实。最终游戏以320×200显示，优先大轮廓、大中尺度形体、粗壮结构和简洁哑光明暗色块；保留远看能辨认的断口与镂空。主动省略微小螺丝、密集刻线、微划痕、细丝、微孔和砂砾地毯；材质噪声克制。采用风格化低面数表现，减少在画面中无法辨认的几何，不用高面数补偿不清楚的轮廓。

叙事：一艘不明归属的工业轨道运输／采掘船在极久以前坠入晶体侵蚀的巨构边缘。它彻底失去动力，破坏来自撞击、长期应力和后来的矿物入侵。观众先认出巨大人造船体，再注意到被异物侵占的不安感。不是正在爆炸的飞船，也不是黑色完整战斗机。

主轮廓：长条而厚重的工业舰体，中段严重断裂。三大残存体量通过扭曲龙骨保持连接：压碎下折的舰艏、撕开侧面的中段、抬起并露出两个凹入引擎喉口的舰尾。至少一侧仍有连续外壳帮助识别原本船型，另一侧大面积剥离。主断口产生宽阔负形，远看也能明确读出中间缺失的部分。

破坏细节：厚装甲向外卷曲，边缘露出夹层和加强筋；内部甲板错位，横向肋梁弯折，少量粗管道齐根断裂。所有板材有实体厚度。优先几个大洞、几处大扭转和几层错位甲板，而不是满船细小刻痕。破片尽量与主体接触，不生成悬浮粒子、密集线缆团或毫无支撑的装饰。内部为空洞废墟，不要求可供玩家探索的完整房间。

材质：炭黑冷灰金属为主，接缝深褐氧化，断边有灰白磨损，少量旧骨白耐热陶瓷。稀疏暗蓝矿脉沿旧断口攀入，维持Crystal主题。引擎熄灭，无橙色尾焰、红灯、警告条、激光和爆炸光。不要把破碎结构全部画在纹理上；大型断口必须由几何表达。表面大块积尘、烧蚀与矿化足够，低分辨率不需要密集greeble。

摆放意图：首版作为北侧外圈不可达地标，约长38米、宽16米、高18米。整体在现有外壳之后，抬起的舰尾与断裂龙骨从墙上露出。出发点、上层弧桥应能辨认它，但不盖住整个天空，不与中央准星区域长期重叠。完整旋转后的边界必须在可达区外，不能只检查模型中心点。没有下载资产和实际视角前，坐标只是候选。

工程合同：单个静态游戏资产，目标12000三角形、硬上限20000三角形；材质分区尽量不超过四个。保留大形、厚断口与真实镂空，减少不可见内部结构。无骨骼、无动画、无地形底台、无场景背景、无人类或标志文字。首轮不作为可行走表面，不新增飞船内部路线，不自动把高细节网格当作碰撞体。导入检查法线、UV、厚度与实际尺寸。

使用方式：本说明供完整设计和验收；API使用同目录prompt.en.txt及negative.en.txt。

### 拟提交英文正向提示词

873 / 1024 字符。

```text
One enormous crashed orbital vessel, a static cosmic-horror prop for 320x200 gameplay. Stylized low-poly forms, heavy silhouette, large damage shapes and simple matte color regions; not intricate photorealism. Three ruined industrial hull masses remain joined by a twisted thick spine. Crushed prow bends down; stern rises with two large dead engine throats. One flank is torn away, exposing a few thick bent ribs and broken deck slabs; opposite flank preserves the ship identity. Wide missing sections make strong openings. Thick curled plates and broad fractures, not fine cables or greebles. Charcoal hull, muted brown seams, a few bone ceramic plates, restrained blue mineral intrusion. Entire isolated wreck, no surrounding landscape, crew, smoke, flames or lights. Clean economical static opaque UV-mapped mesh, broad shapes readable from afar; no rig, text or logos.
```

### 拟提交负向提示词

246 / 255 字符。

```text
intact ship, pristine symmetry, flying ship, flames, smoke, lights, people, text, logos, terrain, diorama, floating debris, thin wires, greebles, tiny screws, dense panel lines, micro scratches, noisy textures, photorealism, high-poly detail, rig
```

### 摆放／接入意图

- `north exterior skyline`：候选位置 `[0, 3, -47]`，旋转 `[0, 0, -8]`；Decorative; entire rotated geometry outside reachable shell, inspect stern visibility over north wall.

## 3. 不可分类巨型生物骨架

目标 **10,000 三角形**，硬上限 **20,000**；包络宽×高×长 **13 × 14 × 32 米**。拟请求seed `1701703`，不保证确定性复现。

不可分类巨型遗骸：完整美术与制作说明

低分辨率优先（最新用户要求）：提示词丰富用于明确造型取舍，不要求模型精细写实。最终游戏以320×200显示，优先大轮廓、大中尺度形体、粗壮结构和简洁哑光明暗色块；保留远看能辨认的断口与镂空。主动省略微小螺丝、密集刻线、微划痕、细丝、微孔和砂砾地毯；材质噪声克制。采用风格化低面数表现，减少在画面中无法辨认的几何，不用高面数补偿不清楚的轮廓。

核心感受：这曾经是庞大生命，但无法按地球生物分类。它不是新敌人、Boss尸体、人类骷髅、恐龙或巨龙。宇宙恐怖来自异常解剖、巨大尺度、漫长时间与缺失结构，避免血肉和尖牙带来的普通怪兽感。

骨架组织：一条粗大、分节、弯曲的脊柱组成破碎新月，两侧连接错位的双列厚肋。肋骨长短、间隔、弯曲方向不同；几根融合，几根折断，部分落在低处脊柱上形成合理支撑。保留足够大的空隙，从远处可以穿过肋骨看见天空。不是整齐等距骨架，也不是细线弯成的笼子。

头部与尾部：一端是钝重、多孔、像头颅却没有脸的骨质体。孔洞偏心、大小不同，不能组合成标准双眼、鼻孔、牙床。少量内翻骨板暗示未知器官连接，不出现可识别的恐龙长吻、龙角或人类颌骨。另一端是一串逐渐破碎的粗尾椎，不能以大量极细针状骨刺收尾。

材质：干燥骨质部分矿化成石头。外脊低饱和旧象牙色，断面粉化灰白，凹腔灰黑沉积。破口偶有暗蓝晶体侵入，骨仍占主导。表面是剥落、分层和被磨平的区域，不是湿润反光、粘液、血迹或皮肤。骨的亮度在游戏里需要压低，不能盖过敌人与子弹。不要红色攻击暗示或发光眼。

构图：拟作为东侧外壳之后的尺度地标，长32米、宽13米、最高14米。沿外圈延展，弧形肋条从墙上方露出，与北侧飞船形成机械／生命两类残骸的对照。首轮不跨越主战场、不成为强制穿行门洞、不压住上层桥与坡道；所有伸出的肋尖都要检查，不以中心点在外圈为合格依据。

工程合同：目标10000三角形、硬上限20000；一个连贯的静态遗骸，允许几块接触主体的独立骨片网格。厚肋和宽洞口优先，微孔可以省略。无绑定、无姿态动画、无博物馆支架、标牌、人形比例尺、矩形岩台或完整地面。实体骨片落点与支撑需检查，避免浮空。首轮不可达装饰不增加复杂骨架碰撞。

使用方式：本说明供完整设计和验收；API使用同目录prompt.en.txt及negative.en.txt。

### 拟提交英文正向提示词

885 / 1024 字符。

```text
One colossal dry fossil skeleton of an unclassifiable alien, a static cosmic-horror prop for 320x200 gameplay. Stylized low-poly sculpture, chunky bones and strong open silhouette; not realistic microdetail. Thick segmented spine curls into a broken crescent with irregular asymmetric double rows of broad ribs. A few missing ribs create wide openings; broken ribs rest on the lower spine for support. One end has a blunt skull-like mass with a few large offset cavities, no human face, paired eyes or dinosaur anatomy; heavy fractured vertebrae taper at the other end. Matte desaturated ivory planes, gray broken faces, dark cavities and sparse broad blue mineral patches. Ancient and still, no flesh or gore. One coherent remain, isolated with no terrain base. Opaque UV-mapped economical static mesh, simplified large and medium forms; no tiny teeth, thin wires, rig, text or logos.
```

### 拟提交负向提示词

225 / 255 字符。

```text
human, dinosaur, dragon, living monster, flesh, gore, glowing eyes, thin bones, tiny teeth, tiny pores, micro scratches, dense noise, photorealism, high-poly detail, text, logos, terrain, pedestal, rig, animation, glass, neon
```

### 摆放／接入意图

- `east exterior skyline`：候选位置 `[43, 2, 0]`，旋转 `[0, 12, 0]`；Decorative; rib tips outside playable shell, no arch crossing combat route.

## 4. 不规则矿化水晶簇

目标 **4,000 三角形**，硬上限 **20,000**；包络宽×高×长 **3.6 × 5.2 × 3.2 米**。拟请求seed `1701704`，不保证确定性复现。

不规则矿化水晶簇：完整美术与制作说明

低分辨率优先（最新用户要求）：提示词丰富用于明确造型取舍，不要求模型精细写实。最终游戏以320×200显示，优先大轮廓、大中尺度形体、粗壮结构和简洁哑光明暗色块；保留远看能辨认的断口与镂空。主动省略微小螺丝、密集刻线、微划痕、细丝、微孔和砂砾地毯；材质噪声克制。采用风格化低面数表现，减少在画面中无法辨认的几何，不用高面数补偿不清楚的轮廓。

用途：连接人工轨道建筑与未知矿物入侵的可复用静态道具。可旋转、等比缩放，嵌在墙根、已有中心晶壳或不可达外圈。不是攻击物、可采集资源、传送点或魔法发光宝石。

轮廓：一根偏斜而宽厚的主晶柱决定生长方向，五根次级晶体围绕它错落嵌生。各自粗细、高度和倾角有差异，共享紧凑根岩。几根沿主柱挤压出裂纹，至少两根顶部断裂成宽阔平面，不是所有尖端都完美锋利。避免径向对称、整齐花束和相同针状尖刺。

表面：以大平面、清楚棱线、略钝断边为主；断面有层状劈裂和少量灰白夹杂。保留实体厚度，不是单面玻璃。自然条带与粒状矿物只辅助大形，不能覆盖成密集闪烁噪声。晶体之间要有能在320×200下识别的负形。

色彩：不透明暗蓝灰矿物，根部蓝黑，受光大面略偏低饱和青色。灰白断边很少，绝大部分不自发光、不折射、不呈彩虹。不要艳紫宝石、霓虹、玻璃管或金属金边。异变颜色由游戏shader统一驱动，材质不内置白热光斑或暖色警报。

基座与尺度：底部为低而自然断裂的根岩，不做圆形雕塑台或方形地台。默认包络约3.6×5.2×3.2米，次级实例可以缩至约一米高。保持基本比例，避免非均匀拉伸失去晶体质感。真实模型下载后检查最大倾斜与突出部分，不能按描述尺寸假设能塞入墙角。

工程合同：目标4000三角形，硬上限20000。一个完整静态簇，清晰UV，不透明base-color，材质不超过两个。高细节视觉网格不自动参与碰撞；可达区域如需实体阻挡，必须先由VECTOR确认简化代理、导航和射线一致性。首轮优先占用现有实体内部或外圈，不修改通路。

使用方式：本说明供完整设计和验收；API使用同目录prompt.en.txt及negative.en.txt。

### 拟提交英文正向提示词

843 / 1024 字符。

```text
One irregular opaque alien mineral crystal cluster for a 320x200 cosmic-horror game. Strong stylized low-poly silhouette, broad flat facets and simple muted color blocks. One thick leaning main shard and five shorter asymmetric intergrown shards with clearly varied heights and widths. Several blunt broken tips and large cleavage planes, not a neat radial bouquet or thin needles. Compact low fractured rock root. Matte dark blue-gray stone with nearly black roots, restrained dull cyan facet planes and a few broad gray fracture edges. Solid opaque mineral, never glass, transparent gems or neon. Simplify detail to silhouette, broad fractures and sparse color variation; no granular surface noise or tiny splinters. Entire isolated static UV-mapped game prop, economical closed geometry, no pedestal, scenery, particles, rig, text or logos.
```

### 拟提交负向提示词

233 / 255 字符。

```text
glass, transparency, refraction, glow, neon, rainbow, symmetry, thin needles, tiny splinters, granular noise, micro scratches, photorealism, high-poly detail, floating shards, particles, text, logos, pedestal, terrain, rig, animation
```

### 摆放／接入意图

- `central existing crystal shell`：候选位置 `[0, 0, 2]`，旋转 `[0, 27, 0]`；Conditional. Reject protrusion beyond current solid footprint or new sightline obstruction; must inspect actual model.
- `northwest exterior shell`：候选位置 `[-24, 0, -25]`，旋转 `[0, 61, 0]`；Decorative beyond radius28 play boundary, inspect wall occlusion.
- `southeast exterior shell`：候选位置 `[25, 0, 24]`，旋转 `[0, -34, 0]`；Decorative beyond radius28; preserve south rift silhouette.

## 5. 岩层断裂碎石与地面装饰

目标 **3,000 三角形**，硬上限 **20,000**；包络宽×高×长 **4.2 × 0.65 × 3.0 米**。拟请求seed `1701705`，不保证确定性复现。

层状断裂地面碎石：完整美术与制作说明

低分辨率优先（最新用户要求）：提示词丰富用于明确造型取舍，不要求模型精细写实。最终游戏以320×200显示，优先大轮廓、大中尺度形体、粗壮结构和简洁哑光明暗色块；保留远看能辨认的断口与镂空。主动省略微小螺丝、密集刻线、微划痕、细丝、微孔和砂砾地毯；材质噪声克制。采用风格化低面数表现，减少在画面中无法辨认的几何，不用高面数补偿不清楚的轮廓。

用途：让轨道地面出现剥落、断层和大小不一的碎石，作为可复用的小范围装饰单元。不是整张地形，不是深裂谷，不强迫玩家跳跃，也不引入可破坏机制。主要放在已有墙根、支撑与战术掩体的基部。

组成：很浅的层状岩板分成三层错开的剥离面，外围两块宽扁大碎块，四块中等楔形片，再辅以少量仍清楚可辨的碎片。分布呈不规则拖尾或弯月，一侧密一侧稀。不要均匀圆环、对称花纹，也不要所有碎片尺寸相同。碎片之间保留地面空隙，不生成整个包络都是实体的矩形底板。

断裂语言：大块能看出共同层理方向，断面锋利但有厚度，边缘掉角，中层略错动，少量片状结构搭叠。表现岩层剥离与压裂，不是圆滑鹅卵石。最高块低于宽度六分之一，整体宽而扁，避免高耸石堆或针状岩刺。

材质：炭黑板岩、低饱和灰褐沉积层、浅灰粉尘断面、少量冷蓝矿脉。没有熔岩发光裂隙、草、苔藓或刺眼白石。中大尺度的断口明暗比细砂纹理重要，不用数百颗均匀小砾石堆出复杂度。所有碎块有明确落地关系，不浮空。

摆放与物理：默认包络约4.2×0.65×3米。较高碎块必须位于已有不可达／实体范围，不能让玩家明显穿过石头。开放可走地面如需放置大碎块，要有相应简化碰撞和导航复核；仅极低薄片才适合作为非阻挡表面细节。不得堵坡道、桥面、桥下净空、出生点和侧路。装饰坐标需要真实模型边界验证，未生成前不写入正式地图。

工程合同：目标3000三角形，硬上限20000；一个静态复用资产，可含多个独立但落地的碎片网格。每块有厚度，底面减少不可见细节。清楚UV和不透明base-color，材质不超过两个；不要背景、人物、完整建筑、地形平面、文字和标志。不能把每粒石子的高精度网格都加入角色碰撞查询。

使用方式：本说明供完整设计和验收；API使用同目录prompt.en.txt及negative.en.txt。

### 拟提交英文正向提示词

857 / 1024 字符。

```text
One low irregular rubble patch for a 320x200 cosmic-horror orbital arena. Stylized low-poly fractured slate: a shallow slab separates into three broad offset layers, with two large flat chunks, four medium angular wedges and only a few small but clearly readable chips. Wide low silhouette with open gaps, chunky solid thickness, no rectangular base. Fractures follow simple visible strata; overlapping slabs suggest torn ground. Tallest chunk below one sixth of patch width. Matte charcoal and muted gray-brown color blocks, broad dusty gray break faces, one sparse dull blue mineral seam. Prioritize large and medium forms; no tiny gravel carpet, dense scratches or realistic microtexture. Fragments rest on a common ground plane. Isolated reusable static opaque UV-mapped prop with economical topology, no terrain scene, plants, glow, rig, text or logos.
```

### 拟提交负向提示词

242 / 255 字符。

```text
boulder mountain, tall spikes, deep pit, rectangular tile, terrain scene, plants, moss, glowing cracks, tiny gravel carpet, tiny fragments, micro scratches, dense noise, photorealism, high-poly detail, floating stones, text, logos, rig, glass
```

### 摆放／接入意图

- `east existing tactical cover base`：候选位置 `[23, 0, -9]`，旋转 `[0, -25, 0]`；Conditional. Tall pieces contained in existing cover footprint; no large noncollidable stones on accessible floor.
- `west existing buttress base`：候选位置 `[-26, 0, -12]`，旋转 `[0, 40, 0]`；Conditional. Inside existing support/wall footprint; keep x=-22 route clear.
- `west existing buttress base`：候选位置 `[-26, 0, 12]`，旋转 `[0, -70, 0]`；Conditional. Inside existing support/wall footprint; keep x=-22 route clear.

## 整体摆放与范围

- 飞船北侧外围、骨架东侧外围，作为巨型地标；水晶与碎石优先嵌入已有实体基部或外圈。保护坡道、桥上桥下、出生点和循环路线。
- 候选最多一艘飞船、一副骨架、三簇水晶、三组碎石，目标新增环境约43,000三角形，武器约6,000。实际性能与可读性优先，下载后按真实网格边界与画面减少实例或简化。
- 资产仍本地保存，不推送GitHub；提示词和非二进制清单可追踪。
- 可选船壳板、单根断骨不单独生成；优先复用五件资产，额外额度暂不消费。

**当前没有生成结果、正式地图新引用或完成的模型摆放。候选坐标不等于验收通过。**
