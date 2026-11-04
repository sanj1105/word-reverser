// WebAssembly entry point: exposes the C++ reversal logic to JavaScript.
// Built with Emscripten by .github/workflows/pages.yml
#include <string>

#include "../backend/reverse.h"

extern "C" {

static std::string g_out;

// mode: 0 = word order, 1 = each word, 2 = whole text
const char* reverse_text(const char* text, int mode) {
    std::string in(text ? text : "");
    g_out = mode == 0 ? reverseWordOrder(in)
          : mode == 1 ? reverseEachWord(in)
                      : reverseChars(in);
    return g_out.c_str();
}

int count_words(const char* text) {
    return static_cast<int>(countWords(text ? text : ""));
}

}  // extern "C"
