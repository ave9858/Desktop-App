#ifndef HelperCommands_h
#define HelperCommands_h

#include <string>
#include <vector>

#define HELPER_CMD_EXECUTE                      0
#define HELPER_CMD_EXECUTE_OPENVPN              1
#define HELPER_CMD_GET_CMD_STATUS               2
#define HELPER_CMD_CLEAR_CMDS                   3
#define HELPER_CMD_SET_KEYCHAIN_ITEM            4
#define HELPER_CMD_SPLIT_TUNNELING_SETTINGS     5
#define HELPER_CMD_SEND_CONNECT_STATUS          6
#define HELPER_CMD_SET_KEXT_PATH                7
#define HELPER_CMD_START_WIREGUARD              8
#define HELPER_CMD_STOP_WIREGUARD               9
#define HELPER_CMD_CONFIGURE_WIREGUARD          10
#define HELPER_CMD_GET_WIREGUARD_STATUS         11


struct CMD_EXECUTE
{
    std::string cmdline;
};

struct CMD_EXECUTE_OPENVPN
{
    std::string cmdline;
};

struct CMD_GET_CMD_STATUS
{
    unsigned long cmdId;
};

struct CMD_CLEAR_CMDS
{
};

struct CMD_SET_KEYCHAIN_ITEM
{
    std::string username;
    std::string password;
};

struct CMD_SPLIT_TUNNELING_SETTINGS
{
    bool isActive;
    bool isExclude;     // true -> SPLIT_TUNNELING_MODE_EXCLUDE, false -> SPLIT_TUNNELING_MODE_INCLUDE

    std::vector<std::string> files;
    std::vector<std::string> ips;
    std::vector<std::string> hosts;
};

struct CMD_SEND_DEFAULT_ROUTE
{
    std::string gatewayIp;
    std::string interfaceName;
    std::string interfaceIp;
};

enum CMD_PROTOCOL_TYPE {
    CMD_PROTOCOL_IKEV2,
    CMD_PROTOCOL_OPENVPN,
    CMD_PROTOCOL_STUNNEL_OR_WSTUNNEL,
    CMD_PROTOCOL_WIREGUARD
};

struct CMD_SEND_CONNECT_STATUS
{
    bool isConnected;
    CMD_PROTOCOL_TYPE protocol;

    std::string gatewayIp;
    std::string interfaceName;
    std::string interfaceIp;
    
    // need for stunnel/wstunnel routing
    std::string connectedIp;

    // need for openvpn routing
    std::string routeVpnGateway;
    std::string routeNetGateway;
    std::string remote_1;
    std::string ifconfigTunIp;
    
    // common
    std::string vpnAdapterName;
    std::string vpnAdapterIp;
    
    std::vector<std::string> dnsServers;
};

struct CMD_SET_KEXT_PATH
{
    std::string kextPath;
};

struct CMD_START_WIREGUARD
{
    std::string exePath;
    std::string deviceName;
};

struct CMD_CONFIGURE_WIREGUARD
{
    std::string clientPrivateKey;
    std::string clientIpAddress;
    std::string clientDnsAddressList;
    std::string clientDnsScriptName;
    std::string peerPublicKey;
    std::string peerPresharedKey;
    std::string peerEndpoint;
    std::string allowedIps;
};

struct CMD_ANSWER
{
    unsigned long cmdId;        // unique ID of command
    int    executed;            // 0 - failed, 1 - executed, 2 - still executing (for openvpn)
    unsigned long long customInfoValue[2];
    std::string body;

    CMD_ANSWER() : cmdId(0), executed(0), customInfoValue(), body() {}
};

enum WireGuardServiceState
{
    WIREGUARD_STATE_NONE,       // WireGuard is not started.
    WIREGUARD_STATE_ERROR,      // WireGuard is in error state.
    WIREGUARD_STATE_STARTING,   // WireGuard is initializing/starting up.
    WIREGUARD_STATE_LISTENING,  // WireGuard is listening for UAPI commands, but not connected.
    WIREGUARD_STATE_CONNECTING, // WireGuard is configured and awaits for a handshake.
    WIREGUARD_STATE_ACTIVE,     // WireGuard is connected.
};

#endif
