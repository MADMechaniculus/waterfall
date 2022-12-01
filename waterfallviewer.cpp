#include "waterfallviewer.h"
#include "./ui_waterfallviewer.h"

WaterfallViewer::WaterfallViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WaterfallViewer)
{
    ui->setupUi(this);

    QCustomPlot * customPlot = this->ui->plotter;

    // configure axis rect:
    customPlot->setInteractions(QCP::iRangeDrag|QCP::iRangeZoom); // this will also allow rescaling the color scale by dragging/zooming
    customPlot->axisRect()->setupFullAxesBox(true);
    customPlot->xAxis->setLabel("Time");
    customPlot->yAxis->setLabel("Frequency");
}

WaterfallViewer::~WaterfallViewer()
{
    delete ui;
}


void WaterfallViewer::on_actionOpen_file_triggered()
{
    const uint32_t windowSize = std::pow(2, 13);

#ifdef WIN32
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open record"), "C:\\", \
                                                    tr("Record files (*.bin *.dat *.pcm *.iq16)"));
#elif __unix__
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open record"), "/home/jana", tr("Image Files (*.png *.jpg *.bmp)"));
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

    uint64_t verticalSize = windowSize;     // 32k fft
    uint64_t horizontalSize = signal.size();

    std::cout << "V: " << verticalSize << ", H: " << horizontalSize << std::endl << std::flush;

    QCPColorScale *colorScale = new QCPColorScale(this->ui->plotter);
    this->ui->plotter->plotLayout()->addElement(0, 1, colorScale); // add it to the right of the main axis rect
    colorScale->setType(QCPAxis::atRight); // scale shall be vertical bar with tick/axis labels right (actually atRight is already the default)
    colorScale->axis()->setLabel("Signal amplitude");

    size_t key = 0;
    for (size_t i = 0; i < horizontalSize; i += windowSize) {
        QCPColorMap * waterfallMap = new QCPColorMap(this->ui->plotter->xAxis, this->ui->plotter->yAxis);
        if (waterfallMap == nullptr) {
            this->ui->statusbar->showMessage("No more color maps...");
            break;
        }
        waterfallMap->data()->setSize(1, verticalSize);
        waterfallMap->data()->setRange(QCPRange(key, key + 1), QCPRange(0, verticalSize));
        key++;

        if (i < (horizontalSize - windowSize)) {
            std::transform(std::begin(signal) + i, std::begin(signal) + i + windowSize, std::begin(complexSignal), [](const iq16_t & item) {
                return std::complex<float>((float)item.I, (float)item.Q);
            });
        } else {
            break;
        }

        stdComplexFFT(std::begin(complexSignal), std::begin(complexFFTRes), std::log2(windowSize));

        std::transform(std::begin(complexFFTRes), std::end(complexFFTRes), std::begin(windowData), [](std::complex<float> & item) {
            return 20.0 * std::log(std::abs(item));
        });

        for (size_t l = 0; l < windowSize; l++) {
            waterfallMap->data()->setCell(0, l, windowData.at(l));
        }

        waterfallMap->setColorScale(colorScale); // associate the color map with the color scale
        waterfallMap->setGradient(QCPColorGradient::gpSpectrum);
        waterfallMap->rescaleDataRange();

        this->ui->plotter->rescaleAxes();
    }
}

