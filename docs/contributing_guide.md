# <img src="icon.png" alt="WinKernelLite Logo" width="64" height="64" style="vertical-align: middle; margin-right: 10px;"> Contributing to WinKernelLite

Thank you for your interest in contributing to WinKernelLite! This guide covers the essentials for getting started.

## Quick Start

### Prerequisites
- Windows OS
- CMake 3.29+
- Visual Studio 2019+ or VS Code
- Git

### Setup
1. Fork and clone the repository
2. Build the project:
   ```powershell
   mkdir build && cd build
   cmake ..
   cmake --build .
   ```
3. Run tests: `.\build\bin\runTests.exe`

## How to Contribute

### Reporting Issues
- Use clear, descriptive titles
- Include steps to reproduce
- Provide system information (OS, compiler, version)
- Add screenshots or code snippets if helpful

### Pull Requests
1. Create a feature branch: `git checkout -b feature/your-feature`
2. Make your changes
3. Add tests for new functionality
4. Ensure all tests pass
5. Submit a pull request with a clear description

## Coding Standards

### C Code Style
- Use PascalCase for functions: `InitializeListHead()`
- Use meaningful variable names
- 4 spaces for indentation
- 80-100 character line limit
- Add parameter annotations (`_In_`, `_Out_`, etc.)

### Example
```c
NTSTATUS
RtlCopyUnicodeString(
    _Inout_ PUNICODE_STRING DestinationString,
    _In_opt_ PCUNICODE_STRING SourceString
)
{
    if (DestinationString == NULL) {
        return STATUS_INVALID_PARAMETER;
    }
    
    // Implementation here
    return STATUS_SUCCESS;
}
```

### Header Files
- Use include guards
- Order includes: standard → Windows → project headers
- Use `extern "C"` for C++ compatibility
- Document public functions

## Testing

### Writing Tests
- Add unit tests for all new features
- Use Google Test framework
- Follow the existing test patterns

### Example Test
```cpp
TEST(UnicodeStringTest, InitializeString) {
    UNICODE_STRING str;
    RtlInitUnicodeString(&str, L"Test");
    
    EXPECT_EQ(8, str.Length);
    EXPECT_NE(nullptr, str.Buffer);
}
```

### Running Tests
```powershell
# All tests
.\build\bin\runTests.exe

# Specific tests
.\build\bin\runTests.exe --gtest_filter="*Unicode*"
```

## Documentation

### Code Comments
- Document all public functions
- Use clear, concise descriptions
- Include parameter and return value information

### Example Documentation
```c
/**
 * @brief Allocates memory from the specified pool.
 * @param PoolType Type of pool to allocate from
 * @param NumberOfBytes Size in bytes
 * @return Pointer to allocated memory, or NULL if failed
 */
PVOID ExAllocatePool(POOL_TYPE PoolType, SIZE_T NumberOfBytes);
```

## Git Workflow

### Branch Naming
- `feature/feature-name` - New features
- `bugfix/issue-description` - Bug fixes
- `docs/update-description` - Documentation updates

### Commit Messages
- Keep first line under 50 characters
- Use clear, descriptive language
- Reference issue numbers when applicable

Example:
```
Add memory allocation tracking

Implements tracked allocation to help debug memory leaks.
Fixes #42
```

## Getting Help

- Create an issue for questions
- Check existing issues and documentation first
- Be specific about your problem or question

By contributing, you agree that your contributions will be licensed under the Apache License 2.0.

---

Happy coding! 🚀
