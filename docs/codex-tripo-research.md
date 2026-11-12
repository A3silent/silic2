# Codex 连接 Tripo 调研

调研日期：2026-10-17（Asia/Hong_Kong）。任务：TRIPO-RESEARCH。只做文档、发布包静态检查与本机只读检查；没有安装、登录、修改 Codex 配置或发起付费生成。

## 结论

能连接。Tripo 官方现有 **Tripo 3D Codex 插件**、**Tripo CLI** 和 CLI 自带 **MCP** 三种入口。优先用官方 Codex 插件；当前账号的插件搜索没有返回 Tripo 时，可直接让 Codex 使用官方 CLI。CLI 不需要 Blender 或 Python。不要把早期 `uvx tripo-mcp` 当成唯一方案。[官方 Codex 插件教程](https://developers.tripo3d.ai/en/docs/codex-plugin)、[CLI 文档](https://developers.tripo3d.ai/en/docs/cli)

## 推荐入口：官方插件或 CLI

官方插件教程：在 Codex 的 Plugins → Plugins Directory 搜索 **Tripo 3D**，安装启用；首次使用时由 Codex 展示设备授权 URL 与一次性验证码，用户在浏览器中登录并核对验证码。无须把 API key 发到聊天。官网也明确给出“搜索不到时改用 CLI”的备选方案。[来源](https://developers.tripo3d.ai/en/docs/codex-plugin)

本次插件目录搜索 `Tripo` 返回空；这只表示当前搜索没有结果，不证明官方插件不存在，也没有验证其他账号或地区的上架状态。

CLI 最小安装与检查流程：

```bash
node -v
npm install -g tripo-cli
tripo login
tripo doctor
```

要求 Node.js 20+。`tripo login` 适合交互终端；由 agent 在非交互终端操作时需要指定账号站点：国际站 `tripo login --region ov`，中国大陆站 `tripo login --region cn`。站点按账号归属选择，不能只凭操作系统语言猜测。用户在浏览器完成设备授权，CLI 保存本地 profile。`doctor` 检查身份、网络和余额等。[CLI 文档](https://developers.tripo3d.ai/en/docs/cli)、[官方 npm 发布包 0.7.0](https://registry.npmjs.org/tripo-cli/-/tripo-cli-0.7.0.tgz) 中 `skill/commands/account.md`、`dist/commands/login.js`

本机只读检查：Node/npm 位于 `/opt/homebrew/opt/node@22/bin`；PATH 中尚无 `tripo`、`tripo-cli`、`uv`、`uvx`。未发现 `/Applications/Blender.app`，不代表全机没有 Blender。

### Codex 如何使用 CLI

安装后先读取随包的 agent 指南，避免猜测参数：

```bash
tripo docs --llm
tripo docs --topic commands/make
tripo docs --topic examples/game-asset
```

以下为可审阅示例，**本次未执行**。先生成离线计划，检查调用步骤；真正生成时去掉 `--dry-run`。CLI 会等待完成并下载文件，`--out` 指定输出目录：

```bash
tripo make "a small alien stone relic" --model tripo-p1 --param face_limit=5000 --param texture=false --param pbr=false --out ./tripo-out --dry-run --json
```

这份配置只生成静态几何，后续格式转换另计操作。不要直接套用 `--for game-mobile` 以为只生成一次：该预设可能自动串联贴图和导出等多项收费任务。0.7.0 包中 `skill/commands/make.md` 说明 `--dry-run` 不上传、不提交；真实命令在非交互环境默认跳过交互确认。使用时应先决定资产数量与预算，再执行生成。CLI 输出可包含 `credits_consumed` 与流水线 `credits_breakdown`；`tripo balance` / `tripo usage` 可查余额与用量。[官方发布包](https://registry.npmjs.org/tripo-cli/-/tripo-cli-0.7.0.tgz)

## 可选：把同一个 CLI 注册成 MCP

已静态核对 0.7.0 的 `package.json`：`tripo` 入口为 `dist/cli.js`，要求 Node >=20。`dist/commands/mcp.js` 实现 STDIO JSON-RPC，注册 `tripo_make`、`tripo_task_get`、`tripo_task_wait`、`tripo_balance`、`tripo_history`。因此不需要额外第三方 MCP 包。[官方 npm 元数据](https://registry.npmjs.org/tripo-cli/latest)、[发布包](https://registry.npmjs.org/tripo-cli/-/tripo-cli-0.7.0.tgz)

完成 CLI 安装和登录后，可用：

```bash
codex mcp add tripo -- tripo mcp
codex mcp list
```

等价配置示意（应替换为 `command -v tripo` 返回的实际绝对路径）：

```toml
[mcp_servers.tripo]
command = "/absolute/path/to/tripo"
args = ["mcp"]
tool_timeout_sec = 1800
```

1800 秒是为阻塞生成预留的配置建议，不是端到端实测结论。`tripo_make` 可能生成数分钟；也可直接使用 CLI，减少 MCP 超时层。已登录的本地 profile 可供 CLI 读取；无需为此把 key 明文写入仓库。若使用 `TRIPO_API_KEY` 环境变量，它优先于 profile；桌面应用不应假设继承另一个终端的 `export`。[CLI 文档](https://developers.tripo3d.ai/en/docs/cli)、[Codex MCP 文档](https://developers.openai.com/codex/mcp)

Codex 支持 STDIO 的 command/args/env/env_vars/cwd；全局配置为 `~/.codex/config.toml`，可信项目也可使用 `.codex/config.toml`。主 agent 已核实本机 Codex CLI 0.154.0 有 `mcp add` / `mcp list` 命令。本次未更改配置，也未测试实际 MCP 握手。[Codex MCP 文档](https://learn.chatgpt.com/docs/extend/mcp?surface=cli)

## 费用边界

- 安装插件、安装 CLI 和登录不收生成费用；实际生成与处理消耗 **Tripo API credits**。[插件教程](https://developers.tripo3d.ai/en/docs/codex-plugin)
- Studio 和 API 使用独立积分与计费体系，Studio 月订阅积分不能直接用于 API。[官方说明](https://www.tripo3d.ai/help/api-plugins/tripo-studiotripo-api)
- 当前 API 标价为 **100 credits = US$1**。H 系列文本生成无贴图/标准贴图分别 10/20 credits，图片生成分别 20/30 credits；基础格式转换 5 credits，进阶转换 10 credits。模型、贴图、重拓扑、绑定等选择会改变成本，不能把这一基础价套给所有 CLI 自动流程。[当前价格表](https://developers.tripo3d.ai/en/pricing)
- 新账号有免费额度；本次未登录，未确认用户可领额度或有效期。旧版 OpenAPI 文档写有“300 credits / 2 weeks”，不能当成每月持续赠送。[旧版定价](https://docs.tripo3d.ai/get-started/pricing.html)
- 当前 CLI 官网另有活动：付费 Studio 的个人 Pro/Max 用户可向匹配账号一次领取 1,000 API credits，团队按规则分配；这仍是额外 API 积分，不是 Studio 积分互通。活动可变，资格需登录确认。[活动与规则](https://studio.tripo3d.ai/cli)
- API 创建任务先冻结积分，成功扣除，失败或取消退回。[当前计费规则](https://developers.tripo3d.ai/en/docs/billing)

## 旧 Python MCP 与 Blender 方案

[VAST-AI-Research/tripo-mcp](https://github.com/VAST-AI-Research/tripo-mcp) 自称官方，但 README 明确是 alpha，依赖 Python 3.10+、Blender 和 Tripo API Blender Addon。启动命令是 `uvx tripo-mcp`；`pyproject.toml` 的脚本入口为 `server:main`，源码以 STDIO 运行。[包配置](https://raw.githubusercontent.com/VAST-AI-Research/tripo-mcp/main/pyproject.toml)

源码连接 `localhost:9876`，用 addon 的 `get_tripo_apikey` 获取 API key；直接读取 `TRIPO_API_KEY` 的代码已注释。因此不能对这个旧包只设置环境变量就声称配置完成。它支持文本/图片生成、查任务、导入 GLB 和 Blender 场景操作。现有 CLI 更适合不依赖 Blender 的生成下载流程。[server.py](https://raw.githubusercontent.com/VAST-AI-Research/tripo-mcp/main/src/server.py)

还需区分 **API Blender 插件** 与新 **Studio DCC Bridge**：后者用于把 Studio 已生成模型送进 Blender，需要相应 Studio 付费计划，不能据此推断它支持旧 MCP 的 socket 协议。[官方区分说明](https://www.tripo3d.ai/integrations/blender)

## 接入 Silic2 的实际边界

Tripo 常见模型产物为 GLB；官方转换支持 GLTF、FBX、OBJ、STL、USDZ、3MF 等，骨骼与纹理限制取决于格式。[转换文档](https://docs.tripo3d.ai/export/conversion.html)

当前 Silic2 的静态网格合同是 Wavefront OBJ，支持三角形和严格凸平面四边形，建议三角化；忽略 MTL，不支持骨骼动画或 MTL 纹理，颜色/材质取自地图实例。加载器每个模型最多 100,000 三角形。以上由主 agent 对 [空间合同](crystal-spatial-contract.md) 和 `src/engine/mesh_asset.cpp` 只读核实。

因此首个接入实验建议为：单个静态道具 → 小面数几何 → 三角化 OBJ → 地图实例材质与独立碰撞代理。生成出来的 GLB/PBR/骨骼不能宣称原样就能进入当前游戏；完整材质与动画需要另立实现任务。

## 验证范围与未知项

官方网页与 npm 发布包已核实；源码仓库链接由包元数据提供，但 GitHub raw 地址未能取得，因此 CLI 代码结论来自已发布 JS 文件，而非假定的主分支源码。发布包只在内存读取，未安装或执行。

尚未验证：用户账号站点、余额、活动资格；当前目录能否显示插件；CLI 登录与网络；MCP 实际握手；真实生成品质、耗时和最终扣费。研究结果支持下一步配置，不能替代端到端试生成验收。
