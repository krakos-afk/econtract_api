# eContract API

A C++17 service for managing electronic contracts: creation, versioning, digital signatures, lifecycle tracking, and compliance metadata.

## Table of Contents
- Overview
- Features
- Project structure
- Build requirements
- Build & run
- Configuration
- Database
- Security
- License

## Overview
This repository contains a C++17 application that exposes contract functionality (create, update, sign, audit, and search). It links against OpenSSL for cryptographic operations and libpqxx/libpq for PostgreSQL access.

## Features
- Contract CRUD and lifecycle (draft -> finalized -> archived)
- Versioning and audit trail
- Signer workflow & signature verification (OpenSSL)
- PostgreSQL persistence (libpqxx)
- Clean build targets via Makefile

## Project structure
```
├─ Makefile
├─ src/
│  ├─ api/               # HTTP/API layer (handlers)
│  ├─ services/          # Business logic
│  ├─ database/          # Database access (libpqxx)
│  ├─ crypto/            # Crypto helpers (OpenSSL)
│  ├─ blockchain/        # Block/ledger utilities
│  ├─ config/            # Configuration helpers
│  └─ main.cpp           # Entry point
├─ build/                # Generated object files and intermediates (ignored)
└─ econtract_api         # Built binary (should not be committed)
```

## Build requirements
- g++ with C++17 support
- OpenSSL (libssl, libcrypto)
- PostgreSQL client libraries (libpq, libpqxx)
- pthread
- make

Ubuntu/Debian example:
```bash
sudo apt-get update
sudo apt-get install -y build-essential libssl-dev libpq-dev libpqxx-dev
```

## Build & run
Use the provided Makefile:
```bash
# build
make

# run
./econtract_api

# clean
make clean

# rebuild from scratch
make rebuild
```

Targets are defined in the Makefile and compile all sources under src/ into the build/ directory, linking into the econtract_api executable.

## Configuration
Place configuration (e.g., database connection) in the config layer (see src/config). If you prefer environment variables, load them early in main.cpp and pass to components. Example variables you might use:
- DATABASE_URL=postgres://user:pass@localhost:5432/econtract
- APP_ENV=development

## Database
Migrations are not included here. Recommended approaches:
- SQL files in a migrations/ directory
- A migration tool (e.g., sqitch or a simple custom runner)

Ensure a PostgreSQL database is available and your DATABASE_URL is correctly set before running.


## Security
- Validate and sanitize all inputs in the API layer
- Use parameterized queries for PostgreSQL
- Protect secrets (never commit secrets; use environment variables or external secret stores)
- Keep OpenSSL and libpqxx up to date


## License
MIT. See LICENSE.
