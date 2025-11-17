# Contributing to xeus-sas

Thank you for your interest in contributing to xeus-sas! This document provides guidelines and information for contributors.

## Getting Started

1. Fork the repository on GitHub
2. Clone your fork locally
3. Set up the development environment
4. Create a branch for your work
5. Make your changes
6. Run tests
7. Submit a pull request

## Development Setup

### Prerequisites

- C++17 compatible compiler
- CMake 3.8+
- xeus, xeus-zmq, xtl, nlohmann_json
- SAS 9.4+ (for testing)
- Google Test (optional, for tests)

### Building from Source

```bash
git clone https://github.com/YOUR_USERNAME/xeus-sas.git
cd xeus-sas
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTS=ON
make -j4
```

### Running Tests

```bash
cd build
ctest --output-on-failure
```

## Code Style

### C++ Style

We follow the C++ Core Guidelines with these specifics:

- Use C++17 features where appropriate
- Prefer `const` and `constexpr` when possible
- Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) over raw pointers
- Use RAII for resource management
- Include guards: `#ifndef XEUS_SAS_FILENAME_HPP`
- Namespace: All code in `xeus_sas` namespace

### Formatting

- Indentation: 4 spaces (no tabs)
- Braces: Opening brace on same line
- Line length: 100 characters max (soft limit)
- Use descriptive variable names

Example:
```cpp
namespace xeus_sas
{
    class my_class
    {
    public:
        my_class();

        void do_something(const std::string& input);

    private:
        std::string m_member_variable;
    };
}
```

### Comments and Documentation

- Use Doxygen-style comments for public APIs
- Comment "why", not "what" (code should be self-documenting)
- Keep comments up-to-date with code changes

Example:
```cpp
/**
 * @brief Parse SAS execution output
 *
 * Separates log from listing and extracts error information.
 *
 * @param raw_output Combined SAS output
 * @return Structured execution result
 */
execution_result parse_execution_output(const std::string& raw_output);
```

## Pull Request Process

1. **Create a feature branch**
   ```bash
   git checkout -b feature/my-feature
   ```

2. **Make your changes**
   - Write code following the style guide
   - Add tests for new functionality
   - Update documentation as needed

3. **Commit your changes**
   - Use clear, descriptive commit messages
   - Reference issues when applicable
   ```
   Add support for ODS graphics parsing

   - Implement extract_graph_files() in parser
   - Add tests for PNG/SVG detection
   - Update documentation

   Fixes #42
   ```

4. **Push to your fork**
   ```bash
   git push origin feature/my-feature
   ```

5. **Submit a pull request**
   - Describe your changes clearly
   - Link related issues
   - Wait for review and address feedback

### PR Checklist

- [ ] Code follows project style guidelines
- [ ] Tests added/updated and passing
- [ ] Documentation updated (if needed)
- [ ] Commit messages are clear and descriptive
- [ ] Branch is up-to-date with main

## Testing Guidelines

### Unit Tests

- Use Google Test framework
- Test files named `test_*.cpp`
- One test file per source file
- Test edge cases and error conditions

Example:
```cpp
TEST(ParserTest, DetectError)
{
    std::string log = "ERROR: Test error";
    int error_code = 0;

    EXPECT_TRUE(contains_error(log, error_code));
    EXPECT_GT(error_code, 0);
}
```

### Integration Tests

- Tests requiring SAS should be marked `DISABLED_`
- Provide instructions for running with SAS installed
- Document expected SAS version/configuration

## Areas for Contribution

### High Priority

- **Output Parsing**: Improve log/listing separation
- **Graphics Support**: Better ODS graphics handling
- **Error Detection**: More robust error parsing
- **Documentation**: Usage examples and tutorials

### Medium Priority

- **Code Completion**: Variable/dataset name completion
- **Inspection**: Enhanced help system
- **Platform Support**: Windows compatibility
- **Performance**: Optimize batch execution

### Nice to Have

- **IOM Support**: Remote SAS connections
- **Macro Debugging**: Step through macro code
- **Interactive Mode**: Persistent SAS session
- **xwidgets**: Interactive widgets support

## Reporting Bugs

When reporting bugs, please include:

1. **Description**: Clear description of the issue
2. **Steps to Reproduce**: Minimal code to reproduce
3. **Expected Behavior**: What should happen
4. **Actual Behavior**: What actually happens
5. **Environment**:
   - OS and version
   - SAS version
   - xeus-sas version
   - Compiler version

Example:
```
### Bug Description
SAS log not displayed when error occurs

### Steps to Reproduce
1. Execute: `INVALID CODE;`
2. Observe output

### Expected
SAS log with error message should be displayed

### Actual
No output shown

### Environment
- OS: Ubuntu 22.04
- SAS: 9.4 M7
- xeus-sas: 0.1.0
- Compiler: GCC 11.2
```

## Suggesting Features

Feature requests are welcome! Please:

1. Check existing issues first
2. Describe the use case
3. Explain why it's valuable
4. Propose an implementation (if you have ideas)

## Code of Conduct

### Our Pledge

We pledge to make participation in this project a harassment-free experience for everyone, regardless of:
- Age, body size, disability, ethnicity
- Gender identity and expression
- Level of experience
- Nationality, personal appearance, race, religion
- Sexual identity and orientation

### Our Standards

**Positive behavior:**
- Using welcoming and inclusive language
- Being respectful of differing viewpoints
- Gracefully accepting constructive criticism
- Focusing on what's best for the community

**Unacceptable behavior:**
- Trolling, insulting/derogatory comments, personal attacks
- Public or private harassment
- Publishing others' private information
- Other conduct reasonably considered inappropriate

## License

By contributing to xeus-sas, you agree that your contributions will be licensed under the BSD-3-Clause License.

## Questions?

- Open a [GitHub Discussion](https://github.com/jupyter-xeus/xeus-sas/discussions)
- Join the [xeus Gitter channel](https://gitter.im/QuantStack/Lobby)
- Email the maintainers (see AUTHORS.md)

## Acknowledgments

Thank you for contributing to xeus-sas! Every contribution, no matter how small, helps make the project better.
