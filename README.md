# SeniorSphere — TIET

A student mentorship platform built with React and a C++17 backend for an Object-Oriented Programming project.

**Live website:** https://seniorsphere-tiet.vercel.app  
**Backend health:** https://seniorsphere-backend-production.up.railway.app/api/health

## Features

- College-email signup, login and editable profiles
- Senior discovery and connections
- Private one-to-one chat with unread counts
- Doubt forum, anonymous questions, answers and question upvotes
- Mentorship slots, help requests and bookings
- Notifications and persistent JSON-file storage

## Technology

| Layer | Technology |
|---|---|
| Frontend | React 18, Vite, Tailwind CSS |
| Backend | C++17 and cpp-httplib |
| JSON | nlohmann/json, behind a small application interface |
| Passwords | libsodium Argon2id |
| Storage | JSON file on a persistent Railway volume |
| Hosting | Vercel frontend; Railway backend |

## How the backend works

React sends an HTTP request. A controller checks login and input, updates model objects through a repository, saves the database, and returns JSON. Chat updates are fetched every three seconds.

`React → Controller → Model / Repository → Database → JSON response`

The feature code uses classes, encapsulation, inheritance, abstract interfaces, polymorphism, templates, exceptions and STL containers. See [the explanation guide](docs/BACKEND_GUIDE.md) for examples and a reading order.

## Run locally

### Frontend

Install Node.js 24 and run from the repository root:

```sh
npm ci
npm run dev
```

Vite forwards `/api` requests to `http://localhost:8080`. To use a different server, copy `.env.example` to `.env` and set `VITE_API_URL`.

### Backend on Debian / Ubuntu

```sh
sudo apt-get install g++ cmake make libsodium-dev
cd backend
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
./build/seniorsphere
```

On Windows, install a C++17 compiler, CMake and libsodium development files, then run `backend/build.bat`. Configure CMake's include/library paths if libsodium is outside the compiler's search path.

The backend starts with an empty database. This public repository contains no student accounts, passwords or saved messages. Its seed file is intentionally empty.

| Variable | Default | Purpose |
|---|---|---|
| `PORT` | `8080` | HTTP server port |
| `DATA_DIR` | `data` | Directory containing `db.json` |
| `CORS_ORIGIN` | Local Vite origins | Allowed frontend origins, comma-separated |
| `SEED_DEMO` | Disabled | Loads the seed only on first start when set to `1` |

Use a data directory outside OneDrive or other synchronized folders to avoid file-replacement conflicts.

## Tests

```sh
python backend/tests/regression.py backend/build/seniorsphere
```

On Windows, pass the built `.exe` path. The test suite uses an isolated temporary database and does not touch application data. It checks authentication, privacy, profile validation, storage failure handling, chat, concurrent slot claims, CORS and restart persistence. The 40-check suite passed during deployment preparation.

## Deployment

**Vercel:** repository root, Vite framework, `npm ci`, `npm run build`, output `dist`. Set `VITE_API_URL` to the backend HTTPS URL before building.

**Railway:** use `backend` as the service root and `Dockerfile` as the build source. Attach a persistent volume at `/data`, use one replica, and configure `PORT=8080`, `DATA_DIR=/data`, `SEED_DEMO=0`, and `CORS_ORIGIN` to the exact production frontend origin. Set `/api/health` as the health check. Configure these settings in Railway; this repository intentionally omits the deprecated `railway.json` format.

The public repository is a source copy. Pushing to it does not automatically update the currently hosted services until Git deployment integration is connected.

## Project limitations

This is an educational project. Email ownership and academic year are not verified. Sessions expire after 24 hours and are cleared when the server restarts. Whole-file JSON storage is intended for small datasets and one server process. There is no password-reset or moderation workflow. Bookings are immediately marked confirmed; cancellation, answer acceptance and answer voting are not implemented. The website runs on hosting-plan allowances, including Railway trial credit.

## Third-party libraries

cpp-httplib and nlohmann/json are vendored under `backend/third_party`, alongside their licenses. libsodium is installed as a system dependency; its license is also included there. Library implementation code is not original project code.
