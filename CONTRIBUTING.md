# <img src="docs/icon.png" alt="WinKernelLite Logo" width="64" height="64" style="vertical-align: middle; margin-right: 10px;"> Contributing to WinKernelLite

Thank you for your interest in contributing to WinKernelLite! This document provides guidelines and instructions for contributing.

## Code of Conduct

Please be respectful and considerate of others when contributing to this project. We aim to foster an inclusive and welcoming community.

## How to Contribute

### Reporting Bugs

If you find a bug, please create an issue on GitHub with the following information:

1. A clear, descriptive title
2. A detailed description of the issue
3. Steps to reproduce the bug
4. Expected behavior vs. actual behavior
5. System information (OS version, compiler version, etc.)
6. Any relevant code snippets or screenshots

### Suggesting Enhancements

For feature requests or enhancement suggestions, please create an issue with:

1. A clear, descriptive title
2. A detailed description of the proposed feature
3. Any relevant examples, mockups, or use cases
4. Why this enhancement would be useful to most users

### Pull Requests

1. Fork the repository
2. Create a new branch (`git checkout -b feature/your-feature-name`)
3. Make your changes
4. Run the tests to ensure they pass
5. Commit your changes (`git commit -m 'Add some feature'`)
6. Push to your branch (`git push origin feature/your-feature-name`)
7. Open a Pull Request

#### Pull Request Guidelines

- Follow the existing code style and conventions
- Include tests for new features or bug fixes
- Update documentation for any changed functionality
- Keep pull requests focused on a single change
- Link to any relevant issues

## Development Setup

### Prerequisites

- Windows operating system (supports both 32-bit and 64-bit)
- CMake 3.29 or higher
- Visual Studio 2019 or later with C/C++ development tools

### Building for Development

```bash
# Clone the repository
git clone https://github.com/TorinKS/WinKernelLite.git
cd WinKernelLite

# Create a build directory
mkdir build
cd build

# Configure the project with tests enabled
cmake .. -DBUILD_TESTS=ON

# Build
cmake --build . --config Debug
```

### Running Tests

```bash
cd build
ctest -C Debug --output-on-failure
```

## Coding Standards

- Use consistent indentation (4 spaces)
- Follow the existing naming conventions
- Write clear, descriptive comments
- Add appropriate header documentation for public functions
- Keep functions focused and reasonably sized

### C Code Style

- Use PascalCase for function names
- Use Hungarian notation for Windows API compatibility
- Add appropriate parameter annotations (_In_, _Out_, etc.)
- Check return values and handle errors appropriately

## Documentation

- Update any relevant documentation when changing code
- Use clear, concise language
- Provide examples for complex functionality
- Document function parameters and return values

## Git Commit Messages

- Use clear, descriptive commit messages
- Start with a short summary line (50 chars or less)
- Follow with a more detailed explanation if necessary
- Reference issue numbers when applicable

Example:
```
Add ExAllocatePoolTracked function

This implements a tracked version of ExAllocatePool that records
allocations for debugging memory leaks.

Fixes #42
```

## License

By contributing to this project, you agree that your contributions will be licensed under the project's [Apache License 2.0](LICENSE).

Thank you for contributing to WinKernelLite!
