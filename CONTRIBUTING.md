# Contributing to Real-Time Camera Viewer

Thank you for your interest in contributing! We welcome pull requests, bug reports, and feature suggestions.

## How to Contribute

### 1. Report a Bug
- Check existing issues first
- Include system information (OS, CMake version, compiler)
- Provide steps to reproduce
- Include error messages and logs

### 2. Suggest a Feature
- Describe the desired functionality
- Explain the use case
- Note if you're willing to implement it

### 3. Submit a Pull Request
1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Code Style

- Follow existing code conventions
- Use snake_case for variables and functions
- Use CamelCase for classes
- Add comments for non-obvious logic
- Keep lines under 100 characters when possible

## Testing

Before submitting a PR:
1. Build without errors: `cmake .. && make`
2. Test basic camera functionality
3. Test CSV export
4. Test with multiple cameras if possible

## Areas Needing Help

- YOLO detection integration fixes
- GPU acceleration (CUDA/OpenGL)
- Additional camera format support
- macOS/Windows compatibility
- Performance optimizations
- Documentation improvements

## Code Review Process

- Maintainer reviews within 5-7 days
- Changes may be requested
- Once approved, PR is merged

## Questions?

Feel free to open an issue with the `question` label!

Thank you for contributing! 🎉
