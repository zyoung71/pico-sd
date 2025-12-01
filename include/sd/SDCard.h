#pragma once

#include <hardware/GPIODevice.h>
#include <util/ArrayAccessor.h>
#include <util/BufferView.h>

#include <hw_config.h>

class SDCard;

class SDCardStream
{
private:
    SDCard* const card;

public:
    SDCardStream(SDCard* card);

    SDCardStream& InsertBuffer(const void* buffer, size_t length);
    SDCardStream& operator<<(const BufferView<void>& buffer);
    SDCardStream& operator<<(const ArrayAccessor<void>& buffer);
    SDCardStream& operator<<(const char* strbuff);
    SDCardStream& operator<<(char* strbuff);
    SDCardStream& operator<<(char c);

    template<typename T>
    SDCardStream& operator<<(const T& item)
    {
        char buff[32];
        if constexpr (std::is_same_v<T, int64_t>)
            snprintf(buff, 32, "%lli", item);
        else if constexpr (std::is_same_v<T, uint64_t>)
            snprintf(buff, 32, "%llu", item);
        else if constexpr (std::is_unsigned_v<T>)
            snprintf(buff, 32, "%u", item);
        else if constexpr (std::is_integral_v<T>)
            snprintf(buff, 32, "%d", item);
        else if constexpr (std::is_floating_point_v<T>)
            snprintf(buff, 32, "%f", item);
        else
            snprintf(buff, 32, "%s", item);

        buff[31] = '\0';
        *this << (char*)buff;

        return *this;
    }

    SDCardStream& ExtractBuffer(void* buffer, size_t length);
    SDCardStream& operator>>(ArrayAccessor<void>& buffer);
};

class SDCard
{
private:
    int _this_inst_idx;

    static SDCard* _insts[8]; // Impossible to wire more than eight. No way.

protected:
    SDCardStream stream;

    sd_card_t card;

    FIL file;
    FATFS fs;
    const char* current_file_path;
    const char* pc_name;
    bool is_file_open;
    bool is_mounted;

public:
    SDCard(const char* pc_name = "0:");
    virtual ~SDCard();

    bool Mount();
    bool Unmount();
    bool OpenFile(const char* file_path, uint8_t permissions_mask);
    bool CloseFile();
    
    bool Seek(uint64_t index);
    bool SeekStart();
    bool SeekEnd();

    uint64_t GetFileSize() const;

    size_t ReadBuffer(void* buffer, size_t max_bytes);
    char ReadCharacter();
    size_t ReadAll(std::unique_ptr<char[]>& buffer);

    size_t WriteBuffer(const void* buffer, size_t max_bytes);
    size_t WriteString(const char* str);
    size_t WriteCharacter(char c);

    size_t AppendBuffer(const void* buffer, size_t max_bytes, bool keep_index = true);
    size_t AppendString(const char* str, bool keep_index = true);
    size_t AppendCharacter(char c, bool keep_index = true);

    bool ClearFile(uint64_t begin_index, uint64_t end_index);
    bool ClearFile(uint64_t begin_index = 0);
    bool DeleteFile(const char* file_path);
    bool DeleteFile();

    SDCardStream& GetStream();

    friend size_t sd_get_num();
    friend sd_card_t* sd_get_by_num(size_t num);
};

class SDCardDetector : public GPIODeviceDebounce
{
private:
    SDCard* card;
    bool auto_mount;

protected:
    void HandleIRQ(uint32_t events_triggered_mask) override;

public:
    SDCardDetector(uint8_t gpio_pin, SDCard* card = nullptr, bool auto_mount = true, void* user_data = nullptr);
};