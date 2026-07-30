## ADDED Requirements

### Requirement: CI 后端基础作业可移植

The CI backend foundation job SHALL build and test the backend foundation without requiring a Visual Studio generator.

#### Scenario: GitHub Actions 运行后端基础测试

- **GIVEN** GitHub Actions receives a push or pull request
- **WHEN** the backend foundation job configures CMake
- **THEN** the job SHALL use a runner and CMake configuration that does not require a local Visual Studio installation
- **AND** the job SHALL build `induspilot-backend-tests`
- **AND** the job SHALL run CTest with failure output enabled
