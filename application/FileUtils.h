#pragma once
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>
#include "Logger.h"
#include <iostream>

#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#define GETCWD _getcwd
#define GETCWD_BUFFER_MAX _MAX_PATH
#else
#include <unistd.h>
#define GETCWD getcwd
#define GETCWD_BUFFER_MAX 1024
#endif


using namespace std;
class FileUtils
{
public:
    static std::vector<char> readFileBinary(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate); // ���ļ�����λ��ĩβ
        if (!file.is_open()) {
            throw std::runtime_error("Failed to open file: " + filePath);
        }

        // ��ȡ�ļ���С
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg); // �ص��ļ���ͷ

        // ��ȡ�����ļ��� vector
        std::vector<char> buffer(size);
        if (!file.read(buffer.data(), size)) {
            throw std::runtime_error("Failed to read file: " + filePath);
        }

        return buffer;
    }
    static std::string get_current_directory() {
        char buffer[GETCWD_BUFFER_MAX];

        if (GETCWD(buffer, sizeof(buffer)) != nullptr) {
            return std::string(buffer);
        }
        else {
            return ""; // ���ؿ��ַ�����ʾʧ��
        }
    }
    static std::string FileConetnt(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) {
            Logger::getInstance().log(LOG_ERROR,"The file does not: "+filePath);
            return string();
        }

        // һ���Զ�ȡ�����ļ����ַ���
        std::string content((std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>());
        return content;
    }
    static std::vector<uint8_t> readFileToBinary(const std::string& filePath) {
        // �Զ�����ģʽ���ļ�
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);

        if (!file.is_open()) {
            return std::vector<uint8_t>();
        }

        // ��ȡ�ļ���С
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        // �����㹻���vector���洢�ļ�����
        std::vector<uint8_t> buffer(size);

        // ��ȡ�����ļ���vector��
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            return std::vector<uint8_t>();
        }

        return buffer;
    }
    static bool fileExists(const std::string& filename) {
        return safeFileCheck(filename);
    }
    static bool safeFileCheck(const std::string& filename) {
        // ����ļ����Ƿ���Ч
        if (filename.empty()) {
            //std::cerr << "����: �ļ���Ϊ��" << std::endl;
            return false;
        }

        // ����ļ�������
        if (filename.length() > 260) { // Windows·������
            //std::cerr << "����: �ļ�������" << std::endl;
            return false;
        }

        // ���Ƿ��ַ�
        if (filename.find("..") != std::string::npos ||
            filename.find("//") != std::string::npos) {
            //std::cerr << "����: �ļ��������Ƿ��ַ�" << std::endl;
            return false;
        }

        try {
            std::ifstream file(filename);
            if (!file.is_open()) {
                // �������������
                if (errno == ENOENT) {
                    // �ļ������� - �����������
                    return false;
                }
                else if (errno == EACCES) {
                    //std::cerr << "����: �ļ����ڵ��޷���Ȩ��" << std::endl;
                    return true; // �ļ����ڵ��޷�����
                }
                else {
                    //std::cerr << "����: �ļ����ʱ����δ֪����" << std::endl;
                    return false;
                }
            }

            file.close();
            return true;

        }
        catch (const std::exception& e) {
            //std::cerr << "�쳣: " << e.what() << std::endl;
            return false;
        }
        catch (...) {
            //std::cerr << "δ֪�쳣" << std::endl;
            return false;
        }
    }
};
class FileStream {
public:
    std::ifstream* file;
    bool OpenFile(const std::string& path) {
        file = new ifstream(path);
        if (!file->is_open()) {
            return false;
        }
    }
    std::string ReadLine() {
        string result;
        std::getline(*file, result);
        return result;
    }
    bool eof() {
        return file->eof();
    }
    ~FileStream() {
        delete file;
    }
};
