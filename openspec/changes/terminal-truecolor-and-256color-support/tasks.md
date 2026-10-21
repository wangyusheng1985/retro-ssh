## 1. PTY 与基础调色板升级

- [x] 1.1 在 `SshSessionWorker.cpp` 中将 PTY 请求终端类型由 `xterm` 升级为 `xterm-256color`
- [x] 1.2 在 `TerminalWidget` 中扩充 ANSI 16 色调色板并实现 xterm 256 色静态计算转换函数 `colorFrom256Index(int idx)`

## 2. SGR 色彩序列解析重构

- [x] 2.1 重构 `TerminalWidget.cpp` 中 SGR `m` 指令解析为基于游标的多参数步进处理循环
- [x] 2.2 实现 24-bit TrueColor（`38;2;R;G;B` 与 `48;2;R;G;B`）前景色与背景色解析
- [x] 2.3 实现 256 色扩展索引（`38;5;N` 与 `48;5;N`）前景色与背景色解析
- [x] 2.4 实现 ANSI Bright 16 高亮色彩（`90~97` 前景与 `100~107` 背景）解析
- [x] 2.5 修复颜色重置指令（`0`、`39`、`49`），使用 Profile 的 `m_defaultFg` / `m_defaultBg` 恢复默认色彩

## 3. OSC 调色板自适应应答

- [x] 3.1 在 `TerminalWidget.cpp` 的 OSC 序列解析中支持 OSC 10 与 OSC 11 查询，格式化回传当前实际底色与前景色

## 4. 编译与效果验证

- [x] 4.1 执行 CMake 构建并排查消除任何编译警告与错误
- [x] 4.2 在 RetroSSH 中启动并验证 Pi Agent 交互界面中的主题面板、高亮字体及背景色彩呈现
