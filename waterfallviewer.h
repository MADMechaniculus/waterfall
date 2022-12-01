#ifndef WATERFALLVIEWER_H
#define WATERFALLVIEWER_H

#include <QMainWindow>
#include <QFileDialog>
#include <QFileInfo>

#include "dsp.hpp"
#include "QCustomPlot/qcustomplot.h"

#include <fstream>
#include <algorithm>
#include <iostream>

QT_BEGIN_NAMESPACE
namespace Ui { class WaterfallViewer; }
QT_END_NAMESPACE

class WaterfallViewer : public QMainWindow
{
    Q_OBJECT

public:
    WaterfallViewer(QWidget *parent = nullptr);
    ~WaterfallViewer();

private slots:
    void on_actionOpen_file_triggered();

private:
    Ui::WaterfallViewer *ui;
};
#endif // WATERFALLVIEWER_H
