#ifndef WATERFALLVIEWER_H
#define WATERFALLVIEWER_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFileInfo>
#include <QString>
#include <QSettings>
#include <QLineEdit>
#include <QLabel>
#include <QMetaType>
#include <QProgressBar>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QImage>
#include <QFile>
#include <QKeyEvent>

#include "dsp.hpp"
#include "qcustomplot.h"
#include "consoleform.h"
#include "customtoolbar.h"
#include "colormapworker.h"
#include "utilitytoolbar.h"

#include <fstream>
#include <algorithm>
#include <iostream>
#include <deque>

QT_BEGIN_NAMESPACE
namespace Ui { class WaterfallViewer; }
QT_END_NAMESPACE

class FileListItem {
protected:
    QString filePath;

public:
    FileListItem(QString path) {
        this->filePath = path;
    }
    FileListItem(const FileListItem & other) {
        this->filePath = other.filePath;
    }
    FileListItem(const FileListItem && other) {
        this->filePath = other.filePath;
    }

    QString getFilePath() {
        return this->filePath;
    }
};

class WaterfallViewer : public QMainWindow
{
    Q_OBJECT

    size_t availThreads{0};

    QCPColorScale * colorScale;
    CustomToolBar * toolBar;
    UtilityToolBar * utilBar;

    bool selectionMode = false;

    uint32_t clickCounter = 0;

    std::pair<double, double> fPoint;
    std::pair<double, double> sPoint;

    std::vector<std::complex<float>> complexSignal;

    double Fs = 1100e6;
    double ts = 0.0;
    double fftOrder = 0.0;
    double fftResolution = 0.0;
    double scale = 0.0;

    QVector<double> dotGraphKeys;
    QVector<double> dotGraphVals;

    QCPGraph * dotGraph;

    QVector<ColorMapWorker*> workers;
    std::vector<FileListItem> filesVector;
    std::vector<ColorMapWorkerTask *> tasks;

    QCPColorMap * colorMap{nullptr};

    std::atomic<float> maxColorValue{0};

    QString selectedFile;

public:
    WaterfallViewer(QWidget *parent = nullptr);
    ~WaterfallViewer();

signals:

private slots:
    void on_actionOpen_file_triggered();
    void plotterMousePressSlot(QMouseEvent * event);
    void on_actionSelection_triggered(bool checked);

    void sampleRateChanged(const QString & text);
    void fftOrderChanged(const QString & text);
    void scaleFactorChanged(const QString & text);

    void onProcessingComplete(void);

    void updateFileList(void);
    void on_clearFileListButton_clicked();

    void on_reprocessButton_clicked();

    void on_fileList_currentRowChanged(int currentRow);

    void appendConsole(QString message);

    void on_actionGrayscale_triggered();
    void on_actionSpectrum_triggered();

private:
    Ui::WaterfallViewer *ui;

    void cleanPlotter(void);
    void colorMapCreation(void);
    void startProcessing(void);
    void updateColorScheme(void);

    void keyPressEvent(QKeyEvent *ev);
    void keyReleaseEvent(QKeyEvent *ev);
};
#endif // WATERFALLVIEWER_H
