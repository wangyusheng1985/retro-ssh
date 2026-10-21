## 1. macOS Bundle Icon Packaging

- [x] 1.1 基于 `src/resources/icons/app_icon.svg` 生成多分辨率 `src/resources/icons/RetroSSH.icns` 图标集文件
- [x] 1.2 在 `CMakeLists.txt` 中添加 `RetroSSH.icns` 为构建资源，配置 `MACOSX_PACKAGE_LOCATION Resources` 与 `MACOSX_BUNDLE_ICON_FILE`
- [x] 1.3 编译验证 macOS App Bundle 内的 `Info.plist` 与 `Contents/Resources/RetroSSH.icns`

## 2. ANSI DECSET / DECRST 私有序列支持

- [x] 2.1 在 `TerminalWidget.h` 中声明鼠标跟踪与备用屏幕状态成员变量及重置逻辑
- [x] 2.2 在 `TerminalWidget.cpp` 的 `handleEscapeSequence` 中实现对 DECSET/DECRST 序列（1000, 1002, 1006, 1049）的解析
- [x] 2.3 在 `resetTerminal` 中复位鼠标跟踪与备用屏幕状态

## 3. 鼠标滚轮分流与 TUI 滚动支持

- [x] 3.1 在 `TerminalWidget::wheelEvent` 中实现 SGR 1006 模式下的鼠标滚轮事件编码及 `dataToSend` 发送
- [x] 3.2 在 `TerminalWidget::wheelEvent` 中实现备用屏幕无鼠标跟踪模式下的方向键（Up/Down）兜底发送
- [x] 3.3 保证非全屏/普通命令行状态下本地 `scrollUp` / `scrollDown` 行为平滑无回归

## 4. 验证与构建测试

- [x] 4.1 执行 CMake 构建并核验 RetroSSH.app 在 macOS 下的图标展示
- [x] 4.2 启动 RetroSSH 并验证在默认 Fullscreen 模式下 Pi Agent 与 Vim 的滚轮上下翻页体验
