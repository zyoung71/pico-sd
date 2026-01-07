#pragma once

#include <hardware/GPIODevice.h>
#include <storage/StorageDevice.h>

#include <vector>

#include <hw_config.h>

class SDCard : public StorageDevice
{
private:
    static std::vector<SDCard*> _insts;

    static DirectoryEntry GetEntryFromFatFsStat(const FILINFO& info);
    static uint32_t TranslateFileAccessFlags(uint32_t access);

protected:
    sd_card_t card;
    
    mutable DIR directory = {};
    FIL file = {};
    mutable FATFS fs;
    const char* current_file_path;
    const char* pc_name;

public:
    SDCard(const char* pc_name = "");
    virtual ~SDCard();

    FILINFO GetFileStats(const char* path) const;
    FILINFO GetFileStats() const;

    UniqueArray<DirectoryEntry> PeekDirectory(const char* dir_path) const override;
    size_t GetTotalCountInDirectory(const char* dir_path) const override;
    size_t GetFileCountInDirectory(const char* dir_path) const override;
    size_t GetDirectoryCountInDirectory(const char* dir_path) const override;
    DirectoryEntry GetDirectoryEntry(const char* path) const override;

    bool ChangeDirectory(const char* path) override;
    bool CreateDirectory(const char* dir_path) override;
    bool Move(const char* path, const char* new_path); // Move and Rename do the same thing override.
    bool Rename(const char* name, const char* new_name) override;

    bool Mount() override;
    bool Unmount() override;
    bool OpenFile(const char* file_path, uint32_t access_mask) override;
    bool CloseFile() override;
    
    bool Seek(uint64_t index) override;
    bool SeekStart() override;
    bool SeekEnd() override;
    bool SeekStep(int64_t d_idx) override;

    uint64_t GetFileSize(const char* path) const override;
    uint64_t GetFileSize() const override;
    uint64_t GetFreeSpace() const override;
    uint64_t GetTotalSpace() const override;
    float GetSpaceUsedPercentage() const override;

    size_t ReadBuffer(void* buffer, size_t max_bytes) override;
    char ReadCharacter() override;
    size_t ReadAll(UniqueArray<char>& buffer) override;
    size_t ReadLine(UniqueArray<char>& buffer, bool from_start_of_line = false) override;

    size_t WriteBuffer(const void* buffer, size_t max_bytes) override;
    size_t WriteString(const char* str) override;
    size_t WriteCharacter(char c) override;

    size_t AppendBuffer(const void* buffer, size_t max_bytes, bool keep_index = true) override;
    size_t AppendString(const char* str, bool keep_index = true) override;
    size_t AppendCharacter(char c, bool keep_index = true) override;

    int64_t FindNextBuffer(const void* buffer, size_t max_bytes, bool keep_index = true) override;
    int64_t FindNextString(const char* str, bool keep_index = true) override;
    int64_t FindNextCharacter(char c, bool keep_index = true) override;
    int64_t FindPreviousBuffer(const void* buffer, size_t max_bytes, bool keep_index = true) override;
    int64_t FindPreviousString(const char* str, bool keep_index = true) override;
    int64_t FindPreviousCharacter(char c, bool keep_index = true) override;

    bool ClearFile(uint64_t begin_index, uint64_t end_index) override;
    bool ClearFile(uint64_t begin_index = 0) override;

    bool Delete(const char* file_path) override;
    bool Delete() override;

    bool Exists(const char* path) const override;

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
    SDCardDetector(uint8_t gpio_pin, SDCard* card = nullptr, bool auto_mount = true);

    inline void SetCard(SDCard* card)
    {
        this->card = card;
    }
};