#include "MCPFileSystem.h"

MCPFileSystem::MCPFileSystem()
    : m_fp(nullptr)
    , m_timestamp(0.0)
    , m_dir_table_offset(0)
    , m_file_table_offset(0)
    , m_stream_offset(0)
    , m_dir_table()
    , m_file_table()
{
}

MCPFileSystem::MCPFileSystem(const char* filename)
    : m_fp(nullptr)
    , m_timestamp(0.0)
    , m_dir_table_offset(0)
    , m_file_table_offset(0)
    , m_stream_offset(0)
    , m_dir_table()
    , m_file_table()
{
    _Init(filename);
}

void MCPFileSystem::_Init(const char* filename) {
    // 打开MCP文件
    this->m_fp = fopen(filename, "rb");
    if (!this->m_fp) {
        printf("Failed to open file: %s\n", filename);
        if (this->m_fp) {
            fclose(this->m_fp);
            this->m_fp = nullptr;
        }
        return;
    }
    // 读取文件头
    MCPHeader header;
    if (fread(&header, 0x18, 1, this->m_fp) != 1 || header.signature != 0x4B50434D) { // "MCPK"
        puts("Invalid file format!");
        if (this->m_fp) {
            fclose(this->m_fp);
            this->m_fp = nullptr;
        }
    }

    // 保存头部信息
    this->m_timestamp = header.timestamp;
    this->m_dir_table_offset = header.dir_table_offset;
    this->m_file_table_offset = header.file_table_offset;
    this->m_stream_offset = header.stream_offset;

    // 定位到目录表并读取
    fseek(this->m_fp, this->m_dir_table_offset, SEEK_SET);

    // 计算目录表条目数量 (文件表偏移 - 目录表偏移) / 12 (每个DirectoryEntry 12字节)
    size_t dir_entry_count = (this->m_file_table_offset - this->m_dir_table_offset) / sizeof(DirectoryEntry);

    // 调整目录表vector大小
    this->m_dir_table.resize(dir_entry_count);

    // 读取整个目录表到内存
    fread(this->m_dir_table.data(),
        sizeof(DirectoryEntry),
        this->m_dir_table.size(),
        this->m_fp);

    // 对目录表进行排序验证（逆向代码中的二分查找逻辑）
    // 这可能是为了确保目录表已排序，或者查找特定条目
    auto first = this->m_dir_table.begin();
    auto last = this->m_dir_table.end();
    /*
    size_t count = std::distance(first, last);

    while (count > 0) {
        auto it = first;
        size_t step = count / 2;
        std::advance(it, step);

        if (it->dir_id >= 0) {
            count = step;
        }
        else {
            first = ++it;
            count -= step + 1;
        }
    }*/

    // 检查目录表是否有效
    if (first == this->m_dir_table.end()) {
        puts("Invalid directory table!");
        if (this->m_fp) {
            fclose(this->m_fp);
            this->m_fp = nullptr;
        }
    }

    return;
}
void MCPFileSystem::CloseFile(const MCPFile* file) {
    if (file) {
        // 调用scalar deleting destructor
        // 实际上就是手动执行清理逻辑
        char* m_buf = file->m_buf;
        if (m_buf) {
            operator delete(m_buf);
        }
        operator delete((void*)file, sizeof(MCPFile));
    }
}
bool MCPFileSystem::_PrepareFileEntries(int dir_id) {
    // 1. 在m_file_table中创建目录条目（如果不存在）
    std::pair<std::map<int, std::vector<FileEntry>>::iterator, bool> result;
    result = m_file_table.insert(std::make_pair(dir_id, std::vector<FileEntry>()));

    if (!result.second) {
        // 目录已经存在，直接返回成功
        return true;
    }

    auto iter = result.first;

    // 2. 在m_dir_table中二分查找目录信息
    /*
    auto dir_begin = m_dir_table.begin();
    auto dir_end = m_dir_table.end();
    auto dir_iter = dir_begin;
    size_t count = m_dir_table.size();

    // 二分查找目录
    while (count > 0) {
        size_t step = count / 2;
        dir_iter = dir_begin + step;

        if (dir_iter->dir_id >= dir_id) {
            count = step;
        }
        else {
            dir_begin = dir_iter + 1;
            count -= step + 1;
        }
    }*/
    bool find = false;
    DirectoryEntry dir_iter;
    for (size_t i = 0; i < m_dir_table.size(); i++)
    {
        if (m_dir_table[i].dir_id == dir_id) {
            find = true;
            dir_iter = m_dir_table[i];
            break;
        }
    }

    // 检查是否找到目录
    if (!find) {
        // 没找到目录，从m_file_table中移除刚创建的条目
        m_file_table.erase(iter);
        return false;
    }



    // 3. 定位到文件表区域并读取文件条目
    uint32_t file_table_pos = m_file_table_offset + dir_iter.offset;
    fseek(m_fp, file_table_pos, SEEK_SET);

    // 4. 调整vector大小以容纳所有文件条目
    std::vector<FileEntry>& file_entries = iter->second;
    file_entries.resize(dir_iter.count);

    // 5. 读取文件条目数据
    fread(file_entries.data(), sizeof(FileEntry), dir_iter.count, m_fp);

    return true;
}

const std::vector<uint8_t> MCPFileSystem::Open(const char* filename) {
    // 1. 分离路径和文件名
    const char* slash_pos = strrchr(filename, '/');
    const char* pure_filename = filename;
    int dir_hash = 0;  // 0表示根目录

    if (slash_pos) {
        // 计算目录部分的哈希
        dir_hash = NeoXHash::StringIDLegacy(filename, slash_pos - filename);
        pure_filename = slash_pos + 1;
    }

    // 2. 在红黑树中查找目录节点
    auto dir_iter = m_file_table.find(dir_hash);
    if (dir_iter == m_file_table.end()) {
        // 目录未加载，动态加载
        if (!_PrepareFileEntries(dir_hash)) {
            return std::vector<uint8_t>();
        }
        dir_iter = m_file_table.find(dir_hash);
        if (dir_iter == m_file_table.end()) {
            return std::vector<uint8_t>();
        }
    }

    // 3. 计算文件名的哈希
    int file_hash = NeoXHash::StringIDLegacy(pure_filename, strlen(pure_filename));

    // 4. 在目录的文件列表中二分查找
    std::vector<FileEntry>& file_list = dir_iter->second;

    
    // 二分查找实现
    FileEntry first;
    bool is_find = false;
    for (size_t i = 0; i < file_list.size(); i++)
    {
        if (file_list[i].file_id == file_hash) {
            is_find = true;
            first.file_id = file_list[i].file_id;
            first.offset = file_list[i].offset;
            first.length = file_list[i].length;
            first.origin_len = file_list[i].origin_len;
            break;
        }
    }
    /*
    FileEntry* last = first + file_list.size();
    size_t count = file_list.size();

    while (count > 0) {
        size_t step = count / 2;
        FileEntry* mid = first + step;

        if (mid->file_id >= file_hash) {
            count = step;
        }
        else {
            first = mid + 1;
            count -= step + 1;
        }
    }

    if (first == last || first->file_id != file_hash) {
        return nullptr;
    }
    */
    if (!is_find) {
        return std::vector<uint8_t>();
    }
    

    // 5. 创建MCPFile对象
    std::vector<uint8_t> file;

    // 6. 分配缓冲区并读取文件数据
    file.resize(first.length);

    // 7. 从NPK包读取数据
    fseek(m_fp, first.offset + m_stream_offset, SEEK_SET);
    fread(file.data(), 1, first.length, m_fp);

    return file;
}
