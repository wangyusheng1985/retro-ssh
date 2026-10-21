## ADDED Requirements

### Requirement: Independent SFTP Window Launch
The system SHALL launch an independent, non-blocking SFTP file manager window when the yellow folder icon on the toolbar is clicked.

#### Scenario: User clicks SFTP icon while connected
- **WHEN** user clicks the Yellow Folder toolbar button on an active connection
- **THEN** system opens a separate SFTP window sharing or multiplexing the authenticated session and displays local and remote file directories

#### Scenario: User clicks SFTP icon without active connection
- **WHEN** user clicks the Yellow Folder toolbar button with no active connection
- **THEN** system prompts Quick Connect dialog to establish the session first

### Requirement: Dual-Pane File Browsing and Drag-and-Drop
The system SHALL present a dual-pane file management interface with the local file system on the left and the remote file system on the right, supporting drag-and-drop file transfers.

#### Scenario: Browse remote directories
- **WHEN** user double-clicks a directory on the remote file tree or list
- **THEN** system queries remote SFTP directory entries and updates the view

#### Scenario: Drag-and-drop file upload
- **WHEN** user drags one or more files from the local left pane and drops them into the remote right pane
- **THEN** system initiates upload tasks into the remote target folder and displays real-time progress

#### Scenario: Drag-and-drop file download
- **WHEN** user drags files from the remote right pane and drops them into the local left pane
- **THEN** system downloads the remote files to the selected local directory
