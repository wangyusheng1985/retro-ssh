## Why

当前 RetroSSH 终端仅支持最基础的标准 8 色 ANSI 颜色，未实现现代终端标配的 24-bit RGB TrueColor（真彩色）以及 256 色扩展调色板，且向远程服务器申请的 PTY 类型为老旧的 `xterm`。这导致 Pi Agent、htop、bat、git diff、vim 等现代命令行与 TUI 界面中的富文本颜色、状态栏背景、气泡色彩及语法高亮完全丢失，退化为单调甚至不可见的黑白文本。本变更旨在全面升级终端色彩解析与协商引擎，使 RetroSSH 具备现代终端完整生动的色彩表现力。

## What Changes

- **24-bit TrueColor（真彩色）支持**：在 `TerminalWidget` 的 SGR 序列（`\e[...m`）中增加对 ISO-8613-3 / xterm 标准真彩色（`38;2;R;G;B` 前景色与 `48;2;R;G;B` 背景色）的参数游标解析与渲染。
- **256 色扩展调色板支持**：增加标准 256 色查找表（包含标准 16 色、6x6x6 RGB 颜色立方体及 24 阶灰度级），完整支持 `38;5;N` 与 `48;5;N`。
- **Bright 16 高亮颜色支持**：支持 `90~97`（高亮前景色）及 `100~107`（高亮背景色）解析。
- **色彩重置保真（SGR 0/39/49）**：在收到 `\e[0m`、`\e[39m`、`\e[49m` 时，准确重置回用户在 Profile Settings 中配置的终端默认前景/背景色，而非强行硬编码为黑白。
- **PTY 终端环境升级**：在 SSH 会话初始化中将申请的 PTY 类型由 `xterm` 升级为 `xterm-256color`，向远程系统明确声明 256 色与真彩色能力。
- **OSC 10 / OSC 11 颜色探测应答**：拦截远程程序查询终端默认前景（`\e]10;?\a`）与背景色（`\e]11;?\a`）的控制序列，并以标准 RGB 格式回复当前终端底色，使 Pi Agent 自适应主题能够精准匹配明暗对比度。

## Capabilities

### New Capabilities
- `terminal-color-engine`: 终端色彩引擎，提供 24-bit TrueColor、256 色立方体、Bright 16 色、Profile 默认底色重置与 OSC 10/11 调色板查询应答。
- `ssh-pty-environment`: 将 SSH PTY 请求类型提升为 `xterm-256color`，确保现代 CLI / TUI 工具正确识别终端色彩能力。

### Modified Capabilities
<!-- 本项目暂无已归档至主规格库 openspec/specs/ 的存量 specs -->

## Impact

- **代码影响**：
  - `src/ui/TerminalWidget.h` / `src/ui/TerminalWidget.cpp`：扩展 `ANSI_COLORS` 为 16 色调色板，新增 256 色颜色映射函数，重构 SGR `m` 序列解析逻辑支持变长多参数（TrueColor / 256色），在 OSC 解析中响应 OSC 10/11 颜色查询。
  - `src/backend/SshSessionWorker.cpp`：将 PTY 请求类型由 `"xterm"` 更改为 `"xterm-256color"`。
- **用户体验**：
  - Pi Agent 默认 Fullscreen 交互模式下的问答、状态条、语法高亮与背景面板完整呈现。
  - 现代 Linux/macOS 终端开发环境（彩色 Git、Zsh/Fish 语法高亮、Neovim 等）色彩完全一致。
