#include <sd/SDCard.h>

#include <f_util.h>

SDCard* SDCard::_insts[8] = {nullptr};

SDCard::SDCard(const char* pc_name)
    : _this_inst_idx(-1), is_file_open(false),
    is_mounted(false), pc_name(pc_name),
    current_file_path(nullptr), stream(this)
{
    for (size_t inst_idx = 0; inst_idx < 4; inst_idx++)
    {
        if (_insts[inst_idx] == nullptr)
        {
            _insts[inst_idx] = this;
            _this_inst_idx = inst_idx;
            break;
        }
    }
}

SDCard::~SDCard()
{
    if (_this_inst_idx != -1)
        _insts[_this_inst_idx] = nullptr;
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
        
        DirectoryEntry entry;
        strncpy(entry.name, file_info.fname, 256);
        entry.attributes_mask = file_info.fattrib;
        ret[count++] = entry;
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

bool SDCard::OpenFile(const char* file_path, uint8_t permissions_mask)
{
    if (is_file_open)
        f_close(&file);
    
    is_file_open = true;
    current_file_path = file_path;
    return f_open(&file, file_path, permissions_mask) == FR_OK;
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
#include "ffconf.h"
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
        char buff[1];
        UINT bytes_read;
        f_read(&file, buff, 1, &bytes_read);
        return buff[0];
    }
    return '\0';
}

size_t SDCard::ReadAll(std::unique_ptr<char[]>& buffer)
{
    if (is_file_open)
    {
        UINT bytes_read;
        uint64_t size = f_size(&file);
        buffer = std::make_unique<char[]>(size);
        f_read(&file, buffer.get(), size, &bytes_read);
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

SDCardStream& SDCard::GetStream()
{
    return stream;
}

size_t sd_get_num()
{
    size_t n = 0;
    for (int i = 0; i < 4; i++)
        if (SDCard::_insts[i])
            n++;
    return n;
}

sd_card_t* sd_get_by_num(size_t num)
{
    num = num > 3 ? 3 : num;
    for (; num < 4; num++)
    {
        if (SDCard::_insts[num])
            return &SDCard::_insts[num]->card;
    }
    return nullptr;
}

SDCardDetector::SDCardDetector(uint8_t gpio_pin, SDCard* card, bool auto_mount, void* user_data)
: GPIODeviceDebounce(gpio_pin, Pull::UP, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 100, user_data), card(card), auto_mount(auto_mount)
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

SDCardStream::SDCardStream(SDCard* card)
    : card(card)
{
}

SDCardStream& SDCardStream::InsertBuffer(const void* buffer, size_t length)
{
    card->WriteBuffer(buffer, length);
    return *this;
}

SDCardStream& SDCardStream::operator<<(const BufferView<void>& buffer)
{
    card->WriteBuffer(buffer.buffer, buffer.length);
    return *this;
}

SDCardStream& SDCardStream::operator<<(const ArrayAccessor<void>& buffer)
{
    card->WriteBuffer(buffer.data, buffer.length);
    return *this;
}

SDCardStream& SDCardStream::operator<<(const char* strbuff)
{
    card->WriteString(strbuff);
    return *this;
}

SDCardStream& SDCardStream::operator<<(char* strbuff)
{
    card->WriteString(strbuff);
    return *this;
}

SDCardStream& SDCardStream::operator<<(char c)
{
    card->WriteCharacter(c);
    return *this;
}

SDCardStream& SDCardStream::ExtractBuffer(void* buffer, size_t length)
{
    card->ReadBuffer(buffer, length);
    return *this;
}

SDCardStream& SDCardStream::operator>>(ArrayAccessor<void>& buffer)
{
    card->ReadBuffer(buffer.data, buffer.length);
    return *this;
}