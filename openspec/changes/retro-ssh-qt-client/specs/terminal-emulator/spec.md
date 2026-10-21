## ADDED Requirements

### Requirement: VT100 and ANSI Terminal Emulation
The system SHALL provide an embedded terminal display widget capable of rendering VT100 and ANSI terminal escape codes, cursor movement, and line drawing.

#### Scenario: Display server terminal output
- **WHEN** the remote SSH channel sends standard output or ANSI escape sequences
- **THEN** system renders text in the configured monospace font (e.g., Courier New or Consolas) with appropriate styles and colors

#### Scenario: User input forwarding
- **WHEN** user presses keys inside the terminal widget
- **THEN** system forwards raw keystrokes and escape sequences directly through the SSH PTY channel

### Requirement: Classic Win32 Window Aesthetics
The system SHALL style the main application window with classic Windows 2000 / XP retro design, including sunken terminal borders, 3D toolbar buttons, and retro status indicators.

#### Scenario: Render classic toolbar
- **WHEN** application launches
- **THEN** system displays the classic toolbar with Quick Connect, Connect, Disconnect, Settings, and the Yellow SFTP Folder button

#### Scenario: Copy and paste shortcuts
- **WHEN** user selects text in the terminal and uses Ctrl+Insert / Ctrl+C or right-clicks
- **THEN** system copies the selection to the system clipboard
