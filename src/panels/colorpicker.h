#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <QWidget>
#include <QColor>
#include <QSlider>
#include <QLineEdit>
#include <QVector>
#include <QComboBox>
#include <QBitmap>

class ColorWheel : public QWidget
{
    Q_OBJECT

public:
    explicit ColorWheel(QWidget *parent = nullptr);

    QColor currentColor() const { return m_currentColor; }
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void updateColor(const QPoint &pos);
    QPointF hueToPoint(qreal hue) const;
    QPointF svToPoint(qreal saturation, qreal value) const;

    QColor m_currentColor;
    QPixmap m_wheelPixmap;
    QPixmap m_trianglePixmap;
    bool m_isSelectingHue;
    bool m_isSelectingSV;
    QPointF m_center;
    qreal m_outerRadius;
    qreal m_innerRadius;
};

class ColorPicker : public QWidget
{
    Q_OBJECT

public:
    explicit ColorPicker(QWidget *parent = nullptr);

    QColor currentColor() const;
    void setColor(const QColor &color);

signals:
    void colorChanged(const QColor &color);
    void textureChanged(int textureType);

private slots:
    void onWheelColorChanged(const QColor &color);
    void onOpacityChanged(int value);
    void onHexChanged();
    void onRecentColorClicked();

private:
    void setupUI();
    void updateUI();
    void addToRecentColors(const QColor &color);
    QPixmap generateTextureIcon(int textureType);

    ColorWheel *m_colorWheel;
    QSlider *m_opacitySlider;
    QLineEdit *m_hexInput;
    QComboBox *m_textureCombo;
    QVector<QColor> m_recentColors;
    QWidget *m_recentColorsWidget;
    QColor m_currentColor;
};

#endif // COLORPICKER_H
