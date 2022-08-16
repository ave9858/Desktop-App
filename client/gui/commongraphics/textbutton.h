#ifndef TEXTBUTTON_H
#define TEXTBUTTON_H

#include <QGraphicsObject>
#include <QVariantAnimation>
#include <QFont>
#include <QColor>
#include "clickablegraphicsobject.h"
#include "graphicresources/fontdescr.h"
#include "utils/textshadow.h"

namespace CommonGraphics {

class TextButton : public ClickableGraphicsObject
{
    Q_OBJECT
public:
    explicit TextButton(QString text, const FontDescr &fd, QColor color, bool bSetClickable, ScalableGraphicsObject *parent = nullptr, int addWidth = 0, bool bDrawWithShadow = false);

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    double getOpacity() const;
    void quickSetOpacity(double opacity);

    void quickHide();
    void animateShow(int animationSpeed);
    void animateHide(int animationSpeed);
    void unhover();

    QFont getFont() const;
    int getWidth() const;
    void setText(QString text);
    QString text() const;
    void setColor(QColor color);

    void setCurrentOpacity(double opacity);
    void setUnhoverOpacity(double unhoverOpacity);

    void recalcBoundingRect();

    void setTextAlignment(int alignment);

protected:
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;

private slots:
    void onTextHoverOpacityChanged(const QVariant &value);

private:
    static constexpr int MARGIN_HEIGHT = 5;
    QString text_;
    QColor color_;
    FontDescr fontDescr_;
    QScopedPointer<TextShadow> textShadow_;

    int width_;
    int height_;
    int addWidth_;   // HACK - allows full displaying of text in Upgrade Widget

    double curTextOpacity_;
    QVariantAnimation textOpacityAnimation_;

    double unhoverOpacity_;
    bool isHovered_;

    int textAlignment_;
};

}
#endif // TEXTBUTTON_H
