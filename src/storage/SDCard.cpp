#include <storage/SDCard.h>

#include <algorithm>

std::vector<SDCard*> SDCard::_insts = std::vector<SDCard*>();

DirectoryEntry SDCard::GetEntryFromFatFsStat(const FILINFO& info)
{
    DirectoryEntry entry;
    strncpy(entry.name, info.fname, 256);
    entry.date_modified = info.fdate;
    entry.time_modified = info.ftime;
    entry.is_readonly = info.fattrib & AM_RDO;
    entry.is_hidden = info.fattrib & AM_HID;
    entry.is_system = info.fattrib & AM_SYS;
    entry.is_archive = info.fattrib & AM_ARC;
    entry.is_directory = info.fattrib & AM_DIR;

    return entry;
}

uint32_t SDCard::TranslateFileAccessFlags(uint32_t access)
{
    uint32_t mask = 0;
    if (access & READ)
        mask |= FA_READ;
    if (access & WRITE)
        mask |= FA_WRITE;
    if (access & OPEN_EXISTING)
        mask |= FA_OPEN_EXISTING;
    if (access & OPEN_OVERWRITE)
        mask |= FA_OPEN_ALWAYS;
    if (access & OPEN_APPEND)
        mask |= FA_OPEN_APPEND;
    if (access & CREATE_NEW)
        mask |= FA_CREATE_NEW;
    if (access & CREATE_OVERWRITE)
        mask |= FA_CREATE_ALWAYS;
    return mask;
}

SDCard::SDCard(const char* pc_name)
    : StorageDevice(), pc_name(pc_name), current_file_path(nullptr)
{
    _insts.push_back(this);
}

SDCard::~SDCard()
{
    CloseFile();
    Unmount();

    auto sd_card_end = std::remove(_insts.begin(), _insts.end(), this);

    _insts.erase(sd_card_end);
}

UniqueArray<DirectoryEntry> SDCard::PeekDirectory(const char* dir_path)
{
    
    size_t count = 0;
    if (f_opendir(&directory, dir_path) != FR_OK)
        return nullptr;

    FILINFO file_info;
    while (f_readdir(&directory, &file_info) == FR_OK)
    {
        if (file_info.fname[0] == 0)
            break;
        
        count++;
    }

    UniqueArray<DirectoryEntry> ret = make_unique_array_empty<DirectoryEntry>(count);

    count = 0;
    f_rewinddir(&directory);
    while (f_readdir(&directory, &file_info) == FR_OK)
    {
        if (file_info.fname[0] == 0)
            break;
        
        ret[count++] = GetEntryFromFatFsStat(file_info);
    }
    return std::move(ret);
}

size_t SDCard::GetTotalCountInDirectory(const char* dir_path) 
{
    
    size_t count = 0;
    if (f_opendir(&directory, dir_path) != FR_OK)
        return 0;
    
    FILINFO f;
    while (f_readdir(&directory, &f) == FR_OK)
    {
        if (f.fname[0] == 0)
            break;
        count++;
    }
    return count;
}

size_t SDCard::GetFileCountInDirectory(const char* dir_path)
{
    
    size_t count = 0;
    if (f_opendir(&directory, dir_path) != FR_OK)
        return 0;
    
    FILINFO f;
    while (f_readdir(&directory, &f) == FR_OK)
    {
        if (f.fname[0] == 0)
            break;
        
        if (f.fattrib & AM_DIR)
            continue;
        
        count++;
    }
    return count;
}

size_t SDCard::GetDirectoryCountInDirectory(const char* dir_path)
{
    size_t count = 0;
    if (f_opendir(&directory, dir_path) != FR_OK)
        return 0;
    
    FILINFO f;
    while (f_readdir(&directory, &f) == FR_OK)
    {
        if (f.fname[0] == 0)
            break;
        
        if (f.fattrib & AM_DIR)
            count++;
    }
    f_closedir(&directory);
    return count;
}

DirectoryEntry SDCard::GetDirectoryEntry(const char* path)
{
    FILINFO f;
    if (f_stat(path, &f) == FR_OK)
    {
        return GetEntryFromFatFsStat(f);
    }
    return {};
}

bool SDCard::ChangeDirectory(const char* path)
{
    return f_chdir(path) == FR_OK;
}

bool SDCard::CreateDirectory(const char* dir_path)
{
    return f_mkdir(dir_path) == FR_OK;
}

bool SDCard::Move(const char* path, const char* new_path)
{
    return f_rename(path, new_path) == FR_OK;
}

bool SDCard::Rename(const char* name, const char* new_name)
{
    return f_rename(name, new_name) == FR_OK;
}

bool SDCard::Mount()
{
    if (is_mounted)
        return false;

    is_mounted = true;
    return f_mount(&fs, pc_name, 1) == FR_OK;
}

bool SDCard::Unmount()
{
    if (is_mounted)
    {
        is_mounted = false;
        return f_unmount(pc_name) == FR_OK;
    }

    return false; 
}

bool SDCard::OpenFile(const char* file_path, uint32_t access_mask)
{
    if (is_file_open)
        f_close(&file);
    
    is_file_open = true;
    current_file_path = file_path;
    return f_open(&file, file_path, TranslateFileAccessFlags(access_mask)) == FR_OK;
}

