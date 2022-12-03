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

#include <fstream>
#include <algorithm>
#include <iostream>

QT_BEGIN_NAMESPACE
namespace Ui { class WaterfallViewer; }
QT_END_NAMESPACE

class CustomToolBar : public QObject {
    Q_OBJECT

    QLineEdit * sampleRate;
    QLineEdit * fftOrder;
    QLineEdit * scaleFactor;
    QProgressBar * progress;

public:
    CustomToolBar(QObject * parent) : QObject(parent) {
        this->sampleRate = new QLineEdit();
        this->sampleRate->setText("1100e6");
        this->fftOrder = new QLineEdit();
        this->fftOrder->setText("10");
        this->scaleFactor = new QLineEdit();
        this->scaleFactor->setText("8");

        this->progress = new QProgressBar();
        this->progress->setMinimum(0);
        this->progress->setMaximum(100);
        this->progress->setValue(100);

        connect(sampleRate, &QLineEdit::textChanged, this, &CustomToolBar::onSampleRate_TextChanged);
        connect(fftOrder, &QLineEdit::textChanged, this, &CustomToolBar::onFFTOrder_TextChanged);
        connect(scaleFactor, &QLineEdit::textChanged, this, &CustomToolBar::onScaleFactor_TextChanged);
    }
    ~CustomToolBar() {}

    void draw(QToolBar * rootBar) {
        rootBar->addWidget(new QLabel("Sample rate"));
        rootBar->addWidget(this->sampleRate);
        rootBar->addSeparator();
        rootBar->addWidget(new QLabel("FFT order"));
        rootBar->addWidget(this->fftOrder);
        rootBar->addSeparator();
        rootBar->addWidget(new QLabel("Scale factor"));
        rootBar->addWidget(this->scaleFactor);
        rootBar->addSeparator();
        rootBar->addWidget(this->progress);
    }

    void emitAll(void) {
        emit this->sampleRate->textChanged(sampleRate->text());
        emit this->fftOrder->textChanged(fftOrder->text());
        emit this->scaleFactor->textChanged(scaleFactor->text());
    }

signals:
    void onSampleRate_TextChanged(const QString & text);
    void onFFTOrder_TextChanged(const QString & text);
    void onScaleFactor_TextChanged(const QString & text);

protected:

};

class WaterfallViewer : public QMainWindow
{
    Q_OBJECT

    ConsoleForm * console;
    QCPColorScale * colorScale;
    CustomToolBar * toolBar;

    bool selectionMode = false;

    uint32_t clickCounter = 0;

    std::pair<double, double> fPoint;
    std::pair<double, double> sPoint;

    std::vector<QCPColorMap *> colorMaps;

    double Fs = 1100e6;
    double ts = 0.0;
    double fftOrder = 0.0;
    double fftResolution = 0.0;
    uint32_t scale = 0;

    QVector<double> dotGraphKeys;
    QVector<double> dotGraphVals;

    QCPGraph * dotGraph;

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
