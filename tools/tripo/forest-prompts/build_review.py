#!/usr/bin/env python3
"""Rebuild human review + hash manifest from the authored text files (offline)."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent
ASSETS = [
    ('01_pine_sparse', '疏冠高松 / Tall sparse-crowned pine', 'pine_sparse', [7, 22, 7], 2000),
    ('02_pine_dense', '层叠密冠松 / Dense layered pine', 'pine_dense', [8, 18, 8], 2500),
    ('03_pine_crooked', '扭干阔冠老松 / Crooked ancient pine', 'pine_crooked', [12, 20, 11], 3000),
    ('04_crystal_spire', '冲天异文巨晶 / Runic crystal spire', 'crystal_spire', [16, 140, 16], 4000),
]
intro = '''# 森林巨晶竞技场：Tripo 建模提示词审阅

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
'''
sections = [intro]
status = json.loads((ROOT/'submission-status.json').read_text()) if (ROOT/'submission-status.json').exists() else {'status':'pending_user_prompt_approval'}
manifest = {'revision':'forest-v1',**status,'model':'P1-20260311','assets':[]}
for folder, title, runtime, dimensions, faces in ASSETS:
    p = ROOT / folder
    texts = {f:p.joinpath(f).read_text().strip() for f in ['brief.zh.txt','brief.en.txt','prompt.en.txt','negative.en.txt']}
    assert len(texts['prompt.en.txt']) <= 1024, (folder,len(texts['prompt.en.txt']))
    assert len(texts['negative.en.txt']) <= 255, (folder,len(texts['negative.en.txt']))
    hashes = {f:hashlib.sha256(p.joinpath(f).read_bytes()).hexdigest() for f in texts}
    manifest['assets'].append({'id':folder,'runtime_id':runtime,'target_dimensions_xyz_m':dimensions,
      'parameters':{'face_limit':faces,'texture':True,'pbr':False},
      'positive_characters':len(texts['prompt.en.txt']),'negative_characters':len(texts['negative.en.txt']),
      'sha256':hashes})
    sections.append(f"\n## {title}\n\n### 中文完整说明\n\n{texts['brief.zh.txt']}\n\n### English full brief\n\n{texts['brief.en.txt']}\n\n### 实际 API 正向提示词（{len(texts['prompt.en.txt'])}/1024字符）\n\n```text\n{texts['prompt.en.txt']}\n```\n\n### 实际 API 负向提示词（{len(texts['negative.en.txt'])}/255字符）\n\n```text\n{texts['negative.en.txt']}\n```\n")
(ROOT/'REVIEW.md').write_text('\n'.join(sections))
(ROOT/'review-manifest.json').write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n')
print(json.dumps([{'id':a['id'],'positive':a['positive_characters'],'negative':a['negative_characters']} for a in manifest['assets']],indent=2))
