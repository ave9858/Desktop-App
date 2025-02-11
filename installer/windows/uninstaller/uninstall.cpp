#include "uninstall.h"

#include <shlwapi.h>
#include <Shobjidl.h>

#include <sstream>

#include "authhelper.h"
#include "registry.h"
#include "remove_directory.h"
#include "resource.h"
#include "../utils/applicationinfo.h"
#include "../utils/logger.h"
#include "../utils/path.h"
#include "../utils/utils.h"
#include "servicecontrolmanager.h"


using namespace std;

wstring Uninstaller::UninstExeFile;
bool Uninstaller::isSilent_ = false;

static bool deleteFile(const wstring &fileName)
{
    return ::DeleteFile(fileName.c_str());
}

Uninstaller::Uninstaller()
{
}

Uninstaller::~Uninstaller()
{
}

void Uninstaller::setUninstExeFile(const wstring& exe_file, bool bFirstPhase)
{
    UninstExeFile = exe_file;
}

void Uninstaller::setSilent(bool isSilent)
{
    isSilent_ = isSilent;
}

bool Uninstaller::ProcessMsgs()
{
    tagMSG Msg;

    bool Result = false;

    while (PeekMessage(&Msg, nullptr, 0, 0, PM_REMOVE)) {
        if (Msg.message == WM_QUIT) {
            Result = true;
            break;
        }

        TranslateMessage(&Msg);
        DispatchMessage(&Msg);
    }

    return Result;
}

void Uninstaller::DelayDeleteFile(const wstring Filename, const unsigned int MaxTries,
                                  const unsigned int FirstRetryDelayMS,
                                  const unsigned int SubsequentRetryDelayMS)
{
    // Attempts to delete Filename up to MaxTries times, retrying if the file is
    // in use. It sleeps FirstRetryDelayMS msec after the first try, and
    // SubsequentRetryDelayMS msec after subsequent tries.
    for (unsigned int I = 0; I < MaxTries; I++) {
        if (I == 1) {
            Sleep(FirstRetryDelayMS);
        }
        else {
            if (I > 1) {
                Sleep(SubsequentRetryDelayMS);
            }

            DWORD error;
            error = GetLastError();
            if (deleteFile(Filename) || (error == ERROR_FILE_NOT_FOUND) || (error == ERROR_PATH_NOT_FOUND)) {
                break;
            }
        }
    }
}

void Uninstaller::DeleteUninstallDataFiles()
{
    Log::instance().out(L"Deleting data file %s", UninstExeFile.c_str());
    DelayDeleteFile(UninstExeFile, 13, 50, 250);
}

bool Uninstaller::PerformUninstall(const P_DeleteUninstallDataFilesProc DeleteUninstallDataFilesProc, const wstring& path_for_installation)
{
    Log::instance().out(L"Starting the uninstallation process.");

    ::CoInitialize(nullptr);

    const wstring keyName = ApplicationInfo::uninstallerRegistryKey(false);
    LSTATUS ret = Registry::RegDeleteKeyIncludingSubkeys1(HKEY_LOCAL_MACHINE, keyName.c_str());

    if (ret != ERROR_SUCCESS) {
        Log::instance().out(L"Uninstaller::PerformUninstall - failed to delete registry entry %s (%lu)", keyName.c_str(), ret);
    }

    RemoveDir::DelTree(path_for_installation, true, true, true, false);

    Log::instance().out(L"Deleting shortcuts.");

    wstring common_desktop = Utils::DesktopFolder();
    wstring group = Utils::StartMenuProgramsFolder();

    deleteFile(common_desktop + L"\\" + ApplicationInfo::name() + L".lnk");
    RemoveDir::DelTree(group + L"\\" + ApplicationInfo::name(), true, true, true, false);

    if (DeleteUninstallDataFilesProc != nullptr) {
        DeleteUninstallDataFilesProc();
    }

    ::CoUninitialize();

    Log::instance().out(L"Uninstallation process succeeded.");

    return true;
}

