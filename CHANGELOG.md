# Changelog

All notable changes to this project will be documented in this file.

## [1.0.2] - 2025-01-26

### Fixed
- **WebHID compatibility**: Reverted to `MODE="0666"` for udev rules. The `uaccess` tag doesn't work reliably with browser sandboxes. `MODE="0666"` is required for WebHID to function properly with sandboxed browsers like Chrome/Brave. WebHID has its own permission model (user must grant access in browser) so this is safe in practice.

## [1.0.1] - 2025-01-26

### Changed
- **Simplified udev rule format**: Removed unnecessary `TAG+="seat"` (only needed for multi-seat configurations)

### Added
- **Edit Rule functionality**: Double-click a rule or select it and click "Edit Rule" to modify existing rules
- **Notes support**: Add notes to rules to remember why you created them. Notes are stored separately in `~/.local/bin/udevme/notes.json` to avoid parsing issues
- **Improved uninstall script**: Now removes `udevme-run` symlink if present and properly reloads udev rules after removing system rules

### Fixed
- **Notes persistence**: Notes are now stored in a separate JSON file instead of embedded in the rules file, fixing truncation issues with special characters
- **Install script**: Now properly handles reinstallation by removing existing files first

## [1.0.0] - 2025-01-20

### Added
- Initial release
- Device discovery via `/sys/bus/usb/devices`
- Application scanning from `.desktop` files
- Rule creation with device and app selection
- Rule management (add, remove, enable/disable)
- System rules sync on startup
- Privilege escalation via pkexec/sudo for applying rules
- Log widget showing operation details
- Theme-native Qt 6 interface

