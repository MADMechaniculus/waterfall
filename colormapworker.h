#ifndef COLORMAPWORKER_H
#define COLORMAPWORKER_H

#include <QObject>

#include <qcustomplot.h>

#include <thread>
#include <vector>
#include <complex>
#include <functional>
#include <algorithm>
#include <atomic>

#include "dsp.hpp"

class ColorMapWorker : public QObject
{
    Q_OBJECT

    std::thread executorThread;
    std::atomic_bool stopped{true};
    std::atomic_bool running{false};

    std::vector<QCPColorMap *> * pool;
    std::vector<std::complex<float>> * signal;

    std::vector<std::complex<float>> complexFFTRes;

    size_t start{0};
    size_t stop{0};

    uint64_t verticalSize{0};
    uint64_t horizontalSize{0};

    uint32_t windowSize{0};
    double scaleFactor{0.0};

    struct checkList {
        bool diap{false};
        bool mapPool{false};
        bool signalData{false};
        bool windowSize{false};
        bool scaleFactor{false};
        bool dimensions{false};

        bool validate(void) {
            return diap && mapPool && signalData && \
                    windowSize && scaleFactor && dimensions;
        }
    };
    struct checkList check;

public:
    ColorMapWorker(QObject * parent = nullptr) : QObject(parent) {}

    void setDimensions(uint64_t vs, uint64_t hs) {
        this->verticalSize = vs;
        this->horizontalSize = hs;
        this->check.dimensions = true;
    }

    void setDiap(size_t startIndex, size_t stopIndex) {
        this->start = startIndex;
        this->stop = stopIndex;
        this->check.diap = true;
    }

    void setSignalData(std::vector<std::complex<float>> * ptr) {
        this->signal = ptr;
        this->check.signalData = true;
    }

    void setWindowSize(uint32_t windowSize) {
        this->windowSize = windowSize;
        complexFFTRes.resize(windowSize);
        this->check.windowSize = true;
    }

    void setScaleFactor(double scaleFactor) {
        this->scaleFactor = scaleFactor;
        this->check.scaleFactor = true;
    }

    void setMapPool(std::vector<QCPColorMap *> * ptr) {
        this->pool = ptr;
        this->check.mapPool = true;
    }

public slots:
    bool startProcessing(void) {
        if (this->check.validate()) {
            if (this->running.load() == false) {
                this->stopped.store(false);
                this->executorThread = std::thread(std::bind(&ColorMapWorker::process, this));
                return true;
            }
        }
        return false;
    }

    bool abortProcessing(void) {
        if (this->running.load() == true) {
            this->stopped.store(true);
            this->executorThread.join();
            return true;
        }
        return false;
    }
signals:
    /**
     * @brief Сигнал завершения процесса вычисления по заданному диапазону
     * @param success Результат выполнеия процесса
     */
    void Complete(bool success);

    /**
     * @brief Сигнал для оповещения родителя о выполнении итерации
     */
    void Progress(void);

protected:
    void process(void) {

        this->running.store(true);

        size_t currentIndex = start;

        while (this->stopped.load() != true) {

            if (currentIndex < stop) {
                // Processing

                if (!(currentIndex + windowSize <= (this->signal->size() - 1))) {
                    break;
                }

                QCPColorMap * waterfallMap = this->pool->at(currentIndex);
                uint32_t offset = this->windowSize * this->scaleFactor;

                stdComplexFFT(signal->begin() + currentIndex + offset, \
                              std::begin(complexFFTRes), std::log2(windowSize));

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

            }

            currentIndex++;

            emit this->Progress();
        }

        this->running.store(true);

    }
};

#endif // COLORMAPWORKER_H
