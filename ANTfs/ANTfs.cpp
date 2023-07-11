// ANTfs.cpp : This file contains the 'main' function. Program execution begins and ends there.
///

#include <Windows.h>
#include <algorithm>
#include <iostream>
#include <ctime>

//#include "privileges.h"
#include "antiforensics.h"

/*
bool 
GenerateOutputPath(
    _Inout_ std::wstring& Output
)
{
    std::wstringstream ss;
    Output.resize(MAX_PATH * 2, '\0');

    DWORD DirLength = GetCurrentDirectory(MAX_PATH, &Output[0]);
    if (DirLength == NULL)
    {
        std::wcerr << "[!] Failed to get current directory: ";
        std::wcerr << GetLastError() << std::endl;

        return false;
    }

    Output.resize(wcslen(Output.c_str()));

    ss << Output.c_str();
    ss << L"\\";
    ss << time(0);
    ss << L"_recovered\\";

    Output = ss.str();

    return true;
}
*/

void Usage(
    _In_ wchar_t* argv
)
{
    std::wcout << "Usage: " << argv << " [-d/--drive drive letter]";
    std::wcout << " [-r/--recover -o/--outfile] || [-w/--wipe-record file name]" << std::endl;
    std::wcout << "\nAn application to recover deleted data or to wipe the record of a file";
    std::wcout << " that has been introduced to the system. The goal is to stay undetected if";
    std::wcout << " the drive is being analyzed by a forensic analyst or to recover data for ";
    std::wcout << "recovering potential sensitive data." << std::endl;
    std::wcout << "\nRequired arguments:" << std::endl;
    std::wcout << "-d, --drive\tThe letter of the drive to search through." << std::endl;
    std::wcout << "\nOptional arguments:" << std::endl;
    std::wcout << "-r, --recover\tAttempts to recover deleted files on the drive." << std::endl;
    std::wcout << "-o, --outfile\tOutputs the file(s) into a directory." << std::endl;
    std::wcout << "-w, --wipe\tAttempts to wipe the record clean of the file." << std::endl;
    std::wcout << "-h, --help\tShow this help message and exit." << std::endl;
    std::wcout << "\nAtleast one of the optional arguments is required. If the recover option "; 
    std::wcout << "is selected, the outfile parameter is required. There is a default output ";
    std::wcout << "file that is a time-stamped folder created in the same directory that ";
    std::wcout << argv << " is being run in." << std::endl;
}

