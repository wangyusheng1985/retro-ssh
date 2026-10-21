## Why

当前编译生成的 macOS RetroSSH.app 缺少原生应用图标，在 Dock、访达（Finder）及聚焦搜索（Spotlight）中显示为通用默认可执行文件图标；同时终端模拟器尚未支持 ANSI 鼠标跟踪协议（DECSET 1000/1002/1006）及备用屏幕缓冲区（Alternate Screen DECSET 1049），导致类似 Pi Agent、Vim、Htop 等全屏 TUI 程序在默认 Fullscreen 模式下无法使用鼠标滚轮上下浏览内容。本变更有针对性地补齐 macOS 原生应用图标打包以及终端对全屏 TUI 鼠标滚轮与备用屏幕交互的支持。

## What Changes

- **macOS App 原生图标支持**：基于 `app_icon.svg` 生成多分辨率 Apple `.icns` 格式图标（包含 16x16 到 1024x1024），并在 CMake 中将其打包至 `Contents/Resources`，同时配置 `MACOSX_BUNDLE_ICON_FILE`。
- **ANSI DECSET/DECRST 私有控制序列解析**：在 `TerminalWidget` 中增加对备用屏幕缓冲区（`\e[?1049h` / `\e[?1049l`）和鼠标上报模式（`\e[?1000h`、`\e[?1002h`、`\e[?1006h` 及其关闭命令）的解析与状态跟踪。
- **全屏 TUI 鼠标滚轮事件协议上报**：在 `TerminalWidget::wheelEvent` 中，当检测到终端已开启 SGR 1006 鼠标跟踪时，将鼠标滚轮动作转换为标准 SGR 鼠标控制序列（向上 `\e[<64;col;rowM`、向下 `\e[<65;col;rowM`）上报给 PTY 通道，使 Pi Agent 等工具在 Fullscreen 模式下能够流畅滚动。
- **Alternate Screen 备用滚轮兜底（Alternate Scroll）**：当远程全屏程序仅进入备用屏幕但未启用鼠标跟踪时，滚轮操作自动转换为发送向上/向下箭头按键序列（`\eOA` / `\eOB` 或 `\e[A` / `\e[B`），保证传统 curses 程序的滚动兼容性。

## Capabilities

### New Capabilities
- `terminal-mouse-and-alt-screen`: 支持 DECSET 1000/1002/1006 鼠标捕获协议与 1049 备用屏幕缓冲区，实现全屏 TUI 程序下的鼠标滚轮双向通信与兼容回退。
- `app-packaging`: 提供平台原生图标资源及打包配置，支持 macOS App Bundle 图标与资源自动部署。

### Modified Capabilities
<!-- 本项目暂无已归档至主规格库 openspec/specs/ 的存量 specs -->

## Impact

- **代码影响**：
  - `CMakeLists.txt`：增加图标资源目标配置及 `MACOSX_BUNDLE_ICON_FILE`。
  - `src/ui/TerminalWidget.h` / `src/ui/TerminalWidget.cpp`：增加鼠标跟踪状态位、备用屏幕状态位，扩展 ANSI 转义序列解析器，修改 `wheelEvent` 交互分支。
  - `src/resources/icons/`：新增 `RetroSSH.icns` 图标集文件。
- **用户体验**：
  - macOS 桌面与 Dock 拥有经典统一的应用程序图标。
  - Pi Agent（默认 fullscreen 模式）无需切换为 regular 模式即可流畅滚轮浏览问答历史。
