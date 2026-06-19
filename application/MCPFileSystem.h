#pragma once
#include <stdio.h>
#include <vector>
#include <map>
#include "MCPHash.h"
struct MCPHeader
{
    unsigned int signature;
	int timestamp;
	int timestamp2;
    unsigned int dir_table_offset;
    unsigned int file_table_offset;
    unsigned int stream_offset;
};
const struct MCPFile
{
	char* m_buf;
	unsigned int m_len;
	unsigned int m_origin_len;
};
struct FileEntry
{
	int file_id;
	unsigned int offset;
	unsigned int length;
	unsigned int origin_len;
};
struct DirectoryEntry
{
    int dir_id;
    unsigned int offset;
	unsigned int count;
};
class MCPFileSystem
{
public:
	MCPFileSystem();
	MCPFileSystem(const char *filename);
	const std::vector<uint8_t> Open(const char* filename);
	void CloseFile(const MCPFile* file);
private:
	void _Init(const char *filename);
	bool _PrepareFileEntries(int dir_id);
	FILE * m_fp;
	long double m_timestamp;
	unsigned int m_dir_table_offset;
	unsigned int m_file_table_offset;
	unsigned int m_stream_offset;
	std::vector<DirectoryEntry> m_dir_table;
	std::map<int, std::vector<FileEntry>> m_file_table;
};

