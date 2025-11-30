# Changelog

All notable changes to RocketEditor will be documented in this file.

## [1.3.0] - 2025-11-30

### Added
- Command-line support for loading .rocket files on startup (all platforms)
- macOS universal binary support (x86_64 + arm64)
- Version display now shows git tag for releases, "Development" for dev builds

### Changed
- Replaced BASS audio library with minimp3 + kissfft for audio decoding (no more proprietary dependencies)
- Switched build system to CMake

### Fixed
- Various build warnings and CI improvements
