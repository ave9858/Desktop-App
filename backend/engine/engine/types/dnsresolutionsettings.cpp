#include "dnsresolutionsettings.h"
#include "Utils/logger.h"

DnsResolutionSettings::DnsResolutionSettings() : isInitialized_(false)
{
}

DnsResolutionSettings::DnsResolutionSettings(const ProtoTypes::ApiResolution &d)
{
    bAutomatic_ = d.is_automatic();
    manualIp_ = QString::fromStdString(d.manual_ip());
    isInitialized_ = true;
}

void DnsResolutionSettings::set(bool bAutomatic, const QString &manualIp)
{
    bAutomatic_ = bAutomatic;
    manualIp_ = manualIp;
    isInitialized_ = true;
}

void DnsResolutionSettings::debugToLog()
{
    Q_ASSERT(isInitialized_);
    if (bAutomatic_)
    {
        qCDebug(LOG_BASIC) << "DNS mode: automatic";
    }
    else
    {
        qCDebug(LOG_BASIC) << "DNS mode: manual";
        qCDebug(LOG_BASIC) << "DNS ip:" << manualIp_;
    }
}

ProtoTypes::ApiResolution DnsResolutionSettings::convertToProtobuf() const
{
    Q_ASSERT(isInitialized_);
    ProtoTypes::ApiResolution drs;
    drs.set_is_automatic(bAutomatic_);
    drs.set_manual_ip(manualIp_.toStdString());
    return drs;
}

bool DnsResolutionSettings::isEqual(const DnsResolutionSettings &other) const
{
    Q_ASSERT(isInitialized_);
    Q_ASSERT(other.isInitialized_);
    return bAutomatic_ == other.bAutomatic_ && manualIp_ == other.manualIp_;
}

bool DnsResolutionSettings::getIsAutomatic() const
{
    Q_ASSERT(isInitialized_);
    return bAutomatic_;
}

QString DnsResolutionSettings::getManualIp() const
{
    Q_ASSERT(isInitialized_);
    return manualIp_;
}
