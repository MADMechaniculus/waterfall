#ifndef WATERFALLVIEWER_H
#define WATERFALLVIEWER_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFileInfo>
#include <QString>
#include <QSettings>

#include "dsp.hpp"
#include "QCustomPlot/qcustomplot.h"
#include "consoleform.h"

#include <fstream>
#include <algorithm>
#include <iostream>

QT_BEGIN_NAMESPACE
namespace Ui { class WaterfallViewer; }
QT_END_NAMESPACE

class WaterfallViewer : public QMainWindow
{
    Q_OBJECT

    ConsoleForm * console;
    QCPColorScale * colorScale;

    bool selectionMode = false;

    uint32_t clickCounter = 0;

    std::pair<double, double> fPoint;
    std::pair<double, double> sPoint;

    std::vector<QCPColorMap *> colorMaps;

    const double Fs = 1100e6;
    double ts = 0.0;
    double fftResolution = 0.0;

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

private:
    Ui::WaterfallViewer *ui;
};
#endif // WATERFALLVIEWER_H
