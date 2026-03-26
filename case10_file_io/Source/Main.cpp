#include <windows.h>
#include <iostream>
#include <vector>
#include <chrono>
#include <random>
#include <cstdio>
#include "Benchmark.h"

constexpr size_t FILE_SIZE = 100 * 1024 * 1024; // 100 MB
constexpr size_t BUFFER_SIZE = 64 * 1024;
constexpr size_t RANDOM_READS = 1'000'000;

volatile unsigned long long gqwSink = 0;

static void GenerateFile(const char* path)
{
    HANDLE hFile = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    std::vector<char> vBuffer(BUFFER_SIZE, 1);

    DWORD dwWritten;
    size_t nRem = FILE_SIZE;
    while (nRem > 0)
    {
        DWORD dwToWrite = static_cast<DWORD>(min(nRem, BUFFER_SIZE));
        WriteFile(hFile, vBuffer.data(), dwToWrite, &dwWritten, nullptr);
        nRem -= dwToWrite;
    }

    CloseHandle(hFile);
}

static void Test_fread(const char* path)
{
    FILE* pFile = fopen(path, "rb");

    char hBuffer[BUFFER_SIZE];
    while (true)
    {
        const size_t nReadBytes = fread(hBuffer, 1, BUFFER_SIZE, pFile);
        if (!nReadBytes)
        {
            break;
        }

        for (size_t i = 0; i < nReadBytes; ++i)
        {
            gqwSink += static_cast<unsigned>(hBuffer[i]);
        }
    }

    fclose(pFile);
}

static void Test_ReadFile_Seq(const char* path)
{
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, nullptr);

    char hBuffer[BUFFER_SIZE];
    
    DWORD dwRead;
    while (ReadFile(hFile, hBuffer, BUFFER_SIZE, &dwRead, nullptr) && dwRead > 0)
    {
        for (DWORD i = 0; i < dwRead; ++i)
        {
            gqwSink += static_cast<unsigned>(hBuffer[i]);
        }
    }

    CloseHandle(hFile);
}

static void Test_ReadFile_Rand(const char* path)
{
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

    std::mt19937 hRng(1234);
    std::uniform_int_distribution<size_t> hDist(0, FILE_SIZE - sizeof(int));

    DWORD dwRead = 0;
    int nValue = 0;

    for (size_t i = 0; i < RANDOM_READS; ++i)
    {
        const size_t nOffset = hDist(hRng);

        SetFilePointer(hFile, static_cast<LONG>(nOffset), nullptr, FILE_BEGIN);
        ReadFile(hFile, &nValue, sizeof(nValue), &dwRead, nullptr);

        gqwSink += static_cast<unsigned>(nValue);
    }

    CloseHandle(hFile);
}

static void Test_Mapping_Rand(const char* path)
{
    HANDLE hFile = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    HANDLE hMap = CreateFileMappingA(hFile, nullptr, PAGE_READONLY, 0, 0, nullptr);

    char* pData = reinterpret_cast<char*>(MapViewOfFile(hMap, FILE_MAP_READ, 0, 0, 0));

    std::mt19937 hRng(1234);
    std::uniform_int_distribution<size_t> hDist(0, FILE_SIZE - sizeof(int));

    for (size_t i = 0; i < RANDOM_READS; i++)
    {
        const size_t nOffset = hDist(hRng);

        const int nValue = *reinterpret_cast<int*>(pData + nOffset);

        gqwSink += static_cast<unsigned>(nValue);
    }

    UnmapViewOfFile(pData);
    CloseHandle(hMap);
    CloseHandle(hFile);
}

int main()
{
    static const char* path = "test_file.bin";

    std::cout << "Generating file...\n";
    GenerateFile(path);

    std::cout << "\n--- Sequential Read ---\n";

    auto BenchMark_fread = [] ()
    {
        Test_fread(path);
    };

    auto BenchMark_ReadFileSeq = [] ()
    {
        Test_ReadFile_Seq(path);
    };

    auto BenchMark_ReadFileRand = [] ()
    {
        Test_ReadFile_Rand(path);
    };

    auto BenchMark_MemMapping = [] ()
    {
        Test_Mapping_Rand(path);
    };

    std::cout << "fread: " << BenchmarkQPC(BenchMark_fread) << " ms\n";
    std::cout << "ReadFile (sequential): " << BenchmarkQPC(BenchMark_ReadFileSeq) << " ms\n";

    std::cout << "\n--- Random Access ---\n";

    std::cout << "ReadFile (Random): " << BenchmarkQPC(BenchMark_ReadFileRand) << " ms\n";
    std::cout << "Memory Mapping (random): " << BenchmarkQPC(BenchMark_MemMapping) << " ms\n";

    std::cout << "\n--- Cleaning Test File --- \n";
    DeleteFileA(path);
    std::cout << "Done\n";

    return 0;
}