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
#include <mutex>

#include "dsp.hpp"

class ColorMapWorkerTask {
protected:
    std::vector<std::complex<float>> * signal;
    QCPColorMap * targetMap;
    size_t mapIndex;
    size_t windowSize;
    size_t step;

    std::atomic_bool inWork{false};
    std::atomic_bool done{false};
    std::mutex taskMutex;
public:
    typedef std::vector<std::complex<float>> cplxSignal_t;

    ColorMapWorkerTask() {}
    ColorMapWorkerTask(cplxSignal_t * pSignal, \
                       QCPColorMap * pColorMap, \
                       size_t index, size_t wSize, \
                       size_t step) : \
        signal(pSignal), targetMap(pColorMap), \
        mapIndex(index), windowSize(wSize), step(step) {}

    bool takeWork(void) {
        std::lock_guard<std::mutex> lock(this->taskMutex);
        if (this->inWork.load() != true) {
            this->inWork.store(true);
            return true;
        }
        return false;
    }

    bool isWorkTaken(void) {
        return this->inWork.load();
    }

    bool isWorkDone(void) {
        return this->done.load();
    }

    friend class ColorMapWorker;
};

class ColorMapWorker : public QObject
{
    Q_OBJECT

    std::vector<ColorMapWorkerTask *> * tasks;

    std::thread executorThread;
    std::atomic_bool stopped{true};
    std::atomic_bool running{false};

    std::vector<std::complex<float>> complexFFTRes;

public:
    ColorMapWorker(QObject * parent = nullptr) : QObject(parent) {}

public slots:
    bool startProcessing(void) {
        if (this->running.load() == false) {
            this->stopped.store(false);
            this->executorThread = std::thread(std::bind(&ColorMapWorker::process, this));
            return true;
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

        while (this->stopped.load() != true) {

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


            emit this->Progress();
        }

        this->running.store(true);

    }
};

#endif // COLORMAPWORKER_H
