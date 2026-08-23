#include "MCPFileSystem.h"
#include <algorithm>

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
    // ��MCP�ļ�
    this->m_fp = fopen(filename, "rb");
    if (!this->m_fp) {
        printf("Failed to open file: %s\n", filename);
        if (this->m_fp) {
            fclose(this->m_fp);
            this->m_fp = nullptr;
        }
        return;
    }
    // ��ȡ�ļ�ͷ
    MCPHeader header;
    if (fread(&header, 0x18, 1, this->m_fp) != 1 || header.signature != 0x4B50434D) { // "MCPK"
        puts("Invalid file format!");
        if (this->m_fp) {
            fclose(this->m_fp);
            this->m_fp = nullptr;
        }
    }

    // ����ͷ����Ϣ
    this->m_timestamp = header.timestamp;
    this->m_dir_table_offset = header.dir_table_offset;
    this->m_file_table_offset = header.file_table_offset;
    this->m_stream_offset = header.stream_offset;

    // ��λ��Ŀ¼������ȡ
    fseek(this->m_fp, this->m_dir_table_offset, SEEK_SET);

    // ����Ŀ¼����Ŀ���� (�ļ���ƫ�� - Ŀ¼��ƫ��) / 12 (ÿ��DirectoryEntry 12�ֽ�)
    size_t dir_entry_count = (this->m_file_table_offset - this->m_dir_table_offset) / sizeof(DirectoryEntry);

    // ����Ŀ¼��vector��С
    this->m_dir_table.resize(dir_entry_count);

    // ��ȡ����Ŀ¼�����ڴ�
    fread(this->m_dir_table.data(),
        sizeof(DirectoryEntry),
        this->m_dir_table.size(),
        this->m_fp);

    if (this->m_dir_table.empty()) {
        puts("Invalid directory table!");
        if (this->m_fp) {
            fclose(this->m_fp);
            this->m_fp = nullptr;
        }
        return;
    }

    std::sort(this->m_dir_table.begin(), this->m_dir_table.end(),
        [](const DirectoryEntry& a, const DirectoryEntry& b) {
            return a.dir_id < b.dir_id;
        });

    return;
}
void MCPFileSystem::CloseFile(const MCPFile* file) {
    if (file) {
        // ����scalar deleting destructor
        // ʵ���Ͼ����ֶ�ִ�������߼�
        char* m_buf = file->m_buf;
        if (m_buf) {
            operator delete(m_buf);
        }
        operator delete((void*)file, sizeof(MCPFile));
    }
}
bool MCPFileSystem::_PrepareFileEntries(int dir_id) {
    // 1. ��m_file_table�д���Ŀ¼��Ŀ����������ڣ�
    std::pair<std::map<int, std::vector<FileEntry>>::iterator, bool> result;
    result = m_file_table.insert(std::make_pair(dir_id, std::vector<FileEntry>()));

    if (!result.second) {
        // Ŀ¼�Ѿ����ڣ�ֱ�ӷ��سɹ�
        return true;
    }

    auto iter = result.first;

    // 二分查找目录
    auto dir_it = std::lower_bound(m_dir_table.begin(), m_dir_table.end(), dir_id,
        [](const DirectoryEntry& entry, int id) {
            return entry.dir_id < id;
        });

    if (dir_it == m_dir_table.end() || dir_it->dir_id != dir_id) {
        m_file_table.erase(iter);
        return false;
    }

    DirectoryEntry dir_iter = *dir_it;



    // 3. ��λ���ļ������򲢶�ȡ�ļ���Ŀ
    uint32_t file_table_pos = m_file_table_offset + dir_iter.offset;
    fseek(m_fp, file_table_pos, SEEK_SET);

    // 4. ����vector��С�����������ļ���Ŀ
    std::vector<FileEntry>& file_entries = iter->second;
    file_entries.resize(dir_iter.count);

    // 5. ��ȡ�ļ���Ŀ����
    fread(file_entries.data(), sizeof(FileEntry), dir_iter.count, m_fp);

    std::sort(file_entries.begin(), file_entries.end(),
        [](const FileEntry& a, const FileEntry& b) {
            return a.file_id < b.file_id;
        });

    return true;
}

const std::vector<uint8_t> MCPFileSystem::Open(const char* filename) {
    // 1. ����·�����ļ���
    const char* slash_pos = strrchr(filename, '/');
    const char* pure_filename = filename;
    int dir_hash = 0;  // 0��ʾ��Ŀ¼

    if (slash_pos) {
        // ����Ŀ¼���ֵĹ�ϣ
        dir_hash = NeoXHash::StringIDLegacy(filename, slash_pos - filename);
        pure_filename = slash_pos + 1;
    }

    // 2. �ں�����в���Ŀ¼�ڵ�
    auto dir_iter = m_file_table.find(dir_hash);
    if (dir_iter == m_file_table.end()) {
        // Ŀ¼δ���أ���̬����
        if (!_PrepareFileEntries(dir_hash)) {
            return std::vector<uint8_t>();
        }
        dir_iter = m_file_table.find(dir_hash);
        if (dir_iter == m_file_table.end()) {
            return std::vector<uint8_t>();
        }
    }

    // 3. �����ļ����Ĺ�ϣ
    int file_hash = NeoXHash::StringIDLegacy(pure_filename, strlen(pure_filename));
    // 二分查找文件
    std::vector<FileEntry>& file_list = dir_iter->second;

    auto file_it = std::lower_bound(file_list.begin(), file_list.end(), file_hash,
        [](const FileEntry& entry, int id) {
            return entry.file_id < id;
        });

    if (file_it == file_list.end() || file_it->file_id != file_hash) {
        return std::vector<uint8_t>();
    }

    const FileEntry& first = *file_it;
    

    // 5. ����MCPFile����
    std::vector<uint8_t> file;

    // 6. ���仺��������ȡ�ļ�����
    file.resize(first.length);

    // 7. ��NPK����ȡ����
    fseek(m_fp, first.offset + m_stream_offset, SEEK_SET);
    fread(file.data(), 1, first.length, m_fp);

    return file;
}
