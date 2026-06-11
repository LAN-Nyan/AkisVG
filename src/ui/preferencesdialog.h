#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QMap>
#include <QKeySequenceEdit>
#include "tools/tool.h"

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);

    bool onionSkinEnabled() const;
    int onionSkinFrames() const;
    QColor onionSkinColor() const;
    int onionSkinOpacity() const;

signals:
    void settingsChanged();

private:
    void setupUI();
    void loadSettings();
    void saveSettings();

    QCheckBox *m_onionSkinEnabled = nullptr;
    QSpinBox *m_onionSkinFrames = nullptr;
    QSpinBox *m_onionSkinOpacity = nullptr;
    QPushButton *m_onionSkinColorBtn = nullptr;
    QColor m_onionSkinColor;
    QMap<ToolType, QKeySequenceEdit *> m_hotkeyEdits;
};

#endif
