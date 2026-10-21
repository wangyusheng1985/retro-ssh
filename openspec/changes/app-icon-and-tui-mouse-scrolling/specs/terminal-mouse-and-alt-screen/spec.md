## ADDED Requirements

### Requirement: ANSI DECSET Private Mode Parsing
The terminal emulator SHALL parse DECSET (`\e[?<n>h`) and DECRST (`\e[?<n>l`) sequences for mouse tracking and alternate screen buffer management, specifically supporting mode 1000 (VT200 normal mouse), mode 1002 (button-event tracking), mode 1006 (SGR extended mouse coordinates), and mode 1049 (alternate screen buffer).

#### Scenario: Enable SGR mouse tracking
- **WHEN** the remote application sends `\e[?1000h\e[?1006h`
- **THEN** the terminal marks mouse tracking active and SGR coordinate reporting format enabled

#### Scenario: Disable mouse tracking
- **WHEN** the remote application sends `\e[?1000l` or `\e[?1006l`
- **THEN** the terminal marks mouse tracking inactive and resumes native local text selection and scrolling

#### Scenario: Switch to alternate screen
- **WHEN** the remote application sends `\e[?1049h`
- **THEN** the terminal records entry into the alternate screen buffer

#### Scenario: Leave alternate screen
- **WHEN** the remote application sends `\e[?1049l`
- **THEN** the terminal restores the primary screen buffer and disables alternate screen tracking

### Requirement: Mouse Wheel Protocol Reporting in Fullscreen TUI Mode
When SGR 1006 mouse tracking mode is enabled by the remote application, the terminal SHALL encode mouse wheel scroll events into SGR mouse reports and transmit them directly to the SSH channel rather than scrolling the local scrollback buffer.

#### Scenario: Scroll up in mouse tracking mode
- **WHEN** user scrolls up on the terminal while mode 1006 is active at column `C` and row `R`
- **THEN** the terminal transmits `\e[<64;C;RM` to the remote backend and does not change the local buffer scroll offset

#### Scenario: Scroll down in mouse tracking mode
- **WHEN** user scrolls down on the terminal while mode 1006 is active at column `C` and row `R`
- **THEN** the terminal transmits `\e[<65;C;RM` to the remote backend and does not change the local buffer scroll offset

### Requirement: Alternate Screen Wheel Fallback
When the terminal is in alternate screen mode (`1049`) but mouse tracking is not active, the terminal SHALL translate mouse wheel events into up/down arrow key sequences to allow scrolling in traditional fullscreen curses tools.

#### Scenario: Scroll up in alternate screen without mouse tracking
- **WHEN** user scrolls up while alternate screen mode is active but mouse tracking is inactive
- **THEN** the terminal transmits the up arrow escape sequence `\e[A` for each scroll step

#### Scenario: Scroll down in alternate screen without mouse tracking
- **WHEN** user scrolls down while alternate screen mode is active but mouse tracking is inactive
- **THEN** the terminal transmits the down arrow escape sequence `\e[B` for each scroll step
