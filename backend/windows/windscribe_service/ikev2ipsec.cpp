#include "all_headers.h"
#include "ikev2ipsec.h"
#include "executecmd.h"
#include "logger.h"
#include <shlobj.h>
#include <wtsapi32.h>

#pragma comment(lib, "Wtsapi32.lib")

namespace
{
static const TCHAR kWindscribeConnectionName[] = TEXT("Windscribe IKEv2");

// RAII helper for security impersonation as an active logged on user.
// This is essential because phone books are specific to the user.
class CurrentUserImpersonationHelper
{
public:
    CurrentUserImpersonationHelper() : token_(INVALID_HANDLE_VALUE), is_impersonated_(false)
    {
        HANDLE base_token;
        if (!WTSQueryUserToken(WTSGetActiveConsoleSessionId(), &base_token))
            return;
        if (DuplicateTokenEx(base_token, MAXIMUM_ALLOWED, NULL, SecurityImpersonation,
            TokenPrimary, &token_)) {
            if (ImpersonateLoggedOnUser(token_))
                is_impersonated_ = true;
        }
        CloseHandle(base_token);
    }
    ~CurrentUserImpersonationHelper()
    {
        if (is_impersonated_)
            RevertToSelf();
        if (token_ != INVALID_HANDLE_VALUE)
            CloseHandle(token_);
    }
    HANDLE token() const { return token_; }
    bool isImpersonated() const { return is_impersonated_; }
private:
    HANDLE token_;
    bool is_impersonated_;
};
}

bool IKEv2IPSec::setIKEv2IPSecParameters()
{
    // First, try to add IPSec parameters to the phonebook manually.
    if (setIKEv2IPSecParametersInPhoneBook())
        return true;
    // If it failed, try to add them via a PowerShell command.
    return setIKEv2IPSecParametersPowerShell();
}

bool IKEv2IPSec::setIKEv2IPSecParametersInPhoneBook()
{
    CurrentUserImpersonationHelper impersonation_helper;
    if (!impersonation_helper.isImpersonated())
        return false;

    TCHAR pbk_path[MAX_PATH];
    for (const auto clsid : { CSIDL_APPDATA, CSIDL_LOCAL_APPDATA, CSIDL_COMMON_APPDATA }) {
        if (SHGetFolderPath(0, clsid, impersonation_helper.token(), SHGFP_TYPE_CURRENT, pbk_path))
            continue;
        wcscat_s(pbk_path, L"\\Microsoft\\Network\\Connections\\Pbk\\rasphone.pbk");
        // Check if the phonebook exists and is valid.
        auto pbk_handle = CreateFile(pbk_path, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                     FILE_ATTRIBUTE_NORMAL, nullptr);
        if (pbk_handle == INVALID_HANDLE_VALUE)
            continue;
        CloseHandle(pbk_handle);
        // Write custom IPSec parameters to the phonebook.
        if (!WritePrivateProfileString(
            kWindscribeConnectionName, L"NumCustomPolicy", L"1", pbk_path) ||
            !WritePrivateProfileString(
                kWindscribeConnectionName, L"CustomIPSecPolicies",
                L"020000000400000005000000080000000500000005000000", pbk_path)) {
            // This is a valid phonebook, but we cannot write it. Don't try other locations, they
            // won't make any sense; better to try other IPSec setup functions, like PowerShell.
            Logger::instance().out(L"Phonebook is not accessible: %ls", pbk_path);
            break;
        }
        return true;
    }
    return false;
}

bool IKEv2IPSec::setIKEv2IPSecParametersPowerShell()
{
    TCHAR command_buffer[1024] = {};
    wsprintf(command_buffer, L"PowerShell -Command \"Set-VpnConnectionIPsecConfiguration"
        " -ConnectionName '%ls'"
        " -AuthenticationTransformConstants GCMAES256 -CipherTransformConstants GCMAES256"
        " -EncryptionMethod AES256 -IntegrityCheckMethod SHA256"
        " -DHGroup ECP384 -PfsGroup ECP384 -Force\"", kWindscribeConnectionName);

    CurrentUserImpersonationHelper impersonation_helper;
    if (!impersonation_helper.isImpersonated())
        return false;

    auto mpr = ExecuteCmd::instance().executeBlockingCmd(
        command_buffer, impersonation_helper.token());
    if (!mpr.success) {
        Logger::instance().out(L"Command failed: %ls", command_buffer);
        if (mpr.sizeOfAdditionalData)
            Logger::instance().out(L"Output: %ls", mpr.szAdditionalData);
    }
    mpr.clear();
    return mpr.success;
}
