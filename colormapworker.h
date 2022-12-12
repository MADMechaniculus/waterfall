#ifndef COLORMAPWORKER_H
#define COLORMAPWORKER_H

#include <QObject>
#include <QColor>

#include <qcustomplot.h>

#include <thread>
#include <vector>
#include <complex>
#include <functional>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <iostream>

#include "dsp.hpp"

class ColorMapWorkerTask {
protected:
    std::vector<std::complex<float>> * signal;
    QCPColorMap * targetMap;

    size_t mapIndex;
    size_t windowSize;
    size_t step;

    std::atomic_bool inWork{false};
    std::mutex taskMutex;

public:
    typedef std::vector<std::complex<float>> cplxSignal_t;

    ColorMapWorkerTask() {}
    ColorMapWorkerTask(cplxSignal_t * pSignal, \
                       QCPColorMap * targetMap, \
                       size_t index, size_t wSize, \
                       size_t step) : \
        signal(pSignal), targetMap(targetMap), \
        mapIndex(index), windowSize(wSize), step(step) {}

    bool takeWork(void) {
        std::lock_guard<std::mutex> lock(this->taskMutex);
        if (this->inWork.load() != true) {
            this->inWork.store(true);
            return true;
        }
        return false;
    }

    friend class ColorMapWorker;
};

class ColorMapWorker : public QObject
{
    Q_OBJECT

    std::vector<ColorMapWorkerTask *> * tasks;
    float maxValue{0};

    std::thread executorThread;
    std::atomic_bool stopped{true};
    std::atomic_bool running{false};

    std::vector<std::complex<float>> complexFFTRes;

public:
    ColorMapWorker(QObject * parent = nullptr) : QObject(parent) {
        complexFFTRes.reserve(std::pow(2, 16));
    }

    void connectTasks(std::vector<ColorMapWorkerTask *> * tskPool) {
        this->tasks = tskPool;
    }

    float getMaxValue(void) {
        return maxValue;
    }

public slots:
    bool startProcessing(void) {
        if (this->running.load() == false) {
            maxValue = 0;
            this->stopped.store(false);
            try {
                this->executorThread = std::thread(std::bind(&ColorMapWorker::process, this));
            } catch (const std::exception &ex) {
                std::cerr << ex.what() << std::endl << std::flush;
            }
            return true;
        }
        return false;
    }

    void abortProcessing(void) {
        this->stopped.store(true);
        if (this->executorThread.joinable())
            this->executorThread.join();
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

        size_t workIndex = 0;

        while (this->stopped.load() != true) {

            if (workIndex == this->tasks->size()) {
                break;
            }

            if (this->tasks->at(workIndex)->takeWork()) {

                QCPColorMap * waterfallMap = this->tasks->at(workIndex)->targetMap;

                if (complexFFTRes.size() != this->tasks->at(workIndex)->windowSize) {
                    complexFFTRes.resize(this->tasks->at(workIndex)->windowSize);
                }

                stdComplexFFT(this->tasks->at(workIndex)->signal->begin() + \
                              this->tasks->at(workIndex)->mapIndex * this->tasks->at(workIndex)->step, \
                              std::begin(complexFFTRes), std::log2(this->tasks->at(workIndex)->windowSize));

                // Half replacements ===========================================
                std::vector<std::complex<float>> tmp{std::begin(complexFFTRes), \
                            std::begin(complexFFTRes) + complexFFTRes.size() / 2};
                std::copy(std::begin(complexFFTRes) + complexFFTRes.size() / 2, \
                          std::end(complexFFTRes), std::begin(complexFFTRes));
                std::copy(std::begin(tmp), std::end(tmp), \
                          std::begin(complexFFTRes) + tmp.size());
                // =============================================================

                std::for_each(std::begin(this->complexFFTRes), std::end(this->complexFFTRes), [this](const std::complex<float> & item) {
                    float tmp = std::abs(item);
                    if (this->maxValue < std::abs(item))
                        this->maxValue = std::abs(item);
                });

                for (size_t l = 0; l < this->tasks->at(workIndex)->windowSize; l++) {
                    waterfallMap->data()->setCell(l, this->tasks->at(workIndex)->mapIndex, std::abs(this->complexFFTRes.at(l)));
                }

                emit this->Progress();
            }
            workIndex++;
        }

        this->running.store(false);

    }
};

#endif // COLORMAPWORKER_H
