#ifndef EXPORTDIALOG_H
#define EXPORTDIALOG_H

#include <QDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    enum class ExportType {
        Video,
        GIF,
        Image
    };

    explicit ExportDialog(ExportType type, QWidget *parent = nullptr);

    // Getters for export settings
    int width() const;
    int height() const;
    int fps() const;
    QString format() const;
    int quality() const;
    bool transparentBackground() const;
    int startFrame() const;
    int endFrame() const;
    bool exportAllFrames() const;

    // Setters for default values
    void setDefaultResolution(int width, int height);
    void setDefaultFPS(int fps);
    void setFrameRange(int start, int end);

private slots:
    void updatePreview();
    void onFormatChanged(const QString &format);

private:
    void setupUI();
    void createVideoSettings();
    void createGIFSettings();
    void createImageSettings();
    void applyModernStyle();

    ExportType m_type;

    // UI Components
    QSpinBox *m_widthSpinBox;
    QSpinBox *m_heightSpinBox;
    QSpinBox *m_fpsSpinBox;
    QComboBox *m_formatComboBox;
    QSlider *m_qualitySlider;
    QLabel *m_qualityLabel;
    QCheckBox *m_transparentBgCheckBox;
    QSpinBox *m_startFrameSpinBox;
    QSpinBox *m_endFrameSpinBox;
    QCheckBox *m_allFramesCheckBox;
    QLabel *m_previewLabel;
    QPushButton *m_resetButton;

    // Defaults
    int m_defaultWidth;
    int m_defaultHeight;
    int m_defaultFPS;
    int m_maxFrame;
};

#endif // EXPORTDIALOG_H
