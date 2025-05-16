#include "wirehair.h"
#include <stdexcept>
#include <print>
#include <vector>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/val.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT
#endif

extern "C" {

EXPORT
WirehairCodec createDecoder(uint64_t messageByte, uint32_t blockBytes)
{
    static bool initWirehair = [] {
        const WirehairResult initResult = wirehair_init();
        if (initResult != Wirehair_Success) {
            throw std::runtime_error("Wirehair initialization failed");
        }
        return true;
    }();
    return wirehair_decoder_create(nullptr, messageByte, blockBytes);
}

EXPORT
WirehairResult decode(WirehairCodec decoder, unsigned blockId, const void* blockData, uint32_t blockSize)
{
    return wirehair_decode(decoder, blockId, blockData, blockSize);
}

EXPORT
uint8_t* getDecodedData(WirehairCodec decoder, uint64_t size)
{
    uint8_t* data = (uint8_t*)malloc(size);
    if (!data) {
        throw std::runtime_error("Failed to allocate memory");
    }
    
    auto res = wirehair_recover(decoder, data, size);
    if (res != Wirehair_Success) {
        free(data);
        throw std::runtime_error("Failed to recover data");
    }
    return data;
}

EXPORT
void destroyDecoder(WirehairCodec decoder) {
    wirehair_free(decoder);
}

EXPORT
void *allocData(size_t dataSize)
{
    return malloc(dataSize);
}

EXPORT
void freeDecodedData(uint8_t* data) {
    free(data);
}

EXPORT
void testFunc()
{
    std::println("Hello world");
}

} // extern "C"
