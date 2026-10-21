## ADDED Requirements

### Requirement: 24-bit TrueColor SGR Parsing and Rendering
The terminal emulator SHALL parse standard 24-bit TrueColor escape sequences `\e[38;2;R;G;Bm` for foreground colors and `\e[48;2;R;G;Bm` for background colors, storing the exact RGB color values into the character cell and rendering them faithfully.

#### Scenario: Parse 24-bit foreground TrueColor
- **WHEN** the terminal receives `\e[38;2;120;180;240m`
- **THEN** the active foreground color is set to RGB(120, 180, 240) for subsequent text output

#### Scenario: Parse 24-bit background TrueColor
- **WHEN** the terminal receives `\e[48;2;30;40;50m`
- **THEN** the active background color is set to RGB(30, 40, 50) for subsequent text output

### Requirement: 256-Color Palette SGR Parsing
The terminal emulator SHALL parse extended 256-color escape sequences `\e[38;5;Nm` for foreground and `\e[48;5;Nm` for background, converting index `N` (0-255) to the standard xterm 256-color palette (including the 6x6x6 color cube and 24 grayscale steps).

#### Scenario: Parse 256-color foreground index
- **WHEN** the terminal receives `\e[38;5;196m`
- **THEN** the active foreground color is set to the color at index 196 in the standard 256-color table

#### Scenario: Parse 256-color background index
- **WHEN** the terminal receives `\e[48;5;236m`
- **THEN** the active background color is set to the color at index 236 in the standard 256-color table

### Requirement: High-Intensity Bright Colors Support
The terminal emulator SHALL parse ANSI high-intensity codes `90` through `97` for bright foreground colors and `100` through `107` for bright background colors.

#### Scenario: Parse bright foreground color
- **WHEN** the terminal receives `\e[90m`
- **THEN** the active foreground color is set to bright black (dark gray)

#### Scenario: Parse bright background color
- **WHEN** the terminal receives `\e[104m`
- **THEN** the active background color is set to bright blue

### Requirement: Dynamic Profile Color Reset
When receiving reset sequence `\e[0m`, `\e[39m` (default foreground), or `\e[49m` (default background), the terminal SHALL restore the active colors to the user's currently configured terminal colors (`m_defaultFg` / `m_defaultBg`) rather than hardcoded monochrome.

#### Scenario: SGR reset attributes
- **WHEN** the terminal receives `\e[0m`
- **THEN** the active foreground color resets to `m_defaultFg` and background resets to `m_defaultBg`

### Requirement: OSC 10 and OSC 11 Color Query Response
The terminal SHALL respond to OSC 10 (`\e]10;?\a`) and OSC 11 (`\e]11;?\a`) queries by returning the current terminal foreground and background colors in `rgb:rrrr/gggg/bbbb` format back to the SSH channel.

#### Scenario: Query background color via OSC 11
- **WHEN** the remote application sends `\e]11;?\a` or `\e]11;?\e\\`
- **THEN** the terminal responds with `\e]11;rgb:<r>/<g>/<b>\a` matching the terminal default background color
