#ifndef CHECKBOXBUTTON_H
#define CHECKBOXBUTTON_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include "clickablegraphicsobject.h"

namespace PreferencesWindow {

class CheckBoxButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit CheckBoxButton(ScalableGraphicsObject *parent = nullptr);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setState(bool isChecked);
    bool isChecked() const;
    void setEnabled(bool enabled);

signals:
    void stateChanged(bool isChecked);

protected:
    static constexpr int TOGGLE_RADIUS = 12;
    static constexpr int BUTTON_OFFSET = 2;
    static constexpr int BUTTON_WIDTH = 18;
    static constexpr int BUTTON_HEIGHT = 18;

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onOpacityChanged(const QVariant &value);

private:
    double animationProgress_;
    bool isChecked_;
    bool enabled_;
    QVariantAnimation opacityAnimation_;
};

} // namespace PreferencesWindow

#endif // CHECKBOXBUTTON_H
