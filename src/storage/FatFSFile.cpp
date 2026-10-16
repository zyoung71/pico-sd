#include "ff.h"
#include <storage/FatFSFile.hpp>

#include <cstring>

bool FatFSFile::Seek(uint64_t index)
{
    uint64_t size = f_size(&file);
    index = index > size ? size : index; // clamp to end if index too high
    return f_lseek(&file, index) == FR_OK;
}

bool FatFSFile::SeekStart()
{
    return f_lseek(&file, 0) == FR_OK;
}

bool FatFSFile::SeekEnd()
{
    return f_lseek(&file, f_size(&file)) == FR_OK;
}

bool FatFSFile::SeekStep(int64_t d_idx)
{
    return f_lseek(&file, f_tell(&file) + d_idx) == FR_OK;
}

uint64_t FatFSFile::GetFileSize() const
{
    return f_size(&file);
}

size_t FatFSFile::ReadBuffer(void* buffer, size_t max_bytes)
{
    UINT bytes_read = 0;
    f_read(&file, buffer, max_bytes, &bytes_read);
    return bytes_read;
}

char FatFSFile::ReadCharacter()
{
    char c = '\0';
    UINT bytes_read;
    f_read(&file, &c, 1, &bytes_read);
    return c;
}

size_t FatFSFile::ReadAll(UniqueArray<char>& buffer)
{

    UINT bytes_read = 0;
    uint64_t size = f_size(&file);
    buffer.array = std::make_unique<char[]>(size);
    f_read(&file, buffer.array.get(), size, &bytes_read);
    return bytes_read;

}

size_t FatFSFile::ReadLine(UniqueArray<char>& buffer, bool from_start_of_line)
{

    if (from_start_of_line)
    {
        int64_t prev_line_end = FindPreviousCharacter('\n');
        f_lseek(&file, prev_line_end + 1);
    }
    char* buff = f_gets(buffer.array.get(), 4096, &file);
    size_t bytes_read = strlen(buff);
    buffer.length = bytes_read + 1;
    return bytes_read;
}

size_t FatFSFile::WriteBuffer(const void* buffer, size_t max_bytes)
{
    UINT bytes_written = 0;
    f_write(&file, buffer, max_bytes, &bytes_written);
    return bytes_written;
}

size_t FatFSFile::WriteString(const char* strbuff)
{
    f_puts(strbuff, &file);
    return strlen(strbuff) + 1;
}

size_t FatFSFile::WriteCharacter(char c)
{

    f_putc(c, &file);
    return 1;
}

size_t FatFSFile::AppendBuffer(const void* buffer, size_t max_bytes, bool keep_index)
{
    UINT bytes_written = 0;
    uint64_t prev_pos = f_tell(&file);
    f_lseek(&file, f_size(&file));
    f_write(&file, buffer, max_bytes, &bytes_written);
    return bytes_written;
}

size_t FatFSFile::AppendString(const char* strbuff, bool keep_index)
{
    UINT bytes_written = 0;
    uint64_t prev_pos = f_tell(&file);
    f_lseek(&file, f_size(&file));
    f_puts(strbuff, &file);
    if (keep_index)
        f_lseek(&file, prev_pos);
    
    return bytes_written;
}

size_t FatFSFile::AppendCharacter(char c, bool keep_index)
{

    UINT bytes_written = 0;
    uint64_t prev_pos = f_tell(&file);
    f_lseek(&file, f_size(&file));
    f_putc(c, &file);
    if (keep_index)
        f_lseek(&file, prev_pos);

    return bytes_written;
}

int64_t FatFSFile::FindNextBuffer(const void* buffer, size_t max_bytes, bool keep_index)
{
    uint64_t loc = f_tell(&file);

    uint8_t buffer_cmp[max_bytes];
    UINT bytes_read;
    f_read(&file, buffer_cmp, max_bytes, &bytes_read); // fill buffer, will be right at the prev fileptr
    SeekStep(1 - bytes_read); // trace back to original position and shift over one
    while (1)
    {
        uint8_t b;
        f_read(&file, &b, 1, &bytes_read); // read byte
        memmove(buffer_cmp, buffer_cmp + 1, max_bytes); // shift buffer to the left
        buffer_cmp[max_bytes - 1] = b;
        if (memcmp(buffer, buffer_cmp, max_bytes) != 0)
        {
            if (f_eof(&file))
            {
                return -1; // not found
            }
        }
        else
            break;
    }

    SeekStep(-max_bytes); // move back to beginning of buffer
    uint64_t found = f_tell(&file);
    if (keep_index)
        f_lseek(&file, loc); // go back to original position
    return found;
}

