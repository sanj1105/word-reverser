# Word Reverser (C++ full stack)

A small full stack app: a dependency-free C++ HTTP server plus a plain HTML/JS frontend.

## Structure

```
word-reverser/
├── backend/main.cpp     # HTTP server + reversal logic (sockets, no libraries)
├── frontend/index.html  # UI, served by the backend at /
├── Makefile
└── README.md
```

## Build and run

Linux / macOS:

```
make
make run          # then open http://localhost:8080
```

Windows (MinGW g++):

```
g++ -std=c++17 -O2 -o backend/reverser.exe backend/main.cpp -lws2_32
cd backend
reverser.exe 8080 ..\frontend
```

Usage: `reverser [port] [frontend_dir]` (defaults: `8080`, `../frontend`).

## API

`POST /api/reverse?mode=<mode>` with the raw text as the request body.

| mode      | input             | output            |
|-----------|-------------------|-------------------|
| `words`   | `hello big world` | `world big hello` |
| `letters` | `hello big world` | `olleh gib dlrow` |
| `full`    | `hello big world` | `dlrow gib olleh` |

Response: `{"result":"...","mode":"words","words":3}`

```
curl -X POST "http://localhost:8080/api/reverse?mode=words" -d "hello big world"
```

## Deploy on GitHub Pages (WebAssembly)

GitHub Pages only serves static files, so the C++ is compiled to WebAssembly and runs in the browser.
The shared logic lives in `backend/reverse.h`; `wasm/reverse_wasm.cpp` exposes it to JavaScript, and
`.github/workflows/pages.yml` builds and publishes it on every push to `main`.

1. Push the repo to GitHub.
2. Repo Settings -> Pages -> Build and deployment -> Source: **GitHub Actions**.
3. Open the Actions tab, wait for "Deploy to GitHub Pages" to turn green.
4. Your site is at `https://<username>.github.io/word-reverser/`.

The same `frontend/index.html` is used in both modes: it calls the WebAssembly module when
`reverse.js` exists (Pages) and the C++ server API otherwise (local).

## Deploy with Docker (optional, needs a host that supports containers)

The included `Dockerfile` builds the native server. It reads the `PORT` environment variable.

## Notes

- Letter reversal works on UTF-8 code points, so non-ASCII text isn't corrupted.
- Each connection is handled on its own thread; request size is capped at 1 MB.
- Ideas to extend: a database for history (SQLite), palindrome check, file upload, unit tests.
