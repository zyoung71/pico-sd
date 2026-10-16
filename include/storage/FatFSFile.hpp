#pragma once

#include <ff.h>

#include <util/SPIDevice.hpp>
#include <storage/File.hpp>

class SDCard;

class FatFSFile : public File
{
protected:
    FIL file;
    const SPIDevice* const spi_device;

public:
    FatFSFile(const SPIDevice* spi_device = nullptr) : spi_device(spi_device) {}
    
    bool Seek(uint64_t index) override;
    bool SeekStart() override;
    bool SeekEnd() override;
    bool SeekStep(int64_t d_idx) override;
    
    virtual uint64_t GetFileSize() const override;

    virtual size_t ReadBuffer(void* buffer, size_t max_bytes) override;
    virtual char ReadCharacter() override;
    virtual size_t ReadAll(UniqueArray<char>& buffer) override;
    virtual size_t ReadLine(UniqueArray<char>& buffer, bool from_start_of_line = false) override;

    virtual size_t WriteBuffer(const void* buffer, size_t max_bytes) override;
    virtual size_t WriteString(const char* str) override;
    virtual size_t WriteCharacter(char c) override;

    virtual size_t AppendBuffer(const void* buffer, size_t max_bytes, bool keep_index = true) override;
    virtual size_t AppendString(const char* str, bool keep_index = true) override;
    virtual size_t AppendCharacter(char c, bool keep_index = true) override;

    virtual int64_t FindNextBuffer(const void* buffer, size_t max_bytes, bool keep_index = true) override;
    virtual int64_t FindNextString(const char* str, bool keep_index = true) override;
    virtual int64_t FindNextCharacter(char c, bool keep_index = true) override;

    virtual int64_t FindPreviousBuffer(const void* buffer, size_t max_bytes, bool keep_index = true) override;
    virtual int64_t FindPreviousString(const char* str, bool keep_index = true) override;
    virtual int64_t FindPreviousCharacter(char c, bool keep_index = true) override;

    virtual bool ClearFile(uint64_t begin_index, uint64_t end_index) override;
    virtual bool ClearFile(uint64_t begin_index = 0) override;

    virtual void Close() override;

    virtual bool IsFileOpen() const override;

    friend SDCard;
};