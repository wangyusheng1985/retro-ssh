## Why

经典的 SSH Secure Shell Client (v3.2.9) 拥有极其高效的千禧时代经典 Win32 交互质感与标志性的“独立 SFTP 左右分栏文件管理器”双窗口联动体验，深受大量老运维和系统工程师的喜爱。然而该软件自 2003 年停更至今已超 20 年，不仅无法在 macOS 等现代操作系统上原生运行，而且仅支持老旧淘汰的加密算法，导致连接现代 Linux 服务器时频繁出现 `Algorithm negotiation failed`。

本项目旨在基于现代 C++ / Qt 6 与 libssh2，1:1 像素级复刻 SSH Secure Shell Client 经典界面的操作习惯与双窗联动工作流，同时集成最新的现代 SSH 安全协议与现代跨平台原生运行时支持。

## What Changes

- **全新经典风格主终端窗口**：1:1 还原经典菜单栏、带 3D 凸起效果的工具栏（Quick Connect、Connect、Disconnect、Settings、黄色文件夹 SFTP 图标）及状态栏。
- **现代化 SSH2 协议内核**：采用 libssh2 + OpenSSL 3.x 驱动，支持 Ed25519、Curve25519、AES-GCM、SHA-2 及 RSA-SHA2 现代算法，原生支持 UTF-8 中文显示。
- **独立弹出的 SFTP 双分栏文件管理器**：点击主窗口工具栏黄色图标即可弹出独立的 SFTP 管理窗口，左侧本地目录，右侧远程目录，支持鼠标左右拖拽上传下载及传输队列监控。
- **经典 Quick Connect & Settings 对话框**：复刻经典的快速连接弹窗与树形配置界面（Profile Settings -> Connection / Authentication）。
- **macOS / Linux / Windows 跨平台原生构建**：基于 CMake + Qt 6 构建原生二进制程序，告别 Wine 模拟层。

## Capabilities

### New Capabilities
- `session-management`: 管理快速连接（Quick Connect）、连接会话属性（Profile Settings）、主机认证与本地加密配置存储。
- `terminal-emulator`: 经典 UI 风格的 VT100 / ANSI 终端模拟核心，支持 PTY 会话流、快捷键映射与 UTF-8 编码。
- `sftp-dual-pane`: 独立弹出的双栏 SFTP 文件管理器，提供本地与远程文件系统树、拖拽传输与传输进度管理。
- `ssh-crypto-backend`: 基于 libssh2 与 OpenSSL 的现代底层网络协议引擎，负责 SSH2 握手、密钥交换、认证与信道复用。

### Modified Capabilities
<!-- 本项目为全新绿地项目，尚无存量 specs -->

## Impact

- **系统平台**：支持 macOS (Intel x86_64 及 Apple Silicon)、Linux (x86_64/ARM64) 和 Windows 原生桌面系统。
- **技术依赖**：引入 Qt 6 (Widgets, Core, Gui, Network)、libssh2 (>= 1.11.0)、OpenSSL (>= 3.0)。
- **构建系统**：采用现代 CMake (>= 3.20) 进行跨平台多目标编译管理。
