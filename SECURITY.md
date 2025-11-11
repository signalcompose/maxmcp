# Security Policy

## Supported Versions

We actively support the following versions of MaxMCP with security updates:

| Version | Supported          |
| ------- | ------------------ |
| 1.x.x   | :white_check_mark: |
| < 1.0   | :x:                |

## Reporting a Vulnerability

We take the security of MaxMCP seriously. If you discover a security vulnerability, please follow these steps:

### Private Disclosure

**DO NOT** create a public GitHub issue for security vulnerabilities.

Instead, please report security vulnerabilities privately:

1. **Email**: Send details to the project maintainers (contact information will be provided when repository is made public)
2. **GitHub Security Advisory**: Use GitHub's [private vulnerability reporting](https://github.com/signalcompose/MaxMCP/security/advisories/new) feature

### What to Include

When reporting a vulnerability, please include:

- **Description**: Clear description of the vulnerability
- **Impact**: Potential impact and attack scenarios
- **Reproduction Steps**: Detailed steps to reproduce the issue
- **Affected Versions**: Which versions are affected
- **Suggested Fix**: If you have one (optional)

### Response Timeline

- **Initial Response**: Within 48 hours
- **Confirmation**: Within 7 days (whether we can reproduce the issue)
- **Fix Timeline**: Depends on severity
  - **Critical**: Within 7 days
  - **High**: Within 14 days
  - **Medium**: Within 30 days
  - **Low**: Next regular release

### Disclosure Policy

- We follow **coordinated disclosure**
- We will notify you when a fix is available
- We will credit you in the security advisory (unless you prefer to remain anonymous)
- Please allow us reasonable time to fix the issue before public disclosure

## Security Best Practices

When using MaxMCP:

- **Keep Updated**: Always use the latest version
- **Network Security**: WebSocket connections should be on localhost (127.0.0.1) or secured networks
- **Dependency Updates**: Monitor for updates to bundled libraries (OpenSSL, libwebsockets)
- **Max Patches**: Be cautious when loading untrusted Max patches

## Known Security Considerations

### WebSocket Connection

- MaxMCP bridge uses WebSocket on `ws://localhost:7400` by default
- This is **intended for local development** only
- For production use, ensure the WebSocket port is not exposed to untrusted networks

### Bundled Libraries

MaxMCP bundles these security-critical libraries:

- **OpenSSL 3.x**: For TLS/SSL support
- **libwebsockets**: For WebSocket protocol

We monitor security advisories for these libraries and update them promptly when vulnerabilities are discovered.

## Security Updates

Security updates are released:

- **Immediately** for critical vulnerabilities
- Via GitHub Security Advisories
- With clear upgrade instructions
- Including CVE numbers when applicable

## Questions?

If you have questions about this security policy, please open a GitHub Discussion or contact the maintainers.

Thank you for helping keep MaxMCP secure!
