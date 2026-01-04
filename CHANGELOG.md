# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Initial project structure
- Integration with hal753 v1.0.1 library
- Active Objects: Blinky, Mongoose, Stack Monitor
- Mongoose network framework integration
- QP/C framework adapter with application_init() entry point
- CMake build system with CPM dependency management
- Docker-based CI/CD using ghcr.io/kodezine/kdocker:latest
- Comprehensive documentation suite
- Pre-commit hooks with clang-format and cmake-format
- Support for GNU ARM 14.3 and ARM Compiler for Embedded 21.1 toolchains

## [0.1.0] - 2026-01-04

### Added
- Initial release
- Firmware executable for STM32H753 Nucleo board
- CAN and Ethernet peripheral support
