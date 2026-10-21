## ADDED Requirements

### Requirement: macOS Application Bundle Icon Packaging
The build system SHALL package a valid Apple multi-resolution `.icns` icon file containing 16x16 through 1024x1024 resolutions into the macOS application bundle `Contents/Resources/` directory and configure the bundle properties accordingly.

#### Scenario: Verify bundle icon configuration
- **WHEN** building the macOS bundle target via CMake
- **THEN** `RetroSSH.icns` is copied into `RetroSSH.app/Contents/Resources/RetroSSH.icns` and `CFBundleIconFile` in `Info.plist` references `RetroSSH.icns`

#### Scenario: Finder and Dock icon display
- **WHEN** the application is compiled and viewed in macOS Finder or Dock
- **THEN** the operating system displays the customized RetroSSH application icon
