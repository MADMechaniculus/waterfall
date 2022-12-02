#include "waterfallviewer.h"
#include "./ui_waterfallviewer.h"

WaterfallViewer::WaterfallViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WaterfallViewer)
{
    ui->setupUi(this);

    this->console = new ConsoleForm(this);
    connect(this, &WaterfallViewer::printConsole, console, &ConsoleForm::appendConsole);

    QCustomPlot * customPlot = this->ui->plotter;
    //    customPlot->setOpenGl(true);

    connect(this->ui->plotter, &QCustomPlot::mousePress, this, &WaterfallViewer::plotterMousePressSlot);

    // configure axis rect:
    customPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    customPlot->axisRect()->setupFullAxesBox(true);
    customPlot->xAxis->setLabel("Frequency");
    customPlot->yAxis->setLabel("Time");

    colorScale = new QCPColorScale(this->ui->plotter);
    this->ui->plotter->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    colorScale->axis()->setLabel("Signal amplitude");
}

WaterfallViewer::~WaterfallViewer()
{
    if (!this->colorMaps.empty()) {
        std::for_each(std::begin(this->colorMaps), std::end(this->colorMaps), [](QCPColorMap * item) {
            if (item != nullptr) {
                delete item;
            }
        });

        this->colorMaps.clear();
    }

    delete ui;
}


void WaterfallViewer::on_actionOpen_file_triggered()
{
    this->ui->plotter->clearPlottables();

    const uint32_t windowSize = std::pow(2, 10);
    fftResolution = Fs / 2.0 / (double)windowSize;
    const uint32_t scale = 8;

#ifdef WIN32
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open record"), "C:\\", \
                                                    tr("Record files (*.bin *.dat *.pcm *.iq16)"));
#elif __unix__
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open record"), "/home/jana", \
                                                    tr("Record files (*.bin *.dat *.pcm *.iq16)"));
#else
#error What is this operating system?
#endif

    if (fileName.isEmpty()) {
        return;
    }

    QFileInfo fileInfo(fileName);

    std::vector<iq16_t> signal(fileInfo.size() / sizeof (iq16_t));
    std::vector<std::complex<float>> complexSignal(windowSize);
    std::vector<std::complex<float>> complexFFTRes(windowSize);
    std::vector<float> windowData(windowSize);

    std::ifstream readFile(fileName.toStdString(), std::ios::binary);
    if (!readFile.is_open()) {
        this->ui->statusbar->showMessage("Error on opening read stream");
        return;
    }

    readFile.read((char*)signal.data(), fileInfo.size());
    readFile.close();

    uint64_t verticalSize = signal.size() / 4;
    uint64_t horizontalSize = windowSize;

    if (!this->colorMaps.empty()) {
        std::for_each(std::begin(this->colorMaps), std::end(this->colorMaps), [](QCPColorMap * item) {
            if (item != nullptr) {
                delete item;
            }
        });

        this->colorMaps.clear();
    }

    ts = (double)2 / Fs * (double)windowSize / (double)scale;

//    size_t blocks = verticalSize % (windowSize / scale);
//    blocks = (verticalSize - blocks) / (windowSize / scale);
//    size_t oneStep = (windowSize / scale);

//    if (std::thread::hardware_concurrency() > 4) {
//        std::vector<std::thread> threadPool(4);

//        std::array<size_t, 4> startIndexes;

//        QString message = QString("Start indexes: ");
//        for (size_t i = 0; i < startIndexes.size(); i++) {
//            startIndexes.at(i) = blocks / threadPool.size() * i;
//            message += QString::number(startIndexes.at(i));
//            if (i != startIndexes.size() - 1) {
//                message += ", ";
//            }
//        }

//        emit printConsole(message);
//        emit printConsole("Total blocks: " + QString::number(blocks));

//        this->colorMaps.resize(blocks);

//        for (size_t i = 0; i < threadPool.size(); i++) {
//            size_t startIndex = startIndexes.at(i);
//            size_t nextIndex = 0;
//            if (i != threadPool.size() - 1)
//                nextIndex = startIndexes.at(i + 1);
//            else
//                nextIndex = blocks;

//            threadPool.at(i) = std::thread([this, startIndex, nextIndex, oneStep, windowSize, horizontalSize, &signal]() {
//                std::vector<std::complex<float>> complexSignal(windowSize);
//                std::vector<std::complex<float>> complexFFTRes(windowSize);
//                std::vector<float> windowData(windowSize);

//                for (size_t i = startIndex; i < nextIndex; i++) {

//                    size_t offset = i * oneStep;

//                    this->colorMaps.at(i) = new QCPColorMap(this->ui->plotter->xAxis, this->ui->plotter->yAxis);
//                    QCPColorMap * waterfallMap = this->colorMaps.back();
//                    if (waterfallMap == nullptr) {
//                        this->ui->statusbar->showMessage("No more color maps...");
//                        break;
//                    }

//                    waterfallMap->data()->setSize(horizontalSize, 1);
//                    waterfallMap->data()->setRange(QCPRange(0, horizontalSize), QCPRange(i, i + 1));

//                    std::transform(std::begin(signal) + i, std::begin(signal) + i + windowSize, std::begin(complexSignal), [](const iq16_t & item) {
//                        return std::complex<float>((float)item.I, (float)item.Q);
//                    });

//                    stdComplexFFT(std::begin(complexSignal), std::begin(complexFFTRes), std::log2(windowSize));

//                    // Half replacements ===================================================
//                    std::vector<std::complex<float>> tmp{std::begin(complexFFTRes), \
//                                std::begin(complexFFTRes) + complexFFTRes.size() / 2};
//                    std::copy(std::begin(complexFFTRes) + complexFFTRes.size() / 2, \
//                              std::end(complexFFTRes), std::begin(complexFFTRes));
//                    std::copy(std::begin(tmp), std::end(tmp), \
//                              std::begin(complexFFTRes) + tmp.size());
//                    // =====================================================================

//                    for (size_t l = 0; l < windowSize; l++) {
//                        waterfallMap->data()->setCell(l, 0, std::abs(complexFFTRes.at(l)));
//                    }

//                    waterfallMap->setColorScale(colorScale); // associate the color map with the color scale
//                    waterfallMap->setGradient(QCPColorGradient::gpSpectrum);
//                    waterfallMap->rescaleDataRange();
//                }
//            });
//        }

//        std::for_each(std::begin(threadPool), std::end(threadPool), [](std::thread & item) {
//            item.join();
//        });
//    }

//    return;

    size_t key = 0;
    for (size_t i = 0; i < verticalSize; i += windowSize / scale) {
        this->colorMaps.push_back(new QCPColorMap(this->ui->plotter->xAxis, this->ui->plotter->yAxis));
        QCPColorMap * waterfallMap = this->colorMaps.back();
        if (waterfallMap == nullptr) {
            this->ui->statusbar->showMessage("No more color maps...");
            break;
        }
        waterfallMap->data()->setSize(horizontalSize, 1);
        waterfallMap->data()->setRange(QCPRange(0, horizontalSize), QCPRange(key, key + 1));
        key++;

        if (i < (verticalSize - windowSize)) {
            std::transform(std::begin(signal) + i, std::begin(signal) + i + windowSize, std::begin(complexSignal), [](const iq16_t & item) {
                return std::complex<float>((float)item.I, (float)item.Q);
            });
        } else {
            break;
        }

        stdComplexFFT(std::begin(complexSignal), std::begin(complexFFTRes), std::log2(windowSize));

        // Half replacements ===================================================
        std::vector<std::complex<float>> tmp{std::begin(complexFFTRes), \
                    std::begin(complexFFTRes) + complexFFTRes.size() / 2};
        std::copy(std::begin(complexFFTRes) + complexFFTRes.size() / 2, \
                  std::end(complexFFTRes), std::begin(complexFFTRes));
        std::copy(std::begin(tmp), std::end(tmp), \
                  std::begin(complexFFTRes) + tmp.size());
        // =====================================================================

        for (size_t l = 0; l < windowSize; l++) {
            waterfallMap->data()->setCell(l, 0, std::abs(complexFFTRes.at(l)));
        }

        waterfallMap->setColorScale(colorScale); // associate the color map with the color scale
        waterfallMap->setGradient(QCPColorGradient::gpSpectrum);
        waterfallMap->rescaleDataRange();
    }

    this->ui->plotter->rescaleAxes();
    this->ui->plotter->replot();
}

