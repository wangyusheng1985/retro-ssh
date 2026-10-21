## Context

当前 RetroSSH 客户端存在两项与日常操作体验直接相关的技术缺失：
1. **macOS 打包未关联应用图标**：CMake 构建生成的 macOS Bundle 缺少 `Contents/Resources/*.icns` 资源及 `MACOSX_BUNDLE_ICON_FILE` 属性配置，在 macOS 桌面环境下表现为白板通用可执行文件图标。
2. **终端未支持全屏 TUI 鼠标事件通信与备用屏幕**：现代命令行全屏工具（如 Pi Agent 的 Fullscreen 交互模式、Vim 等）依赖 ANSI 转义序列切换到 Alternate Screen Buffer，并通过 DECSET 1000/1006 捕获鼠标事件以在应用内部重绘视口。当前 `TerminalWidget` 仅具备基础字符解析与本地视图滚动逻辑，未拦截 DECSET 私有模式，并在 `wheelEvent` 中无差别地操作本地滚动条偏移，导致 Pi Agent 在默认模式下无法上下滚动。

## Goals / Non-Goals

**Goals:**
- 生成标准的 macOS `.icns` 图标并在 CMake 构建阶段自动完成打包与属性声明。
- 在 `TerminalWidget` 中增加 DECSET / DECRST 私有模式的解析支持，跟踪以下状态：
  - `m_mouseTracking` (DECSET 1000 / 1002)
  - `m_sgrMouseMode` (DECSET 1006)
  - `m_altScreenActive` (DECSET 1049)
- 在 `TerminalWidget::wheelEvent` 中实现按状态动态分流：
  1. 激活 SGR 鼠标跟踪时：发送 ANSI SGR 鼠标滚轮报告（`\e[<64;C;RM` / `\e[<65;C;RM`）至 PTY；
  2. 仅激活备用屏幕但未激活鼠标跟踪时：发送光标上下按键（`\e[A` / `\e[B`）作为滚动兜底；
  3. 普通终端状态时：保持原有的本地 `scrollUp` / `scrollDown` 行为。

**Non-Goals:**
- 实现完整的任意像素级精密鼠标拖拽框选协议与所有的 X10 传统协议变体（仅支持现代全屏应用标配的 SGR 1006 格式）。
- 替换现有终端核心为庞大的第三方虚拟终端引擎（如 libvterm），保持当前精简、易维护的轻量级架构。

## Decisions

### 1. 使用 macOS 原生 `sips` 与 `iconutil` 生成 `.icns`
- **方案**：将现有的 `src/resources/icons/app_icon.svg` 渲染为 1024x1024 高分辨率 PNG，并生成 `RetroSSH.iconset` 多规格图标包（16, 32, 64, 128, 256, 512, 1024 及 @2x 视网膜规格），再调用 macOS 内置命令行 `iconutil -c icns` 生成二进制 `RetroSSH.icns`，纳管入版本库。
- **替代方案**：运行时由 CMake 自行调用外部工具生成。
- **理由**：直接预先生成合规 `.icns` 文件并纳入资源目录，能够避免跨平台构建环境（如 Linux 交叉编译或 CI 构建机器未安装 macOS 图标工具）时的外部命令依赖。

### 2. CMake 声明 macOS Bundle 图标与资源分发
- **方案**：在 `CMakeLists.txt` 中将 `RetroSSH.icns` 加入可执行文件源文件列表，并设置其属性为 `PROPERTIES MACOSX_PACKAGE_LOCATION Resources`；同时设置 target 属性 `MACOSX_BUNDLE_ICON_FILE "RetroSSH.icns"`。
- **理由**：这是 CMake 官方推荐的 macOS App 图标打包标准实践，能自动生成符合 Apple 规范的 `Info.plist` 并拷贝资源。

### 3. SGR 1006 鼠标编码与状态分发
- **协议格式**：
  - 滚轮上滚（Button 64）：`\x1b[<64;<col>;<row>M`
  - 滚轮下滚（Button 65）：`\x1b[<65;<col>;<row>M`
  - 坐标基准：1-based（列从 1 开始，行从 1 开始）。
- **理由**：SGR 1006 是目前 Linux/macOS CLI 工具（包括 Node.js / Rust / Go 等现代 TUI 框架）的标准鼠标协议，能支持超过 223 列的大终端尺寸，且编码清晰鲁棒。

## Risks / Trade-offs

- **[Risk] 滚轮灵敏度差异**：macOS 触控板会发送高频率的微小 `pixelDelta`，如果每次像素移动都上报一个按键/滚轮事件，可能导致 Pi Agent 翻页过快或闪烁。
  - **Mitigation**：沿用已有的 `m_wheelAccumulator` 行高累积缓冲逻辑，仅在累积达到单行行高步长时触发一次协议编码并清空步长，保证触控板手势与普通鼠标滚轮手感均自然平滑。
