#ifndef TEXTTOOL_H
#define TEXTTOOL_H

#include "tool.h"
#include <QFont>

// =========================================================
//  TextTool
//  Lets the user click on the canvas to place a text object.
//  Font family, size, bold, italic, and colour are all
//  configurable through the Tool Settings panel (see
//  ToolSettingsPanel).  The settings are stored here so the
//  canvas doesn't need to know about UI controls.
// =========================================================
class TextTool : public Tool
{
    Q_OBJECT

public:
    explicit TextTool(QObject *parent = nullptr);

    // Canvas events
    void mousePressEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event, VectorCanvas *canvas) override;

    // ── Font settings (read / write by ToolSettingsPanel) ──
    QString fontFamily()  const { return m_fontFamily;  }
    int     fontSize()    const { return m_fontSize;    }
    bool    bold()        const { return m_bold;        }
    bool    italic()      const { return m_italic;      }
    bool    underline()   const { return m_underline;   }
    Qt::Alignment alignment() const { return m_alignment; }

public slots:
    void setFontFamily(const QString &family);
    void setFontSize(int size);
    void setBold(bool b);
    void setItalic(bool i);
    void setUnderline(bool u);
    void setAlignment(Qt::Alignment align);

signals:
    void fontSettingsChanged();

private:
    QString       m_fontFamily = "Arial";
    int           m_fontSize   = 48;
    bool          m_bold       = false;
    bool          m_italic     = false;
    bool          m_underline  = false;
    Qt::Alignment m_alignment  = Qt::AlignLeft;
};

#endif // TEXTTOOL_H
