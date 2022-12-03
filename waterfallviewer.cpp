#include "waterfallviewer.h"
#include "./ui_waterfallviewer.h"

WaterfallViewer::WaterfallViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WaterfallViewer)
{
    ui->setupUi(this);
    
    qRegisterMetaType<QCPColorGradient>("QCPColorGradient");
    qRegisterMetaType<QCPRange>("QCPRange");
    
    this->console = new ConsoleForm(this);
    connect(this, &WaterfallViewer::printConsole, console, &ConsoleForm::appendConsole);
    
    // Connect plotter to mousePress slot
    connect(this->ui->plotter, &QCustomPlot::mousePress, this, &WaterfallViewer::plotterMousePressSlot);
    
    QCustomPlot * customPlot = this->ui->plotter;
    // configure axis rect:
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    customPlot->axisRect()->setupFullAxesBox(true);
    customPlot->xAxis->setLabel("Frequency");
    customPlot->yAxis->setLabel("Time");
    
    // Plotter color scale installation
    colorScale = new QCPColorScale(this->ui->plotter);
    this->ui->plotter->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorScale->axis()->setLabel("Signal amplitude");
    
    this->ui->plotter->addLayer("Dots");
    
    this->toolBar = new CustomToolBar(this);
    this->toolBar->draw(this->ui->toolBar);
    
    connect(toolBar, &CustomToolBar::onSampleRate_TextChanged, this, &WaterfallViewer::sampleRateChanged);
    connect(toolBar, &CustomToolBar::onFFTOrder_TextChanged, this, &WaterfallViewer::fftOrderChanged);
    connect(toolBar, &CustomToolBar::onScaleFactor_TextChanged, this, &WaterfallViewer::scaleFactorChanged);
    
    // Обновление параметров анализа (fs, fft_order, scale)
    this->toolBar->emitAll();
}

WaterfallViewer::~WaterfallViewer()
{
    delete ui;
}


void WaterfallViewer::on_actionOpen_file_triggered()
{
    const uint32_t windowSize = std::pow(2, this->fftOrder);
    fftResolution = Fs / 2.0 / (double)windowSize;
    const uint32_t scale = this->scale;
    
#ifdef WIN32
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open record"), "C:\\", \
                                                    tr("Record files (*.bin *.dat *.pcm *.iq16)"));
#elif __unix__
    QString fileName = QFileDialog::getOpenFileName(this,
                                                    tr("Open record"), "/home", \
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
    
    this->cleanPlotter();
    
    readFile.read((char*)signal.data(), fileInfo.size());
    readFile.close();
    
    uint64_t verticalSize = signal.size() / 4;
    uint64_t horizontalSize = windowSize;
    
    ts = (double)2 / Fs * (double)windowSize / (double)scale;
    
    size_t availThreads = std::thread::hardware_concurrency() / 2;
    std::vector<std::thread> threadPool(availThreads);
    
    // Create color maps pool ==================================================
    uint32_t step = windowSize / scale;
    size_t maps = (verticalSize - (verticalSize % scale)) / scale;
    for (size_t i = 0; i < maps; i++) {
        this->colorMaps.push_back(new QCPColorMap(this->ui->plotter->xAxis, \
                                                  this->ui->plotter->yAxis));
    }
    // =========================================================================
    
    // Calculating thread payloads =============================================
    std::vector<std::pair<size_t, size_t>> threadRanges(threadPool.size());
    size_t perThread = maps.size() / threadPool.size();
    size_t prevIndex = 0;
    for (size_t t = 0; t < threadPool.size(); t++) {
        if (t != threadPool.size() - 1) {
            threadRanges.at(t) = std::make_pair(prevIndex, prevIndex + perThread);
        } else {
            threadRanges.at(t) = std::make_pair(prevIndex, maps.size() - 1);
        }
        prevIndex += perThread;
    }
    // =========================================================================
    
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
        waterfallMap->setGradient(QCPColorGradient::gpGrayscale);
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
            this->dotGraphKeys.clear();
            this->dotGraphVals.clear();
            
            this->fPoint = std::make_pair(x, y);
            
            this->dotGraphKeys.push_back(x);
            this->dotGraphVals.push_back(y);
            
            this->clickCounter = 1;
        } else if (this->clickCounter == 1) {
            this->sPoint = std::make_pair(x, y);
            
            this->dotGraphKeys.push_back(x);
            this->dotGraphVals.push_back(y);
            
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
        
        this->dotGraph->setData(this->dotGraphKeys, this->dotGraphVals);
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

void WaterfallViewer::sampleRateChanged(const QString &text)
{
    bool ret = false;
    double sampleRate = text.toDouble(&ret);
    if (ret) {
        this->Fs = sampleRate;
        this->ui->statusbar->showMessage("New sample rate applied");
    } else {
        this->Fs = 192e6;
        this->ui->statusbar->showMessage("Wrong sample rate format [default " + QString::number(Fs) + "]");
    }
}

void WaterfallViewer::fftOrderChanged(const QString &text)
{
    bool ret = false;
    double fftOrder = text.toUInt(&ret);
    if (ret) {
        this->fftOrder = fftOrder;
        this->ui->statusbar->showMessage("New FFT order applied");
    } else {
        this->fftOrder = 10;
        this->ui->statusbar->showMessage("Wrong FFT order format [default " + QString::number(fftOrder) + "]");
    }
}

void WaterfallViewer::scaleFactorChanged(const QString &text)
{
    bool ret = false;
    double scaleFactor = text.toUInt(&ret);
    if (ret) {
        this->scale = scaleFactor;
        this->ui->statusbar->showMessage("New scale factor applied");
    } else {
        this->scale = 8;
        this->ui->statusbar->showMessage("Wrong scale factor format [default " + QString::number(scale) + "]");
    }
}

void WaterfallViewer::cleanPlotter() {
    this->ui->plotter->clearPlottables();
    this->ui->plotter->clearGraphs();
    
    this->dotGraph = this->ui->plotter->addGraph();
    this->dotGraph->setLayer("Dots");
    this->dotGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssPlusCircle, 100));
    this->dotGraph->setLineStyle((QCPGraph::LineStyle::lsNone));
    
    QPen dotPen = QPen(Qt::black);
    dotPen.setWidth(5);
    this->dotGraph->setPen(dotPen);
    
    if (!this->colorMaps.empty()) {
        this->colorMaps.clear();
    }
}

