// Word Reverser - C++ backend (no external dependencies)
// Serves the frontend and exposes POST /api/reverse?mode=<words|letters|full>
// Build: see Makefile.  Run: ./reverser [port]

#include <algorithm>
#include <cctype>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "reverse.h"

#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #pragma comment(lib, "ws2_32.lib")
  typedef SOCKET sock_t;
  #define CLOSESOCK closesocket
#else
  #include <netinet/in.h>
  #include <sys/socket.h>
  #include <unistd.h>
  typedef int sock_t;
  #define CLOSESOCK close
  #define INVALID_SOCKET (-1)
#endif

// ---------------------------------------------------------------- http helpers

static std::string jsonEscape(const std::string& s) {
    std::string o;
    for (unsigned char c : s) {
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof b, "\\u%04x", c); o += b; }
                else o += c;
        }
    }
    return o;
}

static std::string readFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static void sendResponse(sock_t s, int code, const std::string& status,
                         const std::string& type, const std::string& body) {
    std::ostringstream h;
    h << "HTTP/1.1 " << code << " " << status << "\r\n"
      << "Content-Type: " << type << "\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Access-Control-Allow-Origin: *\r\n"
      << "Connection: close\r\n\r\n";
    std::string out = h.str() + body;
    size_t sent = 0;
    while (sent < out.size()) {
        int n = send(s, out.data() + sent, (int)(out.size() - sent), 0);
        if (n <= 0) break;
        sent += n;
    }
}

static std::string queryParam(const std::string& target, const std::string& key) {
    size_t q = target.find('?');
    if (q == std::string::npos) return "";
    std::string qs = target.substr(q + 1), k = key + "=";
    size_t p = qs.find(k);
    if (p == std::string::npos) return "";
    size_t e = qs.find('&', p);
    return qs.substr(p + k.size(), e == std::string::npos ? std::string::npos : e - p - k.size());
}

static std::string gFrontendDir = "../frontend";

static void handleClient(sock_t client) {
    std::string req;
    char buf[4096];
    size_t headerEnd = std::string::npos, contentLen = 0;
    while (true) {
        int n = recv(client, buf, sizeof buf, 0);
        if (n <= 0) break;
        req.append(buf, n);
        if (req.size() > (1u << 20)) break;  // 1 MB cap
        if (headerEnd == std::string::npos) {
            headerEnd = req.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                std::string head = req.substr(0, headerEnd);
                std::string lower = head;
                std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                size_t cl = lower.find("content-length:");
                if (cl != std::string::npos) contentLen = std::strtoul(lower.c_str() + cl + 15, nullptr, 10);
            }
        }
        if (headerEnd != std::string::npos && req.size() >= headerEnd + 4 + contentLen) break;
    }
    if (headerEnd == std::string::npos) { CLOSESOCK(client); return; }

    std::istringstream line(req.substr(0, req.find("\r\n")));
    std::string method, target, version;
    line >> method >> target >> version;
    std::string body = req.substr(headerEnd + 4);
    std::string path = target.substr(0, target.find('?'));

    if (method == "OPTIONS") {
        sendResponse(client, 204, "No Content", "text/plain", "");
    } else if (method == "POST" && path == "/api/reverse") {
        std::string mode = queryParam(target, "mode");
        std::string result;
        if (mode == "words") result = reverseWordOrder(body);
        else if (mode == "letters") result = reverseEachWord(body);
        else if (mode == "full") result = reverseChars(body);
        else {
            sendResponse(client, 400, "Bad Request", "application/json",
                         "{\"error\":\"mode must be words, letters or full\"}");
            CLOSESOCK(client);
            return;
        }
        std::ostringstream js;
        js << "{\"result\":\"" << jsonEscape(result) << "\",\"mode\":\"" << mode
           << "\",\"words\":" << countWords(body) << "}";
        sendResponse(client, 200, "OK", "application/json", js.str());
    } else if (method == "GET" && (path == "/" || path == "/index.html")) {
        std::string page = readFile(gFrontendDir + "/index.html");
        if (page.empty()) sendResponse(client, 404, "Not Found", "text/plain", "index.html not found");
        else sendResponse(client, 200, "OK", "text/html; charset=utf-8", page);
    } else {
        sendResponse(client, 404, "Not Found", "text/plain", "Not found");
    }
    CLOSESOCK(client);
}

int main(int argc, char** argv) {
    // Port priority: command-line arg, then PORT env var (set by hosts like Render), then 8080.
    const char* envPort = std::getenv("PORT");
    int port = argc > 1 ? std::atoi(argv[1]) : (envPort ? std::atoi(envPort) : 8080);
    if (argc > 2) gFrontendDir = argv[2];

#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&yes, sizeof yes);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);
    if (bind(srv, (sockaddr*)&addr, sizeof addr) < 0 || listen(srv, 16) < 0) {
        std::cerr << "Could not bind to port " << port << "\n";
        return 1;
    }
    std::cout << "Word Reverser running at http://localhost:" << port << std::endl;

    while (true) {
        sock_t c = accept(srv, nullptr, nullptr);
        if (c == INVALID_SOCKET) continue;
        std::thread(handleClient, c).detach();
    }
}
