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

void initWirehair()
{
    static bool initWirehair = [] {
        const WirehairResult initResult = wirehair_init();
        if (initResult != Wirehair_Success) {
            throw std::runtime_error("Wirehair initialization failed");
        }
        return true;
    }();
}

extern "C" {

EXPORT
WirehairCodec createDecoder(uint64_t messageByte, uint32_t blockBytes)
{
    initWirehair();
    return wirehair_decoder_create(nullptr, messageByte, blockBytes);
}

EXPORT
WirehairCodec createEncoder(const void* message, uint64_t messageByte, uint32_t blockBytes)
{
    initWirehair();
    return wirehair_encoder_create(nullptr, message, messageByte, blockBytes);
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
void destroyCoder(WirehairCodec coder) {
    wirehair_free(coder);
}

EXPORT
void *allocData(size_t dataSize)
{
    return malloc(dataSize);
}

EXPORT
void freeData(void* data) {
    free(data);
}

EXPORT
void testFunc()
{
    std::println("Hello world");
}

} // extern "C"
