#include "splittunnelingappsgroup.h"

#include <QFileInfo>
#include <QPainter>
#include "commongraphics/commongraphics.h"
#include "graphicresources/fontmanager.h"
#include "preferenceswindow/preferencegroup.h"
#include "utils/logger.h"
#include "utils/utils.h"
#include "dpiscalemanager.h"

#ifdef Q_OS_WIN
    #include "utils/winutils.h"
#else
    #include "utils/macutils.h"
#endif

namespace PreferencesWindow {

SplitTunnelingAppsGroup::SplitTunnelingAppsGroup(ScalableGraphicsObject *parent, const QString &desc, const QString &descUrl)
  : PreferenceGroup(parent, desc, descUrl), mode_(OP_MODE::DEFAULT)
{
    setFlag(QGraphicsItem::ItemIsFocusable);

    splitTunnelingAppsItem_ = new SplitTunnelingAppsItem(this);
    connect(splitTunnelingAppsItem_, &SplitTunnelingAppsItem::addClicked, this, &SplitTunnelingAppsGroup::addClicked);
    connect(splitTunnelingAppsItem_, &SplitTunnelingAppsItem::searchClicked, this, &SplitTunnelingAppsGroup::onSearchClicked);
    addItem(splitTunnelingAppsItem_);

    searchLineEditItem_ = new SearchLineEditItem(this);
    connect(searchLineEditItem_, &SearchLineEditItem::textChanged, this, &SplitTunnelingAppsGroup::onSearchTextChanged);
    connect(searchLineEditItem_, &SearchLineEditItem::searchModeExited, this, &SplitTunnelingAppsGroup::onSearchModeExited);
    connect(searchLineEditItem_, &SearchLineEditItem::focusIn, this, &SplitTunnelingAppsGroup::onSearchBoxFocusIn);
    addItem(searchLineEditItem_);

    hideItems(indexOf(searchLineEditItem_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    populateSearchApps();
}

QList<types::SplitTunnelingApp> SplitTunnelingAppsGroup::apps()
{
    return apps_.values();
}

void SplitTunnelingAppsGroup::setApps(QList<types::SplitTunnelingApp> apps)
{
    for (AppIncludedItem *item : apps_.keys())
    {
        apps_.remove(item);
        item->deleteLater();
    }

    for (types::SplitTunnelingApp app : apps)
    {
        addAppInternal(app);
    }
    emit appsUpdated(apps_.values());
}

void SplitTunnelingAppsGroup::addApp(types::SplitTunnelingApp &app)
{
    addAppInternal(app);
    emit appsUpdated(apps_.values());
}

void SplitTunnelingAppsGroup::addAppInternal(types::SplitTunnelingApp &app)
{
    QString iconPath = app.fullName;

#if defined Q_OS_MAC
    iconPath = MacUtils::iconPathFromBinPath(iconPath);
#endif

    AppIncludedItem *item = new AppIncludedItem(app, iconPath, this);
    connect(item, &AppIncludedItem::deleteClicked, this, &SplitTunnelingAppsGroup::onDeleteClicked);
    apps_[item] = app;

    addItem(item);
    hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    if (mode_ == OP_MODE::DEFAULT)
    {
        showItems(indexOf(item));
    }
}

void SplitTunnelingAppsGroup::addSearchApp(types::SplitTunnelingApp &app)
{
    QString iconPath = app.fullName;

#if defined Q_OS_MAC
    iconPath = MacUtils::iconPathFromBinPath(iconPath);
#endif

    AppSearchItem *item = new AppSearchItem(app, iconPath, this);
    item->setClickable(true);
    connect(item, &AppSearchItem::clicked, this, &SplitTunnelingAppsGroup::onSearchItemClicked);
    searchApps_[item] = app;

    addItem(item);
    hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    if (mode_ == OP_MODE::SEARCH)
    {
        showItems(indexOf(item));
    }
}

void SplitTunnelingAppsGroup::onSearchClicked()
{
    mode_ = OP_MODE::SEARCH;

    // hide everything
    hideItems(0, size() - 1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    // show search bar and search items
    showItems(indexOf(searchLineEditItem_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    showFilteredSearchItems(searchLineEditItem_->text());

    searchLineEditItem_->setFocusOnSearchBar();
}

void SplitTunnelingAppsGroup::onDeleteClicked()
{
    AppIncludedItem *item = static_cast<AppIncludedItem *>(sender());
    apps_.remove(item);
    hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_DELETE_AFTER);
    emit appsUpdated(apps_.values());
}

void SplitTunnelingAppsGroup::onSearchTextChanged(QString text)
{
    showFilteredSearchItems(text);
    searchLineEditItem_->setSelected(true);

    if (text == "")
    {
        searchLineEditItem_->hideButtons();
    }
    else
    {
        searchLineEditItem_->showButtons();
    }
}

void SplitTunnelingAppsGroup::onSearchModeExited()
{
    mode_ = OP_MODE::DEFAULT;

    searchLineEditItem_->setText("");

    // hide everything
    hideItems(0, size() - 1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);

    // show main bar and active items
    showItems(indexOf(splitTunnelingAppsItem_), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    for (BaseItem *item : apps_.keys())
    {
        showItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
    }

    // take focus away from the search bar
    setFocus(Qt::PopupFocusReason);
}

void SplitTunnelingAppsGroup::onSearchBoxFocusIn()
{
    searchLineEditItem_->setSelected(true);
}

void SplitTunnelingAppsGroup::populateSearchApps()
{
    searchApps_.clear();

#ifdef Q_OS_WIN
    const auto runningPrograms = WinUtils::enumerateRunningProgramLocations();
    for (const QString &exePath : runningPrograms)
    {
        if (!exePath.contains("C:\\Windows")
                && !exePath.contains("Windscribe.exe"))
        {
            QFile f(exePath);
            QString name = QFileInfo(f).fileName();

            types::SplitTunnelingApp app;
            app.name = name;
            app.type = SPLIT_TUNNELING_APP_TYPE_SYSTEM;
            app.fullName = exePath;
            addSearchApp(app);
        }
    }
#elif defined Q_OS_MAC
    const auto installedPrograms = MacUtils::enumerateInstalledPrograms();
    for (const QString &binPath : installedPrograms)
    {
        if (!binPath.contains("Windscribe"))
        {
            QFile f(binPath);
            QString name = QFileInfo(f).fileName();

            types::SplitTunnelingApp app;
            app.name = name;
            app.type = SPLIT_TUNNELING_APP_TYPE_SYSTEM;
            app.fullName = binPath;
            addSearchApp(app);
        }
    }
#endif
}

void SplitTunnelingAppsGroup::onSearchItemClicked()
{
    AppSearchItem *item = static_cast<AppSearchItem*>(sender());

    if (item != nullptr)
    {
        onSearchModeExited();
        toggleAppItemActive(item);
    }
}

void SplitTunnelingAppsGroup::toggleAppItemActive(AppSearchItem *item)
{
    QString appName = item->getName();
    AppIncludedItem *existingApp = appByName(appName);

    if (!existingApp)
    {
        types::SplitTunnelingApp app;
        app.name = appName;
        app.type = SPLIT_TUNNELING_APP_TYPE_SYSTEM;
        app.active = true;
        app.fullName = item->getFullName();
        addAppInternal(app);
    }

    emit appsUpdated(apps_.values());
}

AppIncludedItem *SplitTunnelingAppsGroup::appByName(QString name)
{
    for (AppIncludedItem *item : apps_.keys())
    {
        if (apps_[item].name == name)
        {
            return item;
        }
    }
    return nullptr;
}

void SplitTunnelingAppsGroup::showFilteredSearchItems(QString filter)
{
    if (OP_MODE::SEARCH)
    {
        for (AppSearchItem *item : searchApps_.keys())
        {
            if (filter == "" || (searchApps_[item].name.toLower().contains(filter.toLower()) && !appByName(searchApps_[item].name)))
            {
                showItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
            }
            else
            {
                hideItems(indexOf(item), -1, DISPLAY_FLAGS::FLAG_NO_ANIMATION);
            }
        }
    }
}

void SplitTunnelingAppsGroup::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Escape)
    {
        if (mode_ == OP_MODE::SEARCH)
        {
            if (searchLineEditItem_->text() == "")
            {
                onSearchModeExited();
            }
            else
            {
                searchLineEditItem_->setText("");
                emit escape();
            }
        }
        else
        {
            emit escape();
        }
    }
}

void SplitTunnelingAppsGroup::setLoggedIn(bool loggedIn)
{
    splitTunnelingAppsItem_->setClickable(loggedIn);
    searchLineEditItem_->setClickable(loggedIn);
    if (mode_ == OP_MODE::SEARCH)
    {
        onSearchModeExited();
    }

    for (AppIncludedItem *item : apps_.keys())
    {
        item->setClickable(loggedIn);
    }
}

}
