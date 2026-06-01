#pragma once

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "esp_vfs_fat.h"
#include "spi_device.h"

#define MATRIX_WIDTH 36
#define MATRIX_HEIGHT 16
#define MATRIX_PIXELS (MATRIX_WIDTH * MATRIX_HEIGHT)
#define FRAME_BUF_SIZE (MATRIX_PIXELS * 3)

class SDCard : public SPIDevice
{
   public:
    explicit SDCard(spi_host_device_t host, gpio_num_t cs_pin);

    bool Init() override;

    void Deinit() override;

    bool IsMounted() const { return mounted_; }

    // 基础文件操作
    bool FileExists(const char* path);
    size_t GetFileSize(const char* path);
    bool ReadFile(const char* path, uint8_t* buffer, size_t len, size_t offset = 0);
    bool WriteFile(const char* path, const uint8_t* buffer, size_t len);
    bool AppendFile(const char* path, const uint8_t* buffer, size_t len);
    void ListDirectory(const char* path);

    // 字模读取
    bool ReadFontChar(
        const char* fontPath, char ch, char startChar, uint8_t fontW, uint8_t fontH, uint8_t* outBuf);

    // 静态图片/单帧读取
    bool ReadImageFrame(const char* path, uint8_t* outBuf, size_t bufSize);

    // 动图：合并文件格式
    bool ReadAnimFramePacked(const char* animPath, int frameIdx, uint8_t* outBuf, size_t bufSize);
    int GetAnimFrameCountPacked(const char* animPath);

    // 动图：分散文件格式
    bool ReadAnimFrameByIndex(const char* folder, int frameIdx, uint8_t* outBuf, size_t bufSize);

   private:
    sdmmc_card_t* sdcard_ = nullptr;
    bool mounted_ = false;
};
