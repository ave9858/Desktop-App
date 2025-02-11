#include "networkdetectionmanager_mac.h"

#include <QRegularExpression>
#include "../networkdetectionmanager/reachabilityevents.h"
#include "utils/network_utils/network_utils_mac.h"
#include "utils/utils.h"
#include "utils/logger.h"

const int typeIdNetworkInterface = qRegisterMetaType<types::NetworkInterface>("types::NetworkInterface");

NetworkDetectionManager_mac::NetworkDetectionManager_mac(QObject *parent, IHelper *helper) : INetworkDetectionManager (parent)
    , helper_(helper)
    , lastWifiAdapterUp_(false)
{
    connect(&ReachAbilityEvents::instance(), &ReachAbilityEvents::networkStateChanged, this, &NetworkDetectionManager_mac::onNetworkStateChanged);
    lastNetworkList_ = NetworkUtils_mac::currentNetworkInterfaces(true);
    lastNetworkInterface_ = currentNetworkInterfaceFromNetworkList(lastNetworkList_);
    lastWifiAdapterUp_ = isWifiAdapterUp(lastNetworkList_);
    lastIsOnlineState_ = isOnlineImpl();
}

NetworkDetectionManager_mac::~NetworkDetectionManager_mac()
{
}

bool NetworkDetectionManager_mac::isOnline()
{
    QMutexLocker locker(&mutex_);
    return lastIsOnlineState_;
}

void NetworkDetectionManager_mac::onNetworkStateChanged()
{
    bool curIsOnlineState = isOnlineImpl();
    if (lastIsOnlineState_ != curIsOnlineState)
    {
        lastIsOnlineState_ = curIsOnlineState;
        emit onlineStateChanged(curIsOnlineState);
    }

    const QVector<types::NetworkInterface> &networkList = NetworkUtils_mac::currentNetworkInterfaces(true);
    const types::NetworkInterface &networkInterface = currentNetworkInterfaceFromNetworkList(networkList);
    bool wifiAdapterUp = isWifiAdapterUp(networkList);

    if (networkInterface != lastNetworkInterface_)
    {
        if (networkInterface.interfaceName != lastNetworkInterface_.interfaceName)
        {
            if (networkInterface.interfaceIndex == -1)
            {
                qCDebug(LOG_BASIC) << "Primary Adapter down: " << lastNetworkInterface_.interfaceName;
                emit primaryAdapterDown(lastNetworkInterface_);
            }
            else if (lastNetworkInterface_.interfaceIndex == -1)
            {
                qCDebug(LOG_BASIC) << "Primary Adapter up: " << networkInterface.interfaceName;
                emit primaryAdapterUp(networkInterface);
            }
            else
            {
                qCDebug(LOG_BASIC) << "Primary Adapter changed: " << networkInterface.interfaceName;
                emit primaryAdapterChanged(networkInterface);
            }
        }
        else if (networkInterface.networkOrSsid != lastNetworkInterface_.networkOrSsid)
        {
            qCDebug(LOG_BASIC) << "Primary Network Changed: "
                               << networkInterface.interfaceName
                               << " : " << networkInterface.networkOrSsid;

            // if network change or adapter down (no comeup)
            if (lastNetworkInterface_.networkOrSsid != "")
            {
                qCDebug(LOG_BASIC) << "Primary Adapter Network changed or lost";
                emit primaryAdapterNetworkLostOrChanged(networkInterface);
            }
        }
        else
        {
            qCDebug(LOG_BASIC) << "Unidentified interface change";
            // Can happen when changing interfaces
        }

        lastNetworkInterface_ = networkInterface;
        emit networkChanged(networkInterface);
    }
    else if (wifiAdapterUp != lastWifiAdapterUp_)
    {
        if (NetworkUtils_mac::isWifiAdapter(networkInterface.interfaceName)
                || NetworkUtils_mac::isWifiAdapter(lastNetworkInterface_.interfaceName))
        {
            qCDebug(LOG_BASIC) << "Wifi adapter (primary) up state changed: " << wifiAdapterUp;
            emit wifiAdapterChanged(wifiAdapterUp);
        }
    }
    else if (networkList != lastNetworkList_)
    {
        qCDebug(LOG_BASIC) << "Network list changed";
        emit networkListChanged(networkList);
    }

    lastNetworkList_ = networkList;
    lastWifiAdapterUp_ = wifiAdapterUp;
}

bool NetworkDetectionManager_mac::isWifiAdapterUp(const QVector<types::NetworkInterface> &networkList)
{
    for (int i = 0; i < networkList.size(); ++i)
    {
        if (networkList[i].interfaceType == NETWORK_INTERFACE_WIFI)
        {
            return true;
        }
    }
    return false;
}

const types::NetworkInterface NetworkDetectionManager_mac::currentNetworkInterfaceFromNetworkList(const QVector<types::NetworkInterface> &networkList)
{
    // we assume that the first non-empty adapter is a current network interface
    for (int i = 0; i < networkList.size(); ++i)
    {
        if (networkList[i].interfaceIndex != -1)
        {
            return networkList[i];
        }
    }

    return Utils::noNetworkInterface();
}

bool NetworkDetectionManager_mac::isOnlineImpl()
{
    QString command = "netstat -nr -f inet | sed '1,3 d' | awk 'NR==1 { for (i=1; i<=NF; i++) { f[$i] = i  } } NR>1 && $(f[\"Destination\"])==\"default\" { print $(f[\"Gateway\"]), $(f[\"Netif\"]) ; exit }'";
    QString strReply = Utils::execCmd(command).trimmed();
    return !strReply.isEmpty();
}


void NetworkDetectionManager_mac::getCurrentNetworkInterface(types::NetworkInterface &networkInterface)
{
    networkInterface = lastNetworkInterface_;
}
