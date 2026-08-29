# Security Policy

## Supported Versions

Security fixes are applied to the latest stable release line.

| Version | Supported          |
| ------- | ------------------ |
| 0.6.x   | :white_check_mark: |
| < 0.6.0 | :x:                |

## Reporting a Vulnerability

Dragonchain is a post-quantum cryptocurrency that replaces ECDSA with Falcon-512
lattice-based signatures from genesis. The correctness of the signature scheme,
consensus rules, and P2P networking is critical.

Please report security vulnerabilities privately through one of these channels:

- **GitHub Security Advisory** (preferred): use the "Report a vulnerability" flow
  on the repository's Security tab.
- **Email**: `security@dragonchain.cc`

### Guidelines

- Do **not** open a public issue for a security vulnerability.
- Include a clear description, affected version(s), and steps to reproduce
  (if possible).
- Allow a reasonable disclosure window before publicizing the issue.

## Scope

The following areas are in scope:

- Consensus rules and block validation
- Falcon-512 post-quantum signature implementation
- P2P networking and peer management
- Wallet key handling and RPC authentication

## Disclosure Process

1. Vulnerability is reported privately.
2. Maintainers acknowledge receipt within 48 hours.
3. A fix is prepared and released as a patch version.
4. A security advisory is published after a fix is available.