bool SDCard::CloseFile()
{
    if (is_file_open)
    {
        is_file_open = false;
        return f_close(&file) == FR_OK;
    }
    return false;
}

bool SDCard::Seek(uint64_t index)
{
    if (is_file_open)
    {
        uint64_t size = f_size(&file);
        index = index > size ? size : index; // clamp to end if index too high
        return f_lseek(&file, index) == FR_OK;
    }
    return false;
}

bool SDCard::SeekStart()
{
    if (is_file_open)
    {
        return f_lseek(&file, 0) == FR_OK;
    }
    return false;
}

bool SDCard::SeekEnd()
{
    if (is_file_open)
    {
        return f_lseek(&file, f_size(&file)) == FR_OK;
    }
    return false;
}

bool SDCard::SeekStep(int64_t d_idx)
{
    if (is_file_open)
    {
        return f_lseek(&file, f_tell(&file) + d_idx) == FR_OK;
    }
    return false;
}

uint64_t SDCard::GetFileSize(const char* path) const
{
    return GetFileStats(path).fsize;
}

uint64_t SDCard::GetFileSize() const
{
    if (is_file_open)
        return f_size(&file);

    return GetFileSize(current_file_path);
}

uint64_t SDCard::GetFreeSpace() const
{
    DWORD free_clusters;
    auto addr = &fs;

    if (f_getfree(pc_name, &free_clusters, &addr) == FR_OK)
    {
        return free_clusters * fs.csize * FF_MIN_SS; // FF_MIN_SS should be 512
    }
    return 0;
}

uint64_t SDCard::GetTotalSpace() const
{
    return (fs.n_fatent - 2) * fs.csize;
}

float SDCard::GetSpaceUsedPercentage() const
{
    return (GetFreeSpace() / (float)GetTotalSpace()) * 100.f;
}

FILINFO SDCard::GetFileStats(const char* path) const
{
    FILINFO inf;
    f_stat(path, &inf);
    return inf;
}

FILINFO SDCard::GetFileStats() const
{
    return GetFileStats(current_file_path);
}

size_t SDCard::ReadBuffer(void* buffer, size_t max_bytes)
{
    if (is_file_open)
    {
        UINT bytes_read;
        f_read(&file, buffer, max_bytes, &bytes_read);
        return bytes_read;
    }
    return 0;
}

char SDCard::ReadCharacter()
{
    if (is_file_open)
    {
        char c;
        UINT bytes_read;
        f_read(&file, &c, 1, &bytes_read);
        return c;
    }
    return '\0';
}

size_t SDCard::ReadAll(UniqueArray<char>& buffer)
{
    if (is_file_open)
    {
        UINT bytes_read;
        uint64_t size = f_size(&file);
        buffer.array = std::make_unique<char[]>(size);
        f_read(&file, buffer.array.get(), size, &bytes_read);
        return bytes_read;
    }
    return 0;
}

size_t SDCard::ReadLine(UniqueArray<char>& buffer, bool from_start_of_line)
{
    if (is_file_open)
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
    return 0;
}

size_t SDCard::WriteBuffer(const void* buffer, size_t max_bytes)
{
    if (is_file_open)
    {
        UINT bytes_written;
        f_write(&file, buffer, max_bytes, &bytes_written);
        return bytes_written;
    }
    return 0;
}

size_t SDCard::WriteString(const char* strbuff)
{
    if (is_file_open)
    {
        f_puts(strbuff, &file);
        return strlen(strbuff) + 1;
    }
    return 0;
}

size_t SDCard::WriteCharacter(char c)
{
    if (is_file_open)
    {
        f_putc(c, &file);
        return 1;
    }
    return 0;
}

size_t SDCard::AppendBuffer(const void* buffer, size_t max_bytes, bool keep_index)
{
    if (is_file_open)
    {
        UINT bytes_written;
        uint64_t prev_pos = f_tell(&file);
        f_lseek(&file, f_size(&file));
        f_write(&file, buffer, max_bytes, &bytes_written);
        return bytes_written;
    }
    return 0;
}

size_t SDCard::AppendString(const char* strbuff, bool keep_index)
{
    if (is_file_open)
    {
        UINT bytes_written;
        uint64_t prev_pos = f_tell(&file);
        f_lseek(&file, f_size(&file));
        f_puts(strbuff, &file);
        if (keep_index)
            f_lseek(&file, prev_pos);
        
        return bytes_written;
    }
    return 0;
}

size_t SDCard::AppendCharacter(char c, bool keep_index)
{
    if (is_file_open)
    {
        UINT bytes_written;
        uint64_t prev_pos = f_tell(&file);
        f_lseek(&file, f_size(&file));
        f_putc(c, &file);
        if (keep_index)
            f_lseek(&file, prev_pos);

        return bytes_written;
    }
    return 0;
}

