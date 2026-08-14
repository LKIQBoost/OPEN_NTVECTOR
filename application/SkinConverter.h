#pragma once
#include <vector>
#include <cstdint>

class SkinConverter {
public:
    // 禁用实例化
    SkinConverter() = delete;

    /**
     * 将PNG数据转换为SkinData (RGBA格式)
     * @param pngData PNG文件的二进制数据
     * @param width [out] 输出图像的宽度
     * @param height [out] 输出图像的高度
     * @return SkinData的RGBA字节数据，如果失败返回空vector
     */
    static std::vector<uint8_t> pngToSkinData(const std::vector<uint8_t>& pngData,
        int& width, int& height);

    /**
     * 将SkinData转换为PNG数据
     * @param skinData RGBA格式的SkinData
     * @param width 图像的宽度
     * @param height 图像的高度
     * @return PNG文件的二进制数据，如果失败返回空vector
     */
    static std::vector<uint8_t> skinDataToPng(const std::vector<uint8_t>& skinData,
        int width, int height);

    /**
     * 检查是否为有效的Minecraft皮肤尺寸
     */
    static bool isValidSkinSize(int width, int height);
};

