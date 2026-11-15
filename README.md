# SeniorSphere

A platform for TIET students to ask doubts, connect with seniors and book mentorship sessions. Built as a C++ OOP project with a React frontend.

[Live website](https://seniorsphere-tiet.vercel.app)

## Features

- Signup and login with a college email address
- Student profiles and senior directory
- Private chat with unread message counts
- Doubt forum with anonymous posts, answers and question upvotes
- Mentorship slots, help requests and bookings
- Notifications

## Stack

React, Vite and Tailwind CSS for the frontend. C++17 for the backend, with cpp-httplib for HTTP, nlohmann/json for JSON handling and libsodium for password hashing. Data is saved in a JSON file. The frontend runs on Vercel and the backend on Railway.

## Running locally

Install Node.js 24, then run from the project root:

```sh
npm ci
npm run dev
```

To build the backend on Debian/Ubuntu:

```sh
sudo apt-get install g++ cmake make libsodium-dev
cd backend
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
./build/seniorsphere
```

On Windows, use CMake, a C++17 compiler and libsodium development files, then run `backend/build.bat`. Set the include/library paths in CMake if libsodium is installed outside the compiler's search path.

The backend runs on port 8080. Vite forwards `/api` requests to it. To use another backend, copy `.env.example` to `.env` and change `VITE_API_URL`.

Backend settings are listed in `backend/.env.example`. Set `DATA_DIR` to choose where `db.json` is stored and `CORS_ORIGIN` to allow your frontend's origin. Keep the data directory outside cloud-synced folders. The included seed file is empty.

## Backend structure

Controllers handle requests and validation. Models represent users, questions, conversations and bookings. `Repository<T>` stores these objects, and `Database` loads and saves them.

`Entity` and `Controller` provide abstract interfaces. Their derived classes demonstrate inheritance and polymorphism; repositories demonstrate templates. Private identity and password fields in `User` demonstrate encapsulation. Chat is refreshed by polling every three seconds.

## Tests

```sh
python backend/tests/regression.py backend/build/seniorsphere
```

On Windows, use the built executable's path. Tests use a separate temporary database and cover authentication, privacy, chat, slot claims and persistence.

## Hosting

Vercel builds the frontend using `npm run build` and serves `dist`. Set `VITE_API_URL` to the backend's HTTPS URL before deployment.

On Railway, select `backend` as the root directory and use its Dockerfile. Attach a persistent volume at `/data`, use one replica, and set `PORT=8080`, `DATA_DIR=/data`, `SEED_DEMO=0` and `CORS_ORIGIN` to the frontend origin. The health endpoint is `/api/health`. Configure these settings in Railway rather than using the deprecated `railway.json` format.

## Limitations

Email ownership and academic year are not verified. Sessions expire after 24 hours and survive server restarts. Logout invalidates the saved session. JSON-file storage is intended for a small, single-server project. Bookings are marked confirmed immediately; cancellation, answer acceptance, password reset and moderation are not implemented.

## Libraries

cpp-httplib and nlohmann/json are included under `backend/third_party`. Their licenses and the libsodium license are included there.
