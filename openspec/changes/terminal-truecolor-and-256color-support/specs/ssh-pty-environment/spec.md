## ADDED Requirements

### Requirement: Modern PTY Terminal Type Negotiation
The SSH session worker SHALL request `xterm-256color` as the default PTY terminal emulation type when opening an interactive shell channel with the remote host.

#### Scenario: Request PTY with 256color support
- **WHEN** establishing an interactive SSH shell channel
- **THEN** `libssh2_channel_request_pty` is invoked with `"xterm-256color"` instead of legacy `"xterm"`
