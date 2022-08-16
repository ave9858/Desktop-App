#include "tabbutton.h"

#include <QGraphicsView>
#include <QPainter>
#include <QPoint>
#include "commongraphics/commongraphics.h"
#include "dpiscalemanager.h"
#include "graphicresources/imageresourcessvg.h"
#include "tooltips/tooltiptypes.h"
#include "tooltips/tooltipcontroller.h"
#include "utils/logger.h"

namespace PreferencesWindow {

TabButton::TabButton(ScalableGraphicsObject *parent, QString text, PREFERENCES_TAB_TYPE type, QString path, uint32_t flags)
    : ClickableGraphicsObject(parent), type_(type), icon_(ImageResourcesSvg::instance().getIndependentPixmap(path)),
      text_(text), circleOpacity_(OPACITY_UNHOVER_CIRCLE), iconOpacity_(OPACITY_HALF), flags_(flags)
{
    setClickable(true);
    connect(this, &TabButton::clicked, this, &TabButton::onClicked);

    connect(&iconOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TabButton::onIconOpacityChanged);
    connect(&circleOpacityAnimation_, &QVariantAnimation::valueChanged, this, &TabButton::onCircleOpacityChanged);

    setResetHoverOnClick(false);
    connect(this, &TabButton::hoverEnter, this, &TabButton::onHoverEnter);
    connect(this, &TabButton::hoverLeave, this, &TabButton::onHoverLeave);
}

QRectF TabButton::boundingRect() const
{
    return QRectF(0, 0, BUTTON_WIDTH*G_SCALE, BUTTON_HEIGHT*G_SCALE);
}

void TabButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    painter->setRenderHint(QPainter::Antialiasing);

    // circle background
    painter->setOpacity(circleOpacity_);
    painter->setBrush(Qt::white);
    painter->drawEllipse(0, 0, BUTTON_WIDTH*G_SCALE, BUTTON_HEIGHT*G_SCALE);

    // icon
    if (!(flags_ & TAB_BUTTON_FULL_OPACITY))
    {
        painter->setOpacity(iconOpacity_);
        double percent = (iconOpacity_ - OPACITY_HALF) / OPACITY_HALF;
        QColor iconColor(static_cast<int>(255 - percent * (255 - ICON_SELECTED_COLOR_RED)),
                         static_cast<int>(255 - percent * (255 - ICON_SELECTED_COLOR_GREEN)),
                         static_cast<int>(255 - percent * (255 - ICON_SELECTED_COLOR_BLUE)));
        icon_->draw(ICON_MARGIN*G_SCALE, ICON_MARGIN*G_SCALE, ICON_WIDTH*G_SCALE, ICON_HEIGHT*G_SCALE, painter, iconColor);
    }
    else
    {
        painter->setOpacity(1.0);
        icon_->draw(ICON_MARGIN*G_SCALE, ICON_MARGIN*G_SCALE, painter);
    }
}

void TabButton::updateScaling()
{
    ScalableGraphicsObject::updateScaling();
}

void TabButton::setSelected(bool selected)
{
    selected_ = selected;
    hovered_ = selected;

    if (selected)
    {
        onHoverEnter();
    }
    else if (!stickySelection_)
    {
        onHoverLeave();
    }
}

void TabButton::setText(QString text)
{
    text_ = text;
}

void TabButton::onIconOpacityChanged(const QVariant &value)
{
    iconOpacity_ = value.toDouble();
    update();
}

void TabButton::onCircleOpacityChanged(const QVariant &value)
{
    circleOpacity_ = value.toDouble();
    update();
}

void TabButton::onClicked()
{
    emit tabClicked(type_, this);
}

void TabButton::onHoverEnter()
{
    startAnAnimation<double>(iconOpacityAnimation_, iconOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    startAnAnimation<double>(circleOpacityAnimation_, circleOpacity_, OPACITY_FULL, ANIMATION_SPEED_FAST);
    showTooltip();
}

void TabButton::onHoverLeave()
{
    if (!selected_)
    {
        startAnAnimation<double>(iconOpacityAnimation_, iconOpacity_, OPACITY_HALF, ANIMATION_SPEED_FAST);
        startAnAnimation<double>(circleOpacityAnimation_, circleOpacity_, OPACITY_UNHOVER_CIRCLE, ANIMATION_SPEED_FAST);
    }
    hideTooltip();
}

void TabButton::showTooltip()
{
    if (scene() == nullptr)
    {
        return;
    }
    QGraphicsView *view = scene()->views().first();
    QPoint globalPt = view->mapToGlobal(view->mapFromScene(scenePos()));

    TooltipInfo ti(TOOLTIP_TYPE_BASIC, TOOLTIP_ID_PREFERENCES_TAB_INFO);
    ti.x = globalPt.x() + boundingRect().width() + ICON_MARGIN*G_SCALE;
    ti.y = globalPt.y() + boundingRect().height()/2;
    ti.title = text_;
    ti.tailtype =  TOOLTIP_TAIL_LEFT;
    ti.tailPosPercent = 0.5;
    TooltipController::instance().showTooltipBasic(ti);
}

void TabButton::hideTooltip()
{
    TooltipController::instance().hideTooltip(TOOLTIP_ID_PREFERENCES_TAB_INFO);
}

} // namespace PreferencesWindow