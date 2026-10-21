## ADDED Requirements

### Requirement: Quick Connect Dialog
The system SHALL provide a Quick Connect modal dialog styled after the classic SSH Secure Shell 3.2.9 dialog, prompting for Host name, User name, and Port.

#### Scenario: Open Quick Connect dialog
- **WHEN** user presses Space, Enter on empty canvas, or clicks the Quick Connect button
- **THEN** system displays the Quick Connect dialog with Host name, User name, and Port fields pre-focused

#### Scenario: Initiate connection from Quick Connect
- **WHEN** user enters host, username, and port and clicks Connect
- **THEN** system initiates the SSH handshake with the specified server parameters

### Requirement: Profile and Session Management
The system SHALL support creating, saving, editing, and loading session profiles with hierarchical settings (Connection, Cipher List, Authentication).

#### Scenario: Save current connection as profile
- **WHEN** user chooses "Save Settings" or "Add Profile"
- **THEN** system saves the current host, port, username, and authentication preferences to local persistent storage

#### Scenario: Load existing profile
- **WHEN** user selects a profile from Profiles menu
- **THEN** system populates connection parameters and automatically attempts connection if configured
