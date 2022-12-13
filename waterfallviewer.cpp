#include "waterfallviewer.h"
#include "./ui_waterfallviewer.h"

WaterfallViewer::WaterfallViewer(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::WaterfallViewer)
{
    ui->setupUi(this);
    
    QFile file(":/dark-green/stylesheet.qss");
    file.open(QFile::ReadOnly | QFile::Text);
    QTextStream stream(&file);
    this->ui->plotter->setStyleSheet(stream.readAll());

    qRegisterMetaType<QCPColorGradient>("QCPColorGradient");
    qRegisterMetaType<QCPRange>("QCPRange");
    
    // Connect plotter to mousePress slot
    connect(this->ui->plotter, &QCustomPlot::mousePress, this, &WaterfallViewer::plotterMousePressSlot);
    
    QCustomPlot * customPlot = this->ui->plotter;
    // configure axis rect:
    customPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    customPlot->axisRect()->setupFullAxesBox(true);
    customPlot->xAxis->setLabel("Frequency");
    customPlot->yAxis->setLabel("Time");
    customPlot->yAxis->setRangeReversed(true);
    
    // Plotter color scale installation
    colorScale = new QCPColorScale(this->ui->plotter);
    this->ui->plotter->plotLayout()->addElement(0, 1, colorScale);
    colorScale->setType(QCPAxis::atRight);
    colorScale->axis()->setLabel("Signal amplitude");
    
    this->ui->plotter->addLayer("Dots");
    
    this->toolBar = new CustomToolBar(this);
    this->toolBar->draw(this->ui->topToolBar);
    
    connect(toolBar, &CustomToolBar::onSampleRate_TextChanged, this, &WaterfallViewer::sampleRateChanged);
    connect(toolBar, &CustomToolBar::onFFTOrder_TextChanged, this, &WaterfallViewer::fftOrderChanged);
    connect(toolBar, &CustomToolBar::onScaleFactor_TextChanged, this, &WaterfallViewer::scaleFactorChanged);
    
    // Обновление параметров анализа (fs, fft_order, scale)
    this->toolBar->emitAll();

    this->utilBar = new UtilityToolBar(this->ui->bottomToolBar, this);
    this->utilBar->resetProgress();

    connect(this->utilBar, &UtilityToolBar::completeProcessing, this, &WaterfallViewer::onProcessingComplete);

    // Create workers ==========================================================
    this->availThreads = std::thread::hardware_concurrency() / 2;
    this->workers.resize(availThreads);
    for (size_t i = 0; i < this->availThreads; i++) {
        this->workers[i] = new ColorMapWorker(this);
        this->workers[i]->connectTasks(&this->tasks);
        connect(this->workers[i], &ColorMapWorker::Progress, this->utilBar, &UtilityToolBar::increaseProgress);
    }
    // =========================================================================

    // Initial state for colorscheme settings ==================================
    this->ui->actionSpectrum->trigger();
    // =========================================================================
}

WaterfallViewer::~WaterfallViewer()
{
    delete ui;
}


void WaterfallViewer::on_actionOpen_file_triggered()
{
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
        this->ui->statusbar->showMessage("Empty filename");
        return;
    }
    
    this->selectedFile = fileName;

    this->filesVector.emplace_back(fileName);
    this->updateFileList();

    this->startProcessing();
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
            this->appendConsole(msg);
            
            msg.clear();
            msg = QString("Width: ");
            msg += QString::number( width ) + " MHz;";
            this->appendConsole(msg);
            
            msg.clear();
            msg = QString("Speed: ");
            msg += QString::number( width / duration ) + " MHz/us;";
            this->appendConsole(msg);
            
            this->clickCounter = 0;
        }
        
        this->dotGraph->setData(this->dotGraphKeys, this->dotGraphVals);
        this->ui->plotter->replot();
    }
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
    double scaleFactor = text.toDouble(&ret);
    if (ret) {
        this->scale = scaleFactor;
        this->ui->statusbar->showMessage("New scale factor applied");
    } else {
        this->scale = 0.1;
        this->ui->statusbar->showMessage("Wrong scale factor format [default " + QString::number(scale) + "]");
    }
}

void WaterfallViewer::onProcessingComplete()
{
    std::vector<float> maximums(this->workers.size());
    for (size_t i = 0; i < this->workers.size(); i++) {
        maximums[i] = this->workers[i]->getMaxValue();
    }

    colorMap->setDataRange(QCPRange(0, *std::max_element(std::begin(maximums), \
                                                         std::end(maximums))));

    this->ui->plotter->rescaleAxes();
    this->ui->plotter->replot();

    for (ColorMapWorkerTask * item : this->tasks) {
        if (item != nullptr)
            delete item;
    }
    tasks.clear();

    for (ColorMapWorker * item : this->workers) {
        if (item != nullptr) {
            item->abortProcessing();
        }
    }
}

