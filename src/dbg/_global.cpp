/**
\file _global.cpp
\brief Implements the global class.
*/

#include "_global.h"
#include <objbase.h>
#include <shlobj.h>

/**
\brief x64dbg library instance.
*/
HINSTANCE hInst;

/**
\brief Number of allocated buffers by emalloc(). This should be 0 when x64dbg ends.
*/
static int emalloc_count = 0;

/**
\brief Path for debugging, used to create an allocation trace file on emalloc() or efree(). Not used.
*/
static char alloctrace[MAX_PATH] = "";

/**
\brief Allocates a new buffer.
\param size The size of the buffer to allocate (in bytes).
\param reason The reason for allocation (can be used for memory allocation tracking).
\return Always returns a valid pointer to the buffer you requested. Will quit the application on errors.
*/
void* emalloc(size_t size, const char* reason)
{
    ASSERT_NONZERO(size);

    unsigned char* a = (unsigned char*)GlobalAlloc(GMEM_FIXED, size);
    if(!a)
    {
        MessageBoxW(0, L"Could not allocate memory", L"Error", MB_ICONERROR);
        ExitProcess(1);
    }
    memset(a, 0, size);
    emalloc_count++;
    /*
    FILE* file = fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:  alloc:" fhex ":%s:" fhex "\n", emalloc_count, a, reason, size);
    fclose(file);
    */
    return a;
}

/**
\brief Reallocates a buffer allocated with emalloc().
\param [in] Pointer to memory previously allocated with emalloc(). When NULL a new buffer will be allocated by emalloc().
\param size The new memory size.
\param reason The reason for allocation (can be used for memory allocation tracking).
\return Always returns a valid pointer to the buffer you requested. Will quit the application on errors.
*/
void* erealloc(void* ptr, size_t size, const char* reason)
{
    ASSERT_NONZERO(size);

    // Free the memory if the pointer was set (as per documentation).
    if(ptr)
        efree(ptr, reason);

    return emalloc(size, reason);
}

/**
\brief Free memory previously allocated with emalloc().
\param [in] Pointer to the memory to free.
\param reason The reason for freeing, should be the same as the reason for allocating.
*/
void efree(void* ptr, const char* reason)
{
    emalloc_count--;
    /*
    FILE* file = fopen(alloctrace, "a+");
    fprintf(file, "DBG%.5d:   free:" fhex ":%s\n", emalloc_count, ptr, reason);
    fclose(file);
    */
    GlobalFree(ptr);
}

void* json_malloc(size_t size)
{
    return emalloc(size, "json:ptr");
}

void json_free(void* ptr)
{
    efree(ptr, "json:ptr");
}

/**
\brief Gets the number of memory leaks. This number is only valid in _dbg_dbgexitsignal().
\return The number of memory leaks.
*/
int memleaks()
{
    return emalloc_count;
}

/**
\brief Sets the path for the allocation trace file.
\param file UTF-8 filepath.
*/
void setalloctrace(const char* file)
{
    strcpy_s(alloctrace, file);
}

/**
\brief A function to determine if a string is contained in a specifically formatted 'array string'.
\param cmd_list Array of strings separated by '\1'.
\param cmd The string to look for.
\return true if \p cmd is contained in \p cmd_list.
*/
bool arraycontains(const char* cmd_list, const char* cmd)
{
    //TODO: fix this function a little
    if(!cmd_list || !cmd)
        return false;
    char temp[deflen] = "";
    strcpy_s(temp, cmd_list);
    int len = (int)strlen(cmd_list);
    if(len >= deflen)
        return false;
    for(int i = 0; i < len; i++)
        if(temp[i] == 1)
            temp[i] = 0;
    if(!_stricmp(temp, cmd))
        return true;
    for(int i = (int)strlen(temp); i < len; i++)
    {
        if(!temp[i])
        {
            if(!_stricmp(temp + i + 1, cmd))
                return true;
            i += (int)strlen(temp + i + 1);
        }
    }
    return false;
}

/**
\brief Compares two strings without case-sensitivity.
\param a The first string.
\param b The second string.
\return true if the strings are equal (case-insensitive).
*/
bool scmp(const char* a, const char* b)
{
    if(_stricmp(a, b))
        return false;
    return true;
}

/**
\brief Formats a string to hexadecimal format (removes all non-hex characters).
\param [in,out] String to format.
*/
void formathex(char* string)
{
    int len = (int)strlen(string);
    _strupr(string);
    Memory<char*> new_string(len + 1, "formathex:new_string");
    for(int i = 0, j = 0; i < len; i++)
        if(isxdigit(string[i]))
            j += sprintf(new_string() + j, "%c", string[i]);
    strcpy_s(string, len + 1, new_string());
}

/**
\brief Formats a string to decimal format (removed all non-numeric characters).
\param [in,out] String to format.
*/
void formatdec(char* string)
{
    int len = (int)strlen(string);
    _strupr(string);
    Memory<char*> new_string(len + 1, "formatdec:new_string");
    for(int i = 0, j = 0; i < len; i++)
        if(isdigit(string[i]))
            j += sprintf(new_string() + j, "%c", string[i]);
    strcpy_s(string, len + 1, new_string());
}

/**
\brief Queries if a given file exists.
\param file Path to the file to check (UTF-8).
\return true if the file exists on the hard drive.
*/
bool FileExists(const char* file)
{
    DWORD attrib = GetFileAttributesW(StringUtils::Utf8ToUtf16(file).c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && !(attrib & FILE_ATTRIBUTE_DIRECTORY));
}