void Uninstaller::RunSecondPhase()
{
    Log::instance().out(L"Original Uninstall EXE: " + UninstExeFile);

    if (!InitializeUninstall()) {
        Log::instance().out(L"return after InitializeUninstall()");
        PostQuitMessage(0);
        return;
    }

    DeleteService();

    wstring path_for_installation = Path::extractDir(UninstExeFile);

    Log::instance().out(L"Running uninstall second phase with path: %s", path_for_installation.c_str());

    // Clear any existing auto-login entries.
    Registry::regDeleteProperty(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2", L"username");
    Registry::regDeleteProperty(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2", L"password");

    AuthHelper::removeRegEntriesForAuthHelper(path_for_installation);

    if (!isSilent_) {
        Log::instance().out(L"turn off firewall: %s", path_for_installation.c_str());
        Utils::InstExec(Path::append(path_for_installation, L"WindscribeService.exe"), L"-firewall_off", INFINITE, SW_HIDE);
    }

    Log::instance().out(L"kill openvpn process");
    Utils::InstExec(Path::append(Utils::GetSystemDir(), L"taskkill.exe"), L"/f /im openvpn.exe", INFINITE, SW_HIDE);

    Log::instance().out(L"uninstall split tunnel driver");
    UninstallSplitTunnelDriver(path_for_installation);

    Log::instance().out(L"perform uninstall");
    PerformUninstall(&DeleteUninstallDataFiles, path_for_installation);

    // open uninstall screen in browser
    wstring userId;
    Registry::RegQueryStringValue(HKEY_CURRENT_USER, L"Software\\Windscribe\\Windscribe2\\", L"userId", userId);
    if (!userId.empty()) {
        wstring urlStr = L"https://windscribe.com/uninstall/desktop?user=" + userId;
        ::ShellExecute(nullptr, L"open", urlStr.c_str(), L"", L"", SW_SHOW);
    }

    // remove any existing MAC spoofs from registry
    wstring subkeyPath = L"SYSTEM\\CurrentControlSet\\Control\\Class\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\";
    vector<wstring> networkInterfaceNames = Registry::regGetSubkeyChildNames(HKEY_LOCAL_MACHINE, subkeyPath.c_str());

    for (const auto &networkInterfaceName : networkInterfaceNames) {
        wstring interfacePath = subkeyPath + networkInterfaceName;

        if (Registry::regHasSubkeyProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"WindscribeMACSpoofed")) {
            Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"NetworkAddress");
            Registry::regDeleteProperty(HKEY_LOCAL_MACHINE, interfacePath.c_str(), L"WindscribeMACSpoofed");
        }
    }

    Log::instance().out(L"Removing directory %s", path_for_installation.c_str());
    ::RemoveDirectory(path_for_installation.c_str());

    if (!isSilent_) {
        messageBox(IDS_UNINSTALL_SUCCESS, MB_ICONINFORMATION | MB_OK | MB_SETFOREGROUND);
    }

    PostQuitMessage(0);
}

bool Uninstaller::InitializeUninstall()
{
    if (isSilent_) {
        return true;
    }

    HWND hwnd = Utils::appMainWindowHandle();

    while (hwnd) {
        int result = messageBox(IDS_CLOSE_APP, MB_ICONINFORMATION | MB_RETRYCANCEL | MB_SETFOREGROUND);
        if (result == IDCANCEL) {
            return false;
        }

        hwnd = Utils::appMainWindowHandle();
    }

    return true;
}

void Uninstaller::UninstallSplitTunnelDriver(const wstring& installationPath)
{
    wostringstream commandLine;
    commandLine << Path::append(Utils::GetSystemDir(), L"setupapi.dll")
                << L",InstallHinfSection DefaultUninstall 132 "
                << Path::append(installationPath, L"splittunnel\\windscribesplittunnel.inf");

    wstring appName = Path::append(Utils::GetSystemDir(), L"rundll32.exe");

    auto result = Utils::InstExec(appName, commandLine.str(), 30 * 1000, SW_HIDE);

    if (!result.has_value()) {
        Log::instance().out("WARNING: The split tunnel driver uninstall failed to launch.");
    }
    else if (result.value() == WAIT_TIMEOUT) {
        Log::instance().out("WARNING: The split tunnel driver uninstall stage timed out.");
        return;
    }
    else if (result.value() != NO_ERROR) {
        Log::instance().out("WARNING: The split tunnel driver uninstall returned a failure code (%lu).", result.value());
    }
}

void Uninstaller::DeleteService()
{
    const wstring serviceName = ApplicationInfo::serviceName();
    try {
        wsl::ServiceControlManager svcCtrl;
        svcCtrl.openSCM(SC_MANAGER_ALL_ACCESS);

        if (svcCtrl.isServiceInstalled(serviceName.c_str())) {
            svcCtrl.deleteService(serviceName.c_str());
        }
    }
    catch (system_error& ex) {
        Log::instance().out("WARNING: failed to delete the Windscribe service - %s", ex.what());
    }
}

int Uninstaller::messageBox(const UINT resourceID, const UINT type, HWND ownerWindow)
{
    // TODO: **JDRM** May need to add MB_RTLREADING flag to MessageBox call if the OS is set to a RTL language.

    const int bufSize = 512;
    wchar_t title[bufSize];
    wchar_t message[bufSize];
    ::ZeroMemory(title, bufSize * sizeof(wchar_t));
    ::ZeroMemory(message, bufSize * sizeof(wchar_t));

    HMODULE hModule = ::GetModuleHandle(NULL);
    int result = ::LoadString(hModule, IDS_MSGBOX_TITLE, title, bufSize);
    if (result == 0) {
        Log::instance().out("WARNING: loading of the message box title string resource failed (%lu).", ::GetLastError());
        wcscpy_s(title, bufSize, L"Uninstall Windscribe");
    }

    result = ::LoadString(hModule, resourceID, message, bufSize);
    if (result == 0) {
        Log::instance().out("WARNING: loading of string resource %d failed (%lu).", resourceID, ::GetLastError());
        wcscpy_s(message, bufSize, L"Windscribe uninstall encountered a technical failure.");
    }

    result = ::MessageBox(ownerWindow, message, title, type);

    return result;
}
