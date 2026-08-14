#include "SkinConverter.h"
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <stdexcept>
#include <iostream>

std::vector<uint8_t> SkinConverter::pngToSkinData(const std::vector<uint8_t>& pngData,
    int& width, int& height) {
    int channels;
    unsigned char* image = stbi_load_from_memory(
        pngData.data(),
        static_cast<int>(pngData.size()),
        &width,
        &height,
        &channels,
        STBI_rgb_alpha // 强制转换为RGBA
    );

    if (!image) {
        std::cerr << "Failed to load PNG: " << stbi_failure_reason() << std::endl;
        return {};
    }

    std::vector<uint8_t> result(image, image + width * height * 4);
    stbi_image_free(image);

    return result;
}

std::vector<uint8_t> SkinConverter::skinDataToPng(const std::vector<uint8_t>& skinData,
    int width, int height) {
    if (skinData.size() != static_cast<size_t>(width * height * 4)) {
        throw std::invalid_argument("Invalid skin data size for given dimensions");
    }

    std::vector<uint8_t> pngData;

    // 使用stb_image_write将RGBA数据写入内存中的PNG
    stbi_write_png_to_func(
        [](void* context, void* data, int size) {
            auto& buffer = *static_cast<std::vector<uint8_t>*>(context);
            buffer.insert(buffer.end(),
                static_cast<uint8_t*>(data),
                static_cast<uint8_t*>(data) + size);
        },
        &pngData,
        width,
        height,
        4, // RGBA
        skinData.data(),
        width * 4 // stride
    );

    if (pngData.empty()) {
        std::cerr << "Failed to write PNG" << std::endl;
    }

    return pngData;
}

bool SkinConverter::isValidSkinSize(int width, int height) {
    return (width == 64 && height == 32) ||  // 传统皮肤
        (width == 64 && height == 64) ||  // 高清皮肤
        (width == 128 && height == 64) || // 宽皮肤
        (width == 128 && height == 128);  // 最高清皮肤
}