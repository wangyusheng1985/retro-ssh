# RetroSSH

<p align="center">
  <img src="src/resources/icons/app_icon.png" width="128" height="128" alt="RetroSSH Logo" />
</p>

<p align="center">
  <strong>1:1 像素级复刻经典 SSH Secure Shell Client (v3.2.9) 交互与质感</strong><br>
  <strong>融合现代 SSH 协议内核与跨平台桌面体验</strong>
</p>

---

## 📖 项目简介

经典的 **SSH Secure Shell Client (v3.2.9)** 曾陪伴无数老运维和系统工程师度过了千禧年代。其标志性的独立双栏 SFTP 文件管理器、Win32 经典 3D 凸起浮雕按钮以及迅捷稳定的操作习惯，至今仍被许多工程师奉为经典。

然而原版软件自 2003 年停更至今已超 20 年，不仅无法在现代操作系统（如 macOS、现代化 Linux）上原生运行，而且仅支持已被弃用的旧式加密套件，连接现代 Linux 服务器时频繁报错 `Algorithm negotiation failed`。

**RetroSSH** 基于现代 **C++17**、**Qt 5 / Qt 6** 与 **libssh2 + OpenSSL 3.x** 构建，旨在跨平台原生环境中完整复刻千禧时代的经典工作流，同时具备现代安全标准与现代化终端能力。

---

## ✨ 核心特性

- 🖥 **经典 Win32 交互质感**
  - 1:1 还原经典菜单栏、3D 凸起按钮、经典工具栏（Quick Connect、Connect、Disconnect、Settings、经典黄色 SFTP 文件夹图标）及状态栏。
  - 复刻经典 **Quick Connect**（快速连接）弹窗与多层树形 **Profile Settings**（会话属性配置）对话框。

- 📂 **独立弹出的双栏 SFTP 文件管理器**
  - 点击主窗口工具栏的黄色 SFTP 图标，即可弹出完全独立的 SFTP 窗口。
  - 左侧本地文件系统树与文件列表，右侧远程服务器文件系统，支持双击直接上传/下载、左右拖拽传输与属性查看。

- 🔒 **现代化 SSH2 安全通信**
  - 底层基于 **libssh2** 与 **OpenSSL 3.x**，原生支持现代前沿加密套件：
    - 密钥算法：`ed25519`、`ecdsa-sha2-*`、`rsa-sha2-256/512`
    - 密钥交换：`curve25519-sha256`、`diffie-hellman-group-exchange-sha256`
    - 对称加密：`chacha20-poly1305`、`aes256-gcm`、`aes128-ctr` 等
  - 彻底告别连接现代 Linux/FreeBSD 主机时的算法不匹配问题。

- 🎨 **24-bit TrueColor（真彩色）与 256 色色板**
  - 支持 ISO-8613-3 / xterm 标准的 24 位真彩色（`\e[38;2;R;G;Bm` 与 `\e[48;2;R;G;Bm`）。
  - 内置 6x6x6 RGB 色彩立方体与 24 阶灰度级 256 色色板，完美支持现代命令行工具（`pi agent`、`htop`、`bat`、`git diff`、`neovim` 等）的主题高亮与面板底色。
  - 支持 **OSC 10 / OSC 11** 调色板查询应答，现代 TUI 工具可自动识别宿主明暗底色并自适应调节对比度。

- 🖱 **全屏 TUI 滚轮与鼠标跟踪支持**
  - 完整解析 DECSET 1000/1002/1006 鼠标跟踪协议与 1049 备用屏幕缓冲区（Alternate Screen Buffer）。
  - 在全屏 TUI 程序（如 Pi Coding Agent 的 Fullscreen 模式、Vim 等）中，滚轮事件自动转化为 SGR 鼠标序列上报给服务端视口，享受如同原生终端的顺滑滚动手感。

- 🌐 **原生 UTF-8 中文与多编码支持**
  - 原生支持 UTF-8 编码，消除中文乱码，同时支持 GBK / GB18030 等历史编码切换。

- 🍎 **macOS 原生体验**
  - 生成标准 Apple `.icns` 图标集，完美适配 macOS Dock、访达（Finder）与聚焦搜索（Spotlight）。

---

## 🛠 编译与构建

### 1. 安装构建依赖

#### macOS (Homebrew)
```bash
brew install cmake qt@5 libssh2 openssl@3
# 或者使用 Qt 6:
# brew install qt
```

#### Ubuntu / Debian
```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake qtbase5-dev libssh2-1-dev libssl-dev
```

### 2. 编译项目

```bash
# 克隆仓库
git clone https://github.com/wangyusheng1985/retro-ssh.git
cd retro-ssh

# 创建并进入构建目录
mkdir -p build && cd build

# 执行 CMake 配置并编译
cmake ..
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)
```

构建完成后：
- **macOS**：在 `build/` 目录下生成 `RetroSSH.app`，直接双击或执行 `open RetroSSH.app` 即可启动。
- **Linux**：在 `build/` 目录下生成可执行二进制文件 `./RetroSSH`。

---

## 📂 项目架构

```text
retro-ssh/
├── CMakeLists.txt          # 跨平台 CMake 构建定义与 macOS Bundle 打包配置
├── src/
│   ├── main.cpp            # 程序入口与全局事件循环
│   ├── backend/            # 后端通信引擎
│   │   ├── SshSessionWorker.*  # SSH 交互通道、PTY 请求与数据泵
│   │   ├── SftpWorker.*        # 异步 SFTP 传输与文件系统遍历
│   │   └── ProfileManager.h    # 会话配置与历史连接记录管理
│   ├── ui/                 # 界面呈现层
│   │   ├── MainWindow.*        # 经典风格主终端窗口与工具栏
│   │   ├── TerminalWidget.*    # 核心虚拟终端仿真、SGR 色彩与鼠标滚轮分流
│   │   ├── SftpWindow.*        # 标志性的独立双栏 SFTP 文件传输窗口
│   │   ├── QuickConnectDialog.*# 经典快速连接弹窗
│   │   └── ProfileSettingsDialog.* # 会话属性与认证管理对话框
│   └── resources/          # 经典样式表与图标资源
│       ├── classic_win32.qss   # Win32 经典 3D 浮雕控件 QSS
│       ├── icons/              # 经典工具栏矢量图标与 RetroSSH.icns
│       └── resources.qrc       # Qt 资源集合
└── README.md
```

---

## 📄 许可证

本项目采用 MIT 许可证开源，详情参阅项目代码。
