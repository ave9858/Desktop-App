#pragma once

#include <QObject>
#include <QSet>
#include "engine/apiinfo/staticips.h"

class IHelper;

// basic class FirewallController
// implement state changes logic
class FirewallController : public QObject
{
    Q_OBJECT
public:
    explicit FirewallController(QObject *parent);
    virtual ~FirewallController() {}

    // this function also uses for change firewall ips, then it is already enabled
    virtual bool firewallOn(const QString &connectingIp, const QSet<QString> &ips, bool bAllowLanTraffic, bool bIsCustomConfig);
    virtual bool firewallOff();
    virtual bool firewallActualState() = 0;

    virtual bool whitelistPorts(const apiinfo::StaticIpPortsVector &ports);
    virtual bool deleteWhitelistPorts();

    // Mac/Linux specific functions
    virtual void setInterfaceToSkip_posix(const QString &interfaceToSkip) = 0;
    virtual void enableFirewallOnBoot(bool bEnable, const QSet<QString>& ipTable = QSet<QString>()) = 0;

protected:
    bool isStateChanged();

    QString latestConnectingIp_;
    QSet<QString> latestIps_;
    bool latestAllowLanTraffic_;
    bool latestEnabledState_;
    bool latestIsCustomConfig_;
    apiinfo::StaticIpPortsVector latestStaticIpPorts_;
    bool bInitialized_;
    bool bStateChanged_;
};