int wmain(
    int argc, 
    wchar_t* argv[]
)
{
    std::unique_ptr<AntiForensics> Antiforensics = nullptr;

    try
    {
        Antiforensics = std::make_unique<AntiForensics>(L"\\\\.\\C:");
    }
    catch (const std::exception& e)
    {
        std::wcerr << "[!] " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    std::wcout << "[+] Finished." << std::endl;

    /*
    bool bRecover = false;
    bool bWipe = false;

    std::wstring DriveLetter;
    std::wstring FileRecord;
    std::wstring Storage;

    UINT32 NumberOfRecords = 0;

    std::unique_ptr<antiforensics> Antiforensics;

    std::wcout << "             ___      _____________   __       " << std::endl;
    std::wcout << "             `MM\\     `M'MMMMMMMMMM  69MM      " << std::endl;
    std::wcout << "              MMM\\     M /   MM   \\ 6M' `      " << std::endl;
    std::wcout << "              M\\MM\\    M     MM    _MM______   " << std::endl;
    std::wcout << "              M \\MM\\   M     MM    MMMM6MMMMb\\ " << std::endl;
    std::wcout << "              M  \\MM\\  M     MM     MMMM'    ` " << std::endl;
    std::wcout << "              M   \\MM\\ M     MM     MMYM.      " << std::endl;
    std::wcout << "              M    \\MM\\M     MM     MM YMMMMb  " << std::endl;
    std::wcout << "              MMMMMM\\MMM     MM     MM     `Mb " << std::endl;
    std::wcout << "              M      \\MM     MM     MML    ,MM " << std::endl;
    std::wcout << "             _M_      \\M    _MM_   _MMMYMMMM9  \n" << std::endl;
    std::wcout << std::endl;
    std::wcout << std::endl;
    std::wcout << "       _     ___      _____________   __       " << std::endl;
    std::wcout << "      dM.    `MM\\     `M'MMMMMMMMMM  69MM      " << std::endl;
    std::wcout << "     ,MMb     MMM\\     M /   MM   \\ 6M' `      " << std::endl;
    std::wcout << "     d'YM.    M\\MM\\    M     MM    _MM______   " << std::endl;
    std::wcout << "    ,P `Mb    M \\MM\\   M     MM    MMMM6MMMMb\\ " << std::endl;
    std::wcout << "    d'  YM.   M  \\MM\\  M     MM     MMMM'    ` " << std::endl;
    std::wcout << "   ,P   `Mb   M   \\MM\\ M     MM     MMYM.      " << std::endl;
    std::wcout << "   d'    YM.  M    \\MM\\M     MM     MM YMMMMb  " << std::endl;
    std::wcout << "  ,MMMMMMMMb  MMMMMM\\MMM     MM     MM     `Mb " << std::endl;
    std::wcout << "  d'      YM. M      \\MM     MM     MML    ,MM " << std::endl;
    std::wcout << "_dM_     _dMM_M_      \\M    _MM_   _MMMYMMMM9  \n" << std::endl;

    if (!privileges::checkAdminPrivileges())
    {
        std::wcerr << "[!] Must run with administrator privileges!" << std::endl;
        return EXIT_FAILURE;
    }

    if (argc < 2)
    {
        Usage(argv[0]);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < argc; i++)
    {
        if (wcscmp(argv[i], L"-d") == 0 || wcscmp(argv[i], L"--drive") == 0)
        {
            DriveLetter = argv[i + 1];

            ///
            // Check to see if theres a colon for the drive. If not, add it.
            // 
            if (DriveLetter.find(L":") == std::string::npos)
            {
                DriveLetter.insert(DriveLetter.size(), L":");
            }

            ///
            // Now convert it to the format needed for CreateFile.
            // 
            DriveLetter.insert(0, L"\\\\.\\");
        }
        else if (wcscmp(argv[i], L"-r") == 0 || wcscmp(argv[i], L"--recover") == 0)
        {
            if (argv[i + 1])
            {
                if (wcscmp(argv[i + 1], L"-o") == 0 || wcscmp(argv[i + 1], L"--outfile") == 0)
                {
                    bRecover = true;
                    if (argv[i + 2])
                    {
                        Storage = argv[i + 2];

                        ///
                        // Check to see if the path ends with a back slash. If not,
                        // add it.
                        // 
                        if (reinterpret_cast<const wchar_t*>(Storage.back()) != L"\\")
                        {
                            Storage.insert(Storage.size(), L"\\");
                        }
                    }
                }
            }
        }
        else if (wcscmp(argv[i], L"-w") == 0 || wcscmp(argv[i], L"--wipe") == 0)
        {
            if (argv[i + 1] && argv[i + 1][0] != '-')
            {
                bWipe = true;

                FileRecord = argv[i + 1];
            }
        }
        else if (wcscmp(argv[i], L"-h") == 0 || wcscmp(argv[i], L"--help") == 0)
        {
            Usage(argv[0]);
            return EXIT_SUCCESS;
        }
    }

    if (!bRecover && !bWipe)
    {
        std::wcerr << "[!] Provide an option to either recover or wipe a file record" << std::endl;
        return EXIT_FAILURE;
    }

    Antiforensics = std::make_unique<antiforensics>();

    std::wcout << "[+] Initializing " << DriveLetter << "...";
    if (!Antiforensics->init(DriveLetter.c_str()))
    {
        std::wcerr << "uh-oh!" << std::endl;
        std::wcerr << Antiforensics->what() << std::endl;
        return EXIT_FAILURE;
    }
    else
    {
        std::wcout << "done." << std::endl;
    }

    if (bRecover)
    {
        if (Storage.empty())
        {
            if (!GenerateOutputPath(Storage))
            {
                std::wcerr << "[!] Unable to create string: " << GetLastError() << std::endl;
                return EXIT_FAILURE;
            }
        }

        if (!CreateDirectory(Storage.c_str(), NULL))
        {
            std::wcerr << "[!] Unable to create " << Storage;
            std::wcerr << ": " << GetLastError() << std::endl;
            return EXIT_FAILURE;
        }

        std::wcout << "[+] Scanning to recover files from: ";
        std::wcout << DriveLetter.c_str() << "." << std::endl;

        if (!Antiforensics->RecoverFiles(Storage, &NumberOfRecords))
        {
            std::wcerr << Antiforensics->what();
            return EXIT_FAILURE;
        }

        std::wcout << "[+] Finished recovering " << NumberOfRecords << " records." << std::endl;
    }


    if (bWipe && !FileRecord.empty())
    {
        ///
        // Strip the formatting of the drive so it can get processed within the kernel.
        // 
        size_t pos = DriveLetter.find(L"\\\\.\\");
        if (pos != std::wstring::npos)
        {
            DriveLetter.erase(pos, wcslen(L"\\\\.\\"));
        }
        
        std::wcout << "[+] Going to try and wipe record for ";
        std::wcout << FileRecord.c_str() << "...";

        if (!Antiforensics->WipeRecord(DriveLetter.c_str(), FileRecord.c_str()))
        {
            std::wcerr << "uh-oh!" << std::endl;
            std::wcerr << Antiforensics->what();
        }
        else
        {
            std::wcout << "done." << std::endl;
        }

        std::wcout << "[+] Finished." << std::endl;
    }
    */

    return EXIT_SUCCESS;
}


// EOF