int64_t FatFSFile::FindNextString(const char* str, bool keep_index)
{
    uint64_t loc = f_tell(&file);

    size_t len = strlen(str); // no plus one, as f_read does not add a null terminator
    char str_cmp[len];
    UINT bytes_read;
    f_read(&file, str_cmp, len, &bytes_read);
    SeekStep(1 - bytes_read);
    while (1)
    {
        char c;
        f_read(&file, &c, 1, &bytes_read);
        memmove(str_cmp, str_cmp + 1, len);
        str_cmp[len - 1] = c;

        if (strncmp(str, str_cmp, len) != 0)
        {
            if (f_eof(&file))
                return -1;
        }
        else
            break;
    }

    SeekStep(-len);
    uint64_t found = f_tell(&file);
    if (keep_index)
        f_lseek(&file, loc);
    return found;
}

int64_t FatFSFile::FindNextCharacter(char c, bool keep_index)
{
    uint64_t loc = f_tell(&file);
        
    UINT bytes_read;
    SeekStep(1);
    while (1)
    {
        char c_read;
        f_read(&file, &c_read, 1, &bytes_read);

        if (c_read != c)
        {
            if (f_eof(&file))
                return -1;
        }
        else
            break;
    }

    SeekStep(-1);
    uint64_t found = f_tell(&file);
    if (keep_index)
        f_lseek(&file, loc);
    return found;
}

int64_t FatFSFile::FindPreviousBuffer(const void* buffer, size_t max_bytes, bool keep_index)
{
    uint64_t loc = f_tell(&file);
        
    uint8_t buffer_cmp[max_bytes];
    UINT bytes_read;
    f_read(&file, buffer_cmp, max_bytes, &bytes_read); // fill the buffer first (will be right at the fptr)
    SeekStep(-bytes_read - 1); // move back to original and one additional position to read individual chars in loop
    while (1)
    {
        uint8_t b;
        f_read(&file, &b, 1, &bytes_read);
        memmove(buffer_cmp + 1, buffer_cmp, max_bytes); // shift buffer once to the right
        buffer_cmp[0] = b;
        SeekStep(-1); // f_read contradicts. moves it back where it was before read
        
        if (memcmp(buffer, buffer_cmp, max_bytes) != 0)
        {
            if (f_tell(&file) == 0)
                return -1;
                
            SeekStep(-1); // after a check for zero, move back again for reading before the last
        }
        else
            break;
    }

    uint64_t found = f_tell(&file);
    if (keep_index)
        f_lseek(&file, loc);
    return found;
}

int64_t FatFSFile::FindPreviousString(const char* str, bool keep_index)
{
    uint64_t loc = f_tell(&file);

    size_t len = strlen(str);
    char str_cmp[len];
    UINT bytes_read;
    f_read(&file, str_cmp, len, &bytes_read);
    SeekStep(-bytes_read - 1);
    while (1)
    {
        char c;
        f_read(&file, &c, 1, &bytes_read);
        memmove(str_cmp + 1, str_cmp, len);
        str_cmp[0] = c;
        SeekStep(-1);

        if (strncmp(str, str_cmp, len) != 0)
        {
            if (f_tell(&file) == 0)
                return -1;

            SeekStep(-1);
        }
        else
            break;
    }
    uint64_t found = f_tell(&file);
    f_lseek(&file, loc);
    return found;
}

int64_t FatFSFile::FindPreviousCharacter(char c, bool keep_index)
{
    uint64_t loc = f_tell(&file);

    UINT bytes_read;
    SeekStep(-1);
    while (1)
    {
        char c_cmp;
        f_read(&file, &c_cmp, 1, &bytes_read);
        SeekStep(-1);

        if (c != c_cmp)
        {
            if (f_tell(&file) == 0)
                return -1;

            SeekStep(-1);
        }
        else
            break;
    }
    uint64_t found = f_tell(&file);
    if (keep_index)
        f_lseek(&file, loc);
    return found;
}

bool FatFSFile::ClearFile(uint64_t begin_index, uint64_t end_index)
{
    uint64_t size = f_size(&file);
    end_index = end_index > size ? size : end_index;
        
    uint64_t prev_pos = f_tell(&file);
    f_lseek(&file, begin_index);

    if (end_index == size)
        return f_truncate(&file) == FR_OK;

    size_t diff = size - end_index;
    char buff[diff];
    UINT bytes_read;
    f_read(&file, buff, diff, &bytes_read);

    f_lseek(&file, begin_index);
    FRESULT result = f_truncate(&file);

    UINT bytes_written;
    f_write(&file, buff, diff, &bytes_written);

    return result == FR_OK;
}

bool FatFSFile::ClearFile(uint64_t begin_index)
{
    uint64_t prev_pos = f_tell(&file);
    f_lseek(&file, begin_index);
    FRESULT result = f_truncate(&file);

    if (prev_pos > begin_index) // if previous index was in a spot just deleted
        f_lseek(&file, f_size(&file));
    else
        f_lseek(&file, prev_pos);

    return result == FR_OK;
}

bool FatFSFile::IsFileOpen() const
{
    return file.obj.fs != nullptr;
}

void FatFSFile::Close()
{
    f_close(&file);
}