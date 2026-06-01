#include "sd_card.h"

static const char* TAG = "SDCard";

SDCard::SDCard(spi_host_device_t host, gpio_num_t cs_pin) : SPIDevice(host, cs_pin, TAG)
{
}

bool SDCard::Init()
{
    const sdmmc_host_t host_cfg = SDSPI_HOST_DEFAULT();

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.host_id = host_;
    slot_config.gpio_cs = cs_pin_;

    esp_vfs_fat_mount_config_t mount_config = {};
    mount_config.format_if_mount_failed = false;
    mount_config.max_files = 5;
    mount_config.allocation_unit_size = 0;
    mount_config.disk_status_check_enable = false;

    esp_err_t ret = esp_vfs_fat_sdspi_mount("/sdcard", &host_cfg, &slot_config, &mount_config, &sdcard_);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "SD card mount failed (CS=%d)", cs_pin_);
        mounted_ = false;
        return false;
    }

    mounted_ = true;
    ESP_LOGI(TAG, "SD card mounted (CS=%d)", cs_pin_);

    ListDirectory("/sdcard");
    return true;
}

void SDCard::Deinit()
{
    if (mounted_)
    {
        esp_vfs_fat_sdcard_unmount("/sdcard", sdcard_);
        sdcard_ = nullptr;
        mounted_ = false;
    }
}

bool SDCard::FileExists(const char* path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

size_t SDCard::GetFileSize(const char* path)
{
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return st.st_size;
}

bool SDCard::ReadFile(const char* path, uint8_t* buffer, size_t len, size_t offset)
{
    if (!mounted_) return false;
    FILE* f = fopen(path, "rb");
    if (!f)
    {
        ESP_LOGE(TAG, "Failed to open: %s", path);
        return false;
    }
    if (offset > 0) fseek(f, offset, SEEK_SET);
    size_t read = fread(buffer, 1, len, f);
    fclose(f);
    if (read != len)
    {
        ESP_LOGW(TAG, "Read short: %s, expected %zu, got %zu", path, len, read);
        return false;
    }
    return true;
}

bool SDCard::WriteFile(const char* path, const uint8_t* buffer, size_t len)
{
    if (!mounted_) return false;
    FILE* f = fopen(path, "wb");
    if (!f)
    {
        ESP_LOGE(TAG, "Failed to create: %s", path);
        return false;
    }
    size_t written = fwrite(buffer, 1, len, f);
    fclose(f);
    return (written == len);
}

bool SDCard::AppendFile(const char* path, const uint8_t* buffer, size_t len)
{
    if (!mounted_) return false;
    FILE* f = fopen(path, "ab");
    if (!f)
    {
        ESP_LOGE(TAG, "Failed to append: %s", path);
        return false;
    }
    size_t written = fwrite(buffer, 1, len, f);
    fclose(f);
    return (written == len);
}

void SDCard::ListDirectory(const char* path)
{
    if (!mounted_) return;
    DIR* dir = opendir(path);
    if (!dir)
    {
        ESP_LOGE(TAG, "Failed to open dir: %s", path);
        return;
    }
    struct dirent* entry;
    ESP_LOGI(TAG, "Directory: %s", path);
    while ((entry = readdir(dir)) != NULL)
    {
        ESP_LOGI(TAG, "  %s", entry->d_name);
    }
    closedir(dir);
}

bool SDCard::ReadFontChar(const char* fontPath, char ch, char startChar, uint8_t fontW, uint8_t fontH, uint8_t* outBuf)
{
    if (!mounted_) return false;
    size_t bytesPerChar = (fontW * fontH + 7) / 8;
    int idx = (int)ch - (int)startChar;
    if (idx < 0) return false;
    size_t offset = (size_t)idx * bytesPerChar;

    FILE* f = fopen(fontPath, "rb");
    if (!f) return false;
    fseek(f, offset, SEEK_SET);
    size_t read = fread(outBuf, 1, bytesPerChar, f);
    fclose(f);
    return (read == bytesPerChar);
}

bool SDCard::ReadImageFrame(const char* path, uint8_t* outBuf, size_t bufSize)
{
    if (bufSize < FRAME_BUF_SIZE)
    {
        ESP_LOGE(TAG, "Buffer too small, need %d bytes", FRAME_BUF_SIZE);
        return false;
    }
    return ReadFile(path, outBuf, FRAME_BUF_SIZE, 0);
}

bool SDCard::ReadAnimFramePacked(const char* animPath, int frameIdx, uint8_t* outBuf, size_t bufSize)
{
    if (!mounted_) return false;
    FILE* f = fopen(animPath, "rb");
    if (!f) return false;

    uint32_t frameCount = 0, frameSize = 0;
    fread(&frameCount, 4, 1, f);
    fread(&frameSize, 4, 1, f);
    fseek(f, 8, SEEK_CUR);

    if (frameIdx >= (int)frameCount || bufSize < frameSize)
    {
        fclose(f);
        return false;
    }

    size_t offset = 16 + (size_t)frameIdx * frameSize;
    fseek(f, offset, SEEK_SET);
    size_t read = fread(outBuf, 1, frameSize, f);
    fclose(f);
    return (read == frameSize);
}

int SDCard::GetAnimFrameCountPacked(const char* animPath)
{
    if (!mounted_) return 0;
    FILE* f = fopen(animPath, "rb");
    if (!f) return 0;
    uint32_t frameCount = 0;
    fread(&frameCount, 4, 1, f);
    fclose(f);
    return (int)frameCount;
}

bool SDCard::ReadAnimFrameByIndex(const char* folder, int frameIdx, uint8_t* outBuf, size_t bufSize)
{
    if (bufSize < FRAME_BUF_SIZE) return false;
    char path[80];
    snprintf(path, sizeof(path), "%s/%03d.bin", folder, frameIdx);
    return ReadImageFrame(path, outBuf, bufSize);
}
