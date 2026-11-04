// Shared reversal logic, used by the native server (backend/main.cpp)
// and the WebAssembly build (wasm/reverse_wasm.cpp).
#pragma once
#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <vector>

// ---------------------------------------------------------------- core logic

// Split a UTF-8 string into code points so reversal doesn't corrupt non-ASCII text.
static inline std::vector<std::string> codepoints(const std::string& s) {
    std::vector<std::string> out;
    for (size_t i = 0; i < s.size();) {
        unsigned char c = s[i];
        size_t len = c < 0x80 ? 1 : (c >> 5) == 6 ? 2 : (c >> 4) == 14 ? 3 : (c >> 3) == 30 ? 4 : 1;
        if (i + len > s.size()) len = 1;
        out.push_back(s.substr(i, len));
        i += len;
    }
    return out;
}

static inline std::string reverseChars(const std::string& s) {
    auto cps = codepoints(s);
    std::reverse(cps.begin(), cps.end());
    std::string r;
    for (auto& c : cps) r += c;
    return r;
}

static inline bool isSpace(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }

// "hello big world" -> "world big hello"
static inline std::string reverseWordOrder(const std::string& s) {
    std::vector<std::string> words;
    std::istringstream iss(s);
    std::string w;
    while (iss >> w) words.push_back(w);
    std::reverse(words.begin(), words.end());
    std::string r;
    for (size_t i = 0; i < words.size(); i++) r += (i ? " " : "") + words[i];
    return r;
}

// "hello big world" -> "olleh gib dlrow" (spacing preserved)
static inline std::string reverseEachWord(const std::string& s) {
    std::string r, cur;
    for (char c : s) {
        if (isSpace(c)) {
            r += reverseChars(cur);
            cur.clear();
            r += c;
        } else cur += c;
    }
    r += reverseChars(cur);
    return r;
}

static inline size_t countWords(const std::string& s) {
    std::istringstream iss(s);
    std::string w;
    size_t n = 0;
    while (iss >> w) n++;
    return n;
}