/**
\brief Queries if a given directory exists.
\param dir Path to the directory to check (UTF-8).
\return true if the directory exists.
*/
bool DirExists(const char* dir)
{
    DWORD attrib = GetFileAttributesW(StringUtils::Utf8ToUtf16(dir).c_str());
    return (attrib != INVALID_FILE_ATTRIBUTES && (attrib & FILE_ATTRIBUTE_DIRECTORY) != 0);
}

/**
\brief Gets file path from a file handle.
\param hFile File handle to get the path from.
\param [in,out] szFileName Buffer of size MAX_PATH.
\return true if it succeeds, false if it fails.
*/
bool GetFileNameFromHandle(HANDLE hFile, char* szFileName)
{
    wchar_t wszFileName[MAX_PATH] = L"";
    if(!PathFromFileHandleW(hFile, wszFileName, sizeof(wszFileName)))
        return false;
    strcpy_s(szFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    return true;
}

bool GetFileNameFromProcessHandle(HANDLE hProcess, char* szFileName)
{
    wchar_t wszDosFileName[MAX_PATH] = L"";
    if(!GetProcessImageFileNameW(hProcess, wszDosFileName, sizeof(wszDosFileName)))
        return false;

    wchar_t wszFileName[MAX_PATH] = L"";
    if(!DevicePathToPathW(wszDosFileName, wszFileName, MAX_PATH * sizeof(wchar_t)))
        return false;
    strcpy_s(szFileName, MAX_PATH, StringUtils::Utf16ToUtf8(wszFileName).c_str());
    return true;
}

/**
\brief Get a boolean setting from the configuration file.
\param section The section of the setting (UTF-8).
\param name The name of the setting (UTF-8).
\return true if the setting was set and equals to true, otherwise returns false.
*/
bool settingboolget(const char* section, const char* name)
{
    duint setting;
    if(!BridgeSettingGetUint(section, name, &setting))
        return false;
    if(setting)
        return true;
    return false;
}

/**
\brief Gets file architecture.
\param szFileName UTF-8 encoded file path.
\return The file architecture (::arch).
*/
arch GetFileArchitecture(const char* szFileName)
{
    arch retval = notfound;
    Handle hFile = CreateFileW(StringUtils::Utf8ToUtf16(szFileName).c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(hFile != INVALID_HANDLE_VALUE)
    {
        unsigned char data[0x1000];
        DWORD read = 0;
        DWORD fileSize = GetFileSize(hFile, 0);
        DWORD readSize = sizeof(data);
        if(readSize > fileSize)
            readSize = fileSize;
        if(ReadFile(hFile, data, readSize, &read, 0))
        {
            retval = invalid;
            IMAGE_DOS_HEADER* pdh = (IMAGE_DOS_HEADER*)data;
            if(pdh->e_magic == IMAGE_DOS_SIGNATURE && (size_t)pdh->e_lfanew < readSize)
            {
                IMAGE_NT_HEADERS* pnth = (IMAGE_NT_HEADERS*)(data + pdh->e_lfanew);
                if(pnth->Signature == IMAGE_NT_SIGNATURE)
                {
                    if(pnth->FileHeader.Machine == IMAGE_FILE_MACHINE_I386) //x32
                        retval = x32;
                    else if(pnth->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64) //x64
                        retval = x64;
                }
            }
        }
    }
    return retval;
}

/**
\brief Query if x64dbg is running in Wow64 mode.
\return true if running in Wow64, false otherwise.
*/
bool IsWow64()
{
    BOOL bIsWow64Process = FALSE;
    //x64dbg supports WinXP SP3 and later only, so ignore the GetProcAddress crap :D
    IsWow64Process(GetCurrentProcess(), &bIsWow64Process);
    return !!bIsWow64Process;
}

//Taken from: http://www.cplusplus.com/forum/windows/64088/
bool ResolveShortcut(HWND hwnd, const wchar_t* szShortcutPath, char* szResolvedPath, size_t nSize)
{
    if(szResolvedPath == NULL)
        return SUCCEEDED(E_INVALIDARG);

    //Initialize COM stuff
    CoInitialize(NULL);

    //Get a pointer to the IShellLink interface.
    IShellLink* psl = NULL;
    HRESULT hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (LPVOID*)&psl);
    if(SUCCEEDED(hres))
    {
        //Get a pointer to the IPersistFile interface.
        IPersistFile* ppf = NULL;
        hres = psl->QueryInterface(IID_IPersistFile, (void**)&ppf);
        if(SUCCEEDED(hres))
        {
            //Load the shortcut.
            hres = ppf->Load(szShortcutPath, STGM_READ);

            if(SUCCEEDED(hres))
            {
                //Resolve the link.
                hres = psl->Resolve(hwnd, 0);

                if(SUCCEEDED(hres))
                {
                    //Get the path to the link target.
                    char szGotPath[MAX_PATH] = {0};
                    hres = psl->GetPath(szGotPath, _countof(szGotPath), NULL, SLGP_SHORTPATH);

                    if(SUCCEEDED(hres))
                    {
                        strcpy_s(szResolvedPath, nSize, szGotPath);
                    }
                }
            }

            //Release the pointer to the IPersistFile interface.
            ppf->Release();
        }

        //Release the pointer to the IShellLink interface.
        psl->Release();
    }

    //Uninitialize COM stuff
    CoUninitialize();
    return SUCCEEDED(hres);
}

void WaitForThreadTermination(HANDLE hThread)
{
    WaitForSingleObject(hThread, INFINITE);
    CloseHandle(hThread);
}
