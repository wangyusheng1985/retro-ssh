## Context

SSH Secure Shell Client (v3.2.9) 是千禧年代广泛流行的经典终端与 SFTP 客户端。其最大的特色是极具辨识度的 Win32 灰阶立体 UI、低延迟交互以及点击黄色文件夹即时拉出独立 SFTP 文件管理器的双窗口架构。

由于其停更 20 余年且协议过旧，本项目基于现代 C++ 与 Qt 6，从零重构一个 1:1 复刻其外观与核心交互的现代化跨平台开源客户端，底层使用 libssh2 + OpenSSL 3.x 提供无妥协的现代加密能力。

## Goals / Non-Goals

**Goals:**
- **1:1 视觉与交互复刻**：主界面（菜单栏、工具栏、终端凹陷边框、状态栏、Quick Connect 弹窗、Profile 属性树）严格还原 3.2.9 风格。
- **独立 SFTP 窗口联动**：实现独立的双栏文件管理窗口，支持远程文件树、拖拽上传/下载、覆盖确认与传输进度列表。
- **现代化协议支持**：支持现代 OpenSSH 默认的全部算法套件（Curve25519、Ed25519、AES-GCM、SHA-2），无感直连 modern Linux。
- **跨平台原生支持**：原生编译运行于 macOS、Linux 与 Windows，支持 Retina 高清字体渲染与 UTF-8 编码。

**Non-Goals:**
- **反编译或修改旧二进制**：纯净室（Clean-room）独立开发，不借用或反向修改原版 3.2.9 闭源二进制文件。
- **复杂插件体系**：首期不实现大型第三方脚本引擎或 X11 转发高级扩展，聚焦终端敲命令与 SFTP 传输两大核心体验。

## Decisions

### 1. 技术栈：C++20 + Qt 6 Widgets
- **选择依据**：Qt Widgets 自带成熟的窗口模型与桌面控件，配合 QStyle / QSS 能够以最低成本还原 Win32 经典灰色浮雕质感；同时原生内存占用低（通常 < 50MB），启动速度在毫秒级。
- **备选方案**：Go + Wails/Tauri（需要用 Web 技术画界面，原生感略逊于 Qt Widgets）。

### 2. 网络与安全栈：libssh2 + OpenSSL 3.x
- **选择依据**：libssh2 是 C/C++ 生态中最成熟、最稳定的轻量级 SSH2 客户端库，支持非阻塞 I/O，能直接复用底层 TCP 套接字并同时建立多个 Channel（如终端 PTY Channel 与 SFTP Channel）。
- **备选方案**：libssh（API 较丰富但体积相对较大，libssh2 配合 Qt 的多线程模型更为紧凑）。

### 3. 多线程与并发架构
```
+-------------------------------------------------------------+
|                      Qt Main GUI Thread                     |
|  [ MainWindow: Terminal ]       [ SFTPWindow: Dual-Pane ]   |
+------------------------------+------------------------------+
                               | Qt Signals / Slots
+------------------------------v------------------------------+
|                     Worker Thread Pool                      |
|  +------------------------+    +-------------------------+  |
|  | SSH Session Worker     |    | SFTP Transfer Worker    |  |
|  | - PTY read/write loop  |    | - Chunked upload/down   |  |
|  | - Keep-alive timer     |    | - Progress callbacks    |  |
|  +------------------------+    +-------------------------+  |
|                              |                              |
|                   libssh2 Session Handle                    |
+-------------------------------------------------------------+
```
- **选择依据**：主 GUI 线程绝对不执行任何阻塞性套接字 I/O。终端数据流与文件传输由专门的后台 QThread 负责，通过 Qt 的 Signals & Slots 与主线程进行安全的异步事件交互。

### 4. 经典外观引擎（Classic Retro Theme）
- **选择依据**：设计一套专用的 `classic_win32.qss` 样式表与调色板，配合标准像素级图标资源（Quick Connect、Connect、Disconnect、Yellow Folder 等），确保在 macOS 的原生窗口系统内也能完美呈现 Windows 经典 3D 浮雕与灰底效果。

## Risks / Trade-offs

- **[Risk] macOS 原生窗口修饰与经典 Win32 标题栏风格冲突**  
  → *Mitigation*: 遵循 macOS 标准窗口边框与控制按钮，但在主内容区域（菜单栏、工具栏、工作区分割线）严格保持 3.2.9 的像素级复古质感，既符合苹果系统规范又保留灵魂外观。
- **[Risk] 大文件 SFTP 传输占用带宽导致终端输入卡顿**  
  → *Mitigation*: 为 SFTP 传输与 PTY 交互分配独立的信道队列，并设定 I/O 优先级，终端按键与回显享受绝对最高优先级。
- **[Risk] 不同字符集与编码的中文乱码问题**  
  → *Mitigation*: 默认全局启用 UTF-8 解码引擎，并在会话设置中提供 GBK / GB2312 / UTF-8 手动切换选项。
