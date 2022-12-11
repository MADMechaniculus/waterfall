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

class WaterfallViewer : public QMainWindow
{
    Q_OBJECT

    size_t availThreads{0};

    ConsoleForm * console;
    QCPColorScale * colorScale;
    CustomToolBar * toolBar;
    UtilityToolBar * utilBar;

    bool selectionMode = false;

    uint32_t clickCounter = 0;

    std::pair<double, double> fPoint;
    std::pair<double, double> sPoint;

    std::vector<QCPColorMap *> colorMaps;
    std::vector<std::complex<float>> complexSignal;

    double Fs = 1100e6;
    double ts = 0.0;
    double fftOrder = 0.0;
    double fftResolution = 0.0;
    uint32_t scale = 0;

    QVector<double> dotGraphKeys;
    QVector<double> dotGraphVals;

    QCPGraph * dotGraph;

    QVector<ColorMapWorker*> workers;

public:
    WaterfallViewer(QWidget *parent = nullptr);
    ~WaterfallViewer();

signals:
    void printConsole(QString message);

private slots:
    void on_actionOpen_file_triggered();
    void plotterMousePressSlot(QMouseEvent * event);
    void on_actionOpen_triggered();
    void on_actionSelection_triggered(bool checked);

    void sampleRateChanged(const QString & text);
    void fftOrderChanged(const QString & text);
    void scaleFactorChanged(const QString & text);

private:
    Ui::WaterfallViewer *ui;

    void cleanPlotter(void);
};
#endif // WATERFALLVIEWER_H
