## 1. Project Scaffolding and Build Environment

- [x] 1.1 Create CMake project structure supporting Qt 6, libssh2, and OpenSSL 3.x
- [x] 1.2 Setup retro asset package including classic 3D icons, toolbar bitmaps, and fonts
- [x] 1.3 Implement classic Win32 QSS stylesheet (`classic_win32.qss`) for authentic retro appearance

## 2. SSH and Crypto Backend Core

- [x] 2.1 Implement `SshSessionWorker` wrapper around libssh2 with non-blocking event loop
- [x] 2.2 Implement modern cryptographic negotiation and host key verification
- [x] 2.3 Implement authentication handlers for Password, Public Key, and Keyboard-Interactive methods
- [x] 2.4 Implement PTY allocation, window resize handler, and raw byte stream I/O channels

## 3. Classic Main Window and Quick Connect

- [x] 3.1 Implement `MainWindow` with classic menu bar (File, Edit, View, Profiles, Window, Help)
- [x] 3.2 Implement classic 3D toolbar with Quick Connect, Connect, Disconnect, Settings, and Yellow Folder buttons
- [x] 3.3 Implement `QuickConnectDialog` modal window with host, username, and port fields
- [x] 3.4 Implement `ProfileSettingsDialog` with hierarchical settings tree and local profile storage

## 4. Terminal Emulator Component

- [x] 4.1 Implement `TerminalWidget` with VT100/ANSI escape sequence parser and screen grid buffer
- [x] 4.2 Support keyboard input forwarding (including arrows, function keys, and control sequences)
- [x] 4.3 Support clipboard operations (Ctrl+Insert/Ctrl+C copy, Shift+Insert/Ctrl+V paste, right-click menu)
- [x] 4.4 Add configurable character encoding support (UTF-8, GBK, ISO-8859-1)

## 5. Independent SFTP Dual-Pane File Manager

- [x] 5.1 Implement independent `SftpWindow` launched via toolbar yellow folder icon
- [x] 5.2 Implement local file system explorer pane with tree view and file list
- [x] 5.3 Implement remote SFTP file system explorer pane using libssh2 SFTP subsystem
- [x] 5.4 Implement drag-and-drop file transfer handlers between local and remote panes
- [x] 5.5 Implement transfer queue widget displaying file progress, transfer rate, and errors

## 6. Verification and Integration

- [x] 6.1 Test connection against modern OpenSSH server with default modern crypto suites
- [x] 6.2 Test Quick Connect, Profile save/load, and reconnect workflows
- [x] 6.3 Test bi-directional drag-and-drop SFTP file uploads and downloads
- [x] 6.4 Verify retro UI visual fidelity on macOS, Linux, and Windows

## 7. Pixel-Perfect UI Refinement (Based on Screenshots)

- [x] 7.1 Refine MainWindow to two-tier toolbars, pure white terminal canvas with copyright banner, and 4-segment statusbar
- [x] 7.2 Refine QuickConnectDialog to "Connect to Remote Host" with dual-PC icon, exact fields, and disabled connect button
- [x] 7.3 Refine ProfileSettingsDialog with circular ssh logo, Connection gradient header, Authentication buttons, and Font picker
- [x] 7.4 Refine SftpWindow with dual independent path bars, "Add" buttons, and Transfer/Queue tabbed view
- [x] 7.5 Compile, test and launch application for visual verification