void WaterfallViewer::cleanPlotter() {
    this->ui->plotter->clearPlottables();
    this->ui->plotter->clearGraphs();

    this->dotGraph = this->ui->plotter->addGraph();
    this->dotGraph->setLayer("Dots");
    this->dotGraph->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCross, 50));
    this->dotGraph->setLineStyle((QCPGraph::LineStyle::lsNone));

    QPen dotPen = QPen(Qt::gray);
    dotPen.setWidth(2);
    this->dotGraph->setPen(dotPen);
}

void WaterfallViewer::startProcessing()
{
    const uint32_t windowSize = std::pow(2, this->fftOrder);
    fftResolution = Fs / 2.0 / (double)windowSize;

    QFileInfo fileInfo(this->selectedFile);

    std::ifstream readFile(this->selectedFile.toStdString(), std::ios::binary);
    if (!readFile.is_open()) {
        this->ui->statusbar->showMessage("Error on opening read stream");
        return;
    }

    iq16_t sample;
    std::complex<float> tmp;
    bool pushing = false;
    if ((this->complexSignal.size() != (fileInfo.size() / sizeof (iq16_t))) && !complexSignal.empty()) {
        this->complexSignal.resize(fileInfo.size() / sizeof (iq16_t));
    } else {
        pushing = true;
    }
    for (size_t i = 0; i < fileInfo.size() / sizeof (iq16_t); i++) {
        readFile.read((char*)&sample, sizeof (iq16_t));
        tmp.real((float)sample.I);
        tmp.imag((float)sample.Q);
        if (pushing)
            this->complexSignal.push_back(tmp);
        else
            this->complexSignal[i] = tmp;
    }
    readFile.close();

    uint64_t verticalSize = this->complexSignal.size();

    ts = (double)2 / Fs * (double)windowSize * scale;

    size_t maps = ( verticalSize - (verticalSize % (uint64_t)(windowSize * scale))) / (scale * windowSize);

    tasks.resize(maps);

    this->cleanPlotter();

    this->colorMap = new QCPColorMap(this->ui->plotter->xAxis, \
                                     this->ui->plotter->yAxis);

    this->colorMap->data()->setSize(windowSize, maps);
    this->colorMap->data()->setRange(QCPRange(0, windowSize), QCPRange(0, maps));
    this->colorMap->setColorScale(this->colorScale);
    this->colorMap->setInterpolate(true);

    this->updateColorScheme();

    for (size_t i = 0; i < maps; i++) {
        tasks[i] = new ColorMapWorkerTask(&this->complexSignal, \
                                          this->colorMap, \
                                          i, windowSize, windowSize * scale);
    }

    this->utilBar->resetProgress();
    this->utilBar->setMode(UtilityToolBar::UtilityToolBar_Progress_Mode_DataProcessing);
    this->utilBar->setTotalOperations(maps);

    this->ui->plotter->rescaleAxes();

    for (ColorMapWorker * item : this->workers) {
        item->startProcessing();
    }
}

void WaterfallViewer::updateColorScheme()
{
    if (this->colorMap != nullptr) {
        if (this->ui->actionGrayscale->isChecked())
            this->colorMap->setGradient(QCPColorGradient::gpGrayscale);
        if (this->ui->actionSpectrum->isChecked())
            this->colorMap->setGradient(QCPColorGradient::gpSpectrum);
    }
}

void WaterfallViewer::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift) {
        this->selectionMode = true;
    }
}

void WaterfallViewer::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift) {
        this->selectionMode = false;
    }
}

void WaterfallViewer::updateFileList() {
    this->ui->fileList->clear();
    for (size_t i = 0; i < this->filesVector.size(); i++) {
        this->ui->fileList->addItem(this->filesVector[i].getFilePath());
    }
}

void WaterfallViewer::on_clearFileListButton_clicked()
{
    this->filesVector.clear();
    this->updateFileList();
    this->selectedFile = "";
}

void WaterfallViewer::on_reprocessButton_clicked()
{
    if (!this->selectedFile.isEmpty())
        this->startProcessing();
    else
        this->ui->statusbar->showMessage("No target file selected");
}

void WaterfallViewer::on_fileList_currentRowChanged(int currentRow)
{
    if (currentRow == -1) {
        return;
    }
    this->selectedFile = this->filesVector.at(currentRow).getFilePath();
}

void WaterfallViewer::appendConsole(QString message)
{
    this->ui->consolePanel->appendPlainText(message);
}

void WaterfallViewer::on_actionGrayscale_triggered()
{
    if (this->ui->actionGrayscale->isChecked() == false) {
        this->ui->actionGrayscale->setChecked(true);
    }
    this->ui->actionSpectrum->setChecked(false);
    this->updateColorScheme();
}

void WaterfallViewer::on_actionSpectrum_triggered()
{
    if (this->ui->actionSpectrum->isChecked() == false) {
        this->ui->actionSpectrum->setChecked(true);
    }
    this->ui->actionGrayscale->setChecked(false);
    this->updateColorScheme();
}

