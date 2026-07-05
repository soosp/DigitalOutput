# Changelog

All notable changes to this project will be documented in this file.

## [Unreleased]

## [0.4.0]

### Added

- Add MCP23X08 expander chip support

### Changed

- `_init()` and `_operate()` hardware hooks now return `bool` to report
  success or failure

### Fixed

- Roll back cached state and return `false` when a hardware write fails,
  instead of reporting success with a state that diverges from the hardware

## [0.3.1]

### Added

- Add "One instance per pin" note
- Add Doxygen comment about the parameter of `begin()` method.

### Changed

- The list of methods has been removed from the introduction to `McpDigitalOutput`

### Fixed

- Fix `PulseProgress` example to work with Arduino IDE Serial Monitor.
- Check `Serial` in `Morse` example to avoid hanging on headless CDC ports.
- Markdown fixes in Changelog

## [0.3.0]

### Added

- Add `isPulsing()` method
- Add `remaining()` method
- Add `Status` snapshot structure
- Add `getStatus()` snapshot method
- Add `PulseProgress` example

### Changed

- Fix Changelog

## [0.2.0]

### Added

- Add `getBaseline()` method

### Changed

- Minor documentation fixes

## [0.1.2]

### Changed

- Fix dependecy for Adruino Library Registry (again)

## [0.1.1]

### Changed

- Fix dependecy for Adruino Library Registry

## [0.1.0]

### Added

- First public release

[unreleased]: https://github.com/soosp/DigitalOutput/compare/0.4.0...HEAD
[0.4.0]: https://github.com/soosp/DigitalOutput/compare/0.3.1...0.4.0
[0.3.1]: https://github.com/soosp/DigitalOutput/compare/0.3.0...0.3.1
[0.3.0]: https://github.com/soosp/DigitalOutput/compare/0.2.0...0.3.0
[0.2.0]: https://github.com/soosp/DigitalOutput/compare/0.1.2...0.2.0
[0.1.2]: https://github.com/soosp/DigitalOutput/compare/0.1.1...0.1.2
[0.1.1]: https://github.com/soosp/DigitalOutput/compare/0.1.0...0.1.1
[0.1.0]: https://github.com/soosp/DigitalOutput/releases/tag/0.1.0
