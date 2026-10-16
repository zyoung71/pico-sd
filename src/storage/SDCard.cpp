#include <storage/SDCard.hpp>
#include <storage/FatFSFile.hpp>

#include <algorithm>

std::vector<SDCard*> SDCard::_insts = std::vector<SDCard*>();

DirectoryEntry SDCard::GetEntryFromFatFsStat(const FILINFO& info)
{
    DirectoryEntry entry;
    strlcpy(entry.name, info.fname, 256);
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
    : StorageDevice(), pc_name(pc_name)
{
    _insts.push_back(this);
}

SDCard::~SDCard()
{
    Unmount();

    auto sd_card_end = std::remove(_insts.begin(), _insts.end(), this);

    _insts.erase(sd_card_end);
}

UniqueArray<DirectoryEntry> SDCard::PeekDirectory(const char* dir_path) const
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
    f_closedir(&directory);
    return std::move(ret);
}

size_t SDCard::GetTotalCountInDirectory(const char* dir_path) const
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

    f_closedir(&directory);
    return count;
}

size_t SDCard::GetFileCountInDirectory(const char* dir_path) const
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
    f_closedir(&directory);
    return count;
}

size_t SDCard::GetDirectoryCountInDirectory(const char* dir_path) const
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

DirectoryEntry SDCard::GetDirectoryEntry(const char* path) const
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
    return f_mount(&fs, pc_name, 1) == FR_OK;
}

bool SDCard::Unmount()
{
    return f_unmount(pc_name) == FR_OK;
}

bool SDCard::IsMounted() const
{
    return fs.fs_type != 0;
}

std::unique_ptr<File> SDCard::OpenFile(const char* file_path, uint32_t access_mask)
{
    std::unique_ptr<FatFSFile> f = std::make_unique<FatFSFile>();
    f_open(&f->file, file_path, TranslateFileAccessFlags(access_mask));
    return std::move(f);
}

uint64_t SDCard::GetFileSize(const char* path) const
{
    return GetFileStats(path).fsize;
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

size_t __attribute__((weak)) sd_get_num()
{
    return SDCard::_insts.size();
}

sd_card_t* __attribute__((weak)) sd_get_by_num(size_t num)
{
    if (num < sd_get_num())
        return &SDCard::_insts[num]->card;
    return nullptr;
}

SDCardDetector::SDCardDetector(uint8_t gpio_pin, SDCard* card, bool auto_mount)
: GPIODeviceDebounce(gpio_pin, Pull::DOWN, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, 100), card(card), auto_mount(auto_mount)
{
    //card->card.use_card_detect = true;
    //card->card.card_detect_gpio = gpio_pin;
}

void SDCardDetector::HandleIRQ(uint32_t events_triggered_mask)
{
    if (event_mask & events_triggered_mask)
    {
        if (debouncer.Allow())
        {
            Event* ev = new GPIOEvent(this, events_triggered_mask);
            ProcessImmediateActions(ev);
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

bool SDCard::Delete(const char* file_path)
{
    return f_unlink(file_path) == FR_OK;
}

bool SDCard::Exists(const char* path) const
{
    return f_stat(path, nullptr) == FR_OK;
}