void WaterfallViewer::plotterMousePressSlot(QMouseEvent *event)
{


    if (selectionMode) {

        double x = this->ui->plotter->xAxis->pixelToCoord(event->pos().x());
        double y = this->ui->plotter->yAxis->pixelToCoord(event->pos().y());

        if (this->clickCounter == 0) {
            this->ui->plotter->clearGraphs();

            this->fPoint = std::make_pair(x, y);

            this->clickCounter = 1;
        } else if (this->clickCounter == 1) {
            this->sPoint = std::make_pair(x, y);

            double duration = std::abs(fPoint.second - sPoint.second) * ts * 1e6;
            double width = std::abs(fPoint.first - sPoint.first) * fftResolution / 1e6;

            QString msg = QString("Duration: ");
            msg += QString::number( duration ) + " us;";
            emit printConsole(msg);

            msg.clear();
            msg = QString("Width: ");
            msg += QString::number( width ) + " MHz;";
            emit printConsole(msg);

            msg.clear();
            msg = QString("Speed: ");
            msg += QString::number( width / duration ) + " MHz/us;";
            emit printConsole(msg);

            this->clickCounter = 0;
        }

        QCPGraph * graph = this->ui->plotter->addGraph();

        graph->setData(QVector<double>{x}, QVector<double>{y});
        graph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlus, 100));
        graph->setLineStyle((QCPGraph::LineStyle::lsNone));
        graph->setPen(QPen(Qt::black));

        this->ui->plotter->replot();
    }
}


void WaterfallViewer::on_actionOpen_triggered()
{
    this->console->show();
}


void WaterfallViewer::on_actionSelection_triggered(bool checked)
{
    this->selectionMode = checked;
}