int64_t SDCard::FindNextBuffer(const void* buffer, size_t max_bytes)
{
    if (is_file_open)
    {
        uint64_t loc = f_tell(&file);

        uint8_t buffer_cmp[max_bytes];
        UINT bytes_read;
        SeekStep(1);
        f_read(&file, buffer_cmp, max_bytes, &bytes_read);
        while ((memcmp(buffer, buffer_cmp, max_bytes) != 0) && !f_eof(&file))
        {
            uint8_t b;
            f_read(&file, &b, 1, &bytes_read);
            memmove(buffer_cmp, buffer_cmp + 1, max_bytes);
            buffer_cmp[max_bytes - 1] = b;
        }
        uint64_t found = f_tell(&file);
        f_lseek(&file, loc); // go back to original position
        return found;
    }
    return -1;
}

int64_t SDCard::FindNextString(const char* str)
{
    if (is_file_open)
    {
        uint64_t loc = f_tell(&file);

        size_t len = strlen(str); // no plus one, as f_read does not add a null terminator
        char str_cmp[len];
        UINT bytes_read;
        SeekStep(1);
        f_read(&file, str_cmp, len, &bytes_read);
        while ((strncmp(str_cmp, str, len) != 0) && !f_eof(&file))
        {
            char c;
            f_read(&file, &c, 1, &bytes_read);
            memmove(str_cmp, str_cmp + 1, len);
            str_cmp[len - 1] = c;
        }
        uint64_t found = f_tell(&file);
        f_lseek(&file, loc);
        return found;
    }
    return -1;
}

int64_t SDCard::FindNextCharacter(char c)
{
    if (is_file_open)
    {
        uint64_t loc = f_tell(&file);
        
        char c_cmp;
        UINT bytes_read;
        SeekStep(1);
        f_read(&file, &c_cmp, 1, &bytes_read);
        while ((c_cmp != c) && !f_eof(&file))
        {
            char c_read;
            f_read(&file, &c_read, 1, &bytes_read);
            c_cmp = c_read;
        }
    }
    return -1;
}

int64_t SDCard::FindPreviousBuffer(const void* buffer, size_t max_bytes)
{
    if (is_file_open)
    {
        uint64_t loc = f_tell(&file);
        
        uint8_t buffer_cmp[max_bytes];
        UINT bytes_read;
        f_read(&file, buffer_cmp, max_bytes, &bytes_read); // fill the buffer first (will be right at the fptr)
        SeekStep(-max_bytes - 1); // move back to original and one additional position to read individual chars in loop
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
        f_lseek(&file, loc);
        return found;
    }
    return -1;
}

int64_t SDCard::FindPreviousString(const char* str)
{
    if (is_file_open)
    {
        uint64_t loc = f_tell(&file);

        size_t len = strlen(str);
        char str_cmp[len];
        UINT bytes_read;
        f_read(&file, str_cmp, len, &bytes_read);
        SeekStep(-len - 1);
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
    return -1;
}

int64_t SDCard::FindPreviousCharacter(char c)
{
    if (is_file_open)
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
        f_lseek(&file, loc);
        return found;
    }
    return -1;
}

bool SDCard::ClearFile(uint64_t begin_index, uint64_t end_index)
{
    if (is_file_open)
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

        return result;
    }
    return false;
}

bool SDCard::ClearFile(uint64_t begin_index)
{
    if (is_file_open)
    {
        uint64_t prev_pos = f_tell(&file);
        f_lseek(&file, begin_index);
        f_truncate(&file);

        if (prev_pos > begin_index) // if previous index was in a spot just deleted
            f_lseek(&file, f_size(&file));
        else
            f_lseek(&file, prev_pos);
    }
    return false;
}

bool SDCard::Delete(const char* file_path)
{
    if (strcmp(current_file_path, file_path) == 0)
    {
        f_close(&file);
        is_file_open = false;
        return f_unlink(current_file_path) == FR_OK;
    }
    return f_unlink(file_path) == FR_OK;
}

bool SDCard::Delete()
{
    if (is_file_open)
    {
        f_close(&file);
        is_file_open = false;
    }
    return f_unlink(current_file_path) == FR_OK;
}

bool SDCard::Exists(const char* path) const
{
    FILINFO info;
    return f_stat(path, &info) == FR_OK;
}

size_t sd_get_num()
{
    return SDCard::_insts.size();
}

sd_card_t* sd_get_by_num(size_t num)
{
    if (num < sd_get_num())
        return &SDCard::_insts[num]->card;
    return nullptr;
}

SDCardDetector::SDCardDetector(uint8_t gpio_pin, SDCard* card, bool auto_mount)
: GPIODeviceDebounce(gpio_pin, Pull::UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 100), card(card), auto_mount(auto_mount)
{
}

void SDCardDetector::HandleIRQ(uint32_t events_triggered_mask)
{
    if (event_mask & events_triggered_mask)
    {
        if (debouncer.Allow())
        {
            Event* ev = new GPIOEvent(this, events_triggered_mask);
            queue_try_add(&Event::event_queue, &ev);
            if (card && auto_mount)
            {
                if (events_triggered_mask & GPIO_IRQ_EDGE_RISE)
                    card->Mount();
                else if (events_triggered_mask & GPIO_IRQ_EDGE_FALL)
                    card->Unmount();
            }
        }
    }
}