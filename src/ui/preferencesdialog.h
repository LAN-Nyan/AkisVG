#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QComboBox>

class PreferencesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PreferencesDialog(QWidget *parent = nullptr);
    
    // Getters for settings
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
    
    QCheckBox *m_onionSkinEnabled;
    QSpinBox *m_onionSkinFrames;
    QSpinBox *m_onionSkinOpacity;
    QPushButton *m_onionSkinColorBtn;
    QColor m_onionSkinColor;
};

#endif // PREFERENCESDIALOG_H
