## ADDED Requirements

### Requirement: Modern SSH2 Protocol and Key Exchange
The backend SHALL use libssh2 and OpenSSL 3.x to support modern SSH2 key exchange, public key algorithms, and symmetric ciphers.

#### Scenario: Connect to modern OpenSSH server
- **WHEN** client connects to an OpenSSH 8.x / 9.x server with default settings
- **THEN** client successfully negotiates modern algorithms (e.g. Curve25519 / Diffie-Hellman-Group-Exchange-SHA256, AES-256-CTR / GCM, RSA-SHA2 / Ed25519) without negotiation failure

### Requirement: Multiple Authentication Mechanisms
The backend SHALL support Password, Public Key (RSA, Ed25519), and Keyboard-Interactive authentication methods.

#### Scenario: Password authentication fallback
- **WHEN** server requests password authentication
- **THEN** client prompts the user for credentials and forwards them securely over the encrypted channel

#### Scenario: Public key authentication
- **WHEN** user configures private key authentication
- **THEN** client signs the authentication request using the local key file
