## Context

当前 RetroSSH 的终端颜色支持停留在基础的 8 色 ANSI 阶段。当用户在远程连接中运行 Pi Agent、htop、bat、git diff、Neovim 等现代命令行与 TUI 交互程序时，由于 RetroSSH 缺少 24-bit TrueColor（`38;2;r;g;b`）与 256 色（`38;5;n`）解析，且请求的 PTY 类型为旧版 `"xterm"`，导致丰富的背景色块、语法高亮和渐变色全部丢失。此外，Pi Agent 在启动时依赖 OSC 10/11 探测宿主明暗背景以自适应对比度，终端未能应答导致其退化为保守的无背景模式。

## Goals / Non-Goals

**Goals:**
- 将 SSH PTY 终端类型升级为 `"xterm-256color"`。
- 在 `TerminalWidget` 中重构 SGR (`'m'`) 解析机制，采用游标式（Cursor-based）参数遍历，完整支持：
  - 24-bit TrueColor (`38;2;R;G;B` 与 `48;2;R;G;B`)
  - 256 色调色板索引 (`38;5;N` 与 `48;5;N`)
  - 标准 Bright 16 色（90~97，100~107）
  - 属性重置（`0`、`39`、`49`）准确恢复到当前 Profile 设定的前景色与背景色
- 支持标准 xterm 256 色生成算法（0~15 标准色，16~231 6x6x6 RGB 立方体，232~255 24 阶灰度级）。
- 拦截并响应 OSC 10（前景色）与 OSC 11（背景色）查询，以 `\e]11;rgb:rr/gg/bb\a` 格式回复当前底色。

**Non-Goals:**
- 不引入重型第三方虚拟终端库，保持在轻量级 `TerminalWidget` 内部实现原生 Qt 高性能渲染。
- 不影响现有的经典 Win32 视觉基调与用户自定义配色方案。

## Decisions

### 1. 采用游标步进式 SGR 解析替代简单的单个元素遍历
- **背景**：原有逻辑使用 `for (const QString &p : parts)`，无法处理具有变长连续参数的复合指令（如 `38;2;r;g;b` 占 5 个分号节，`38;5;n` 占 3 个分号节）。
- **设计**：采用游标循环：
  ```cpp
  for (int i = 0; i < parts.size(); ++i) {
      int val = parts[i].toInt();
      if ((val == 38 || val == 48) && i + 1 < parts.size()) {
          int subType = parts[i + 1].toInt();
          if (subType == 2 && i + 4 < parts.size()) { // 24-bit TrueColor
              int r = parts[i + 2].toInt();
              int g = parts[i + 3].toInt();
              int b = parts[i + 4].toInt();
              QColor color = QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
              if (val == 38) m_currentFg = color; else m_currentBg = color;
              i += 4;
          } else if (subType == 5 && i + 2 < parts.size()) { // 256-color
              int idx = parts[i + 2].toInt();
              QColor color = colorFrom256Index(idx);
              if (val == 38) m_currentFg = color; else m_currentBg = color;
              i += 2;
          }
      } ...
  }
  ```
- **收益**：完全向下兼容标准 ANSI，同时精确处理多参数色彩指令。

### 2. 标准 256 色算法静态映射
- 索引 0~15：使用扩展的 16 色标准高对比度 ANSI 调色板。
- 索引 16~231：通过 `16 + 36*r + 6*g + b` 算法快速计算出 `(r ? 55 + r*40 : 0)` 的 6 级台阶值。
- 索引 232~255：通过 `8 + (idx - 232)*10` 计算 24 级平滑灰度。
- **收益**：无需庞大的外部数据表，内存占用为 0，计算效率极高。

### 3. OSC 10 / OSC 11 颜色反馈
- 当收到 `\e]10;?` 或 `\e]11;?` 时，读取当前有效的前景色或背景色（考虑 `m_reverseVideo` 与 Profile），格式化为 `\x1b]%1;rgb:%2/%3/%4\x07` 并通过 `dataToSend` 发回 PTY，让 Pi Agent 等 TUI 能立即得到宿主底色反馈。

## Risks / Trade-offs

- **[Risk] 背景色与终端底色融合问题**：某些应用发送了与终端默认白底/黑底对比度不足的颜色。
  - **Mitigation**：正确响应 OSC 11 背景色查询后，Pi Agent 的主题算法会自动针对白底或黑底调整文字与面板的 lightness，保障可读性。
