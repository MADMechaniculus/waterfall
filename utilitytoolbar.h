#ifndef UTILITYTOOLBAR_H
#define UTILITYTOOLBAR_H

#include <QObject>
#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QToolBar>

#ifdef WIN32
#include <windows.h>
#include <tchar.h>
#else
#include <sys/sysinfo.h>
#endif

class UtilityToolBar : public QObject
{
    Q_OBJECT

    QToolBar * parentToolBar;

    QProgressBar * processingBar;
    QProgressBar * memoryUsage;

    QTimer * memoryUsageTimer;
    QTimer * processingTimer;

    uint32_t totalOps;
    std::atomic<uint32_t> currentOpsDone;

    uint16_t mode = 0;

#ifdef WIN32
    uint64_t getTotalMemory(void) {
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof (statex);
        GlobalMemoryStatusEx (&statex);
        return statex.ullTotalPhys;
    }

    uint64_t getFreeMemory(void) {
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof (statex);
        GlobalMemoryStatusEx (&statex);
        return statex.ullAvailPhys;
    }

    uint32_t getMemoryLoad(void) {
        MEMORYSTATUSEX statex;
        statex.dwLength = sizeof (statex);
        GlobalMemoryStatusEx (&statex);
        return statex.dwMemoryLoad;
    }
#else
    uint64_t getTotalMemory(void) {
        struct sysinfo info;
        sysinfo(&info);

        return info.totalram;
    }

    uint64_t getFreeMemory(void) {
        struct sysinfo info;
        sysinfo(&info);

        return info.freeram;
    }

    uint32_t getMemoryLoad(void) {
        struct sysinfo info;
        sysinfo(&info);

        uint64_t value = (info.totalram - info.freeram) * info.mem_unit;
        uint32_t ret = (double)value / (double)info.totalram * 100.0;

        return ret;
    }
#endif

public:
    enum UtilityToolBar_Progress_Mode_ : uint16_t {
        UtilityToolBar_Progress_Mode_DataProcessing = 0,
        UtilityToolBar_Progress_Mode_ColorMapCreating
    };

    explicit UtilityToolBar(QToolBar * parentToolBar, QObject *parent = nullptr) : \
        parentToolBar(parentToolBar), QObject(parent) {

        mode = UtilityToolBar_Progress_Mode_DataProcessing;

        this->processingBar = new QProgressBar();
        this->processingBar->setRange(0, 100);
        this->memoryUsage = new QProgressBar();
        this->memoryUsage->setRange(0, 100);

        this->parentToolBar->addWidget(new QLabel("Processing:"));
        this->parentToolBar->addWidget(this->processingBar);
        this->parentToolBar->addWidget(new QLabel("Memory usage:"));
        this->parentToolBar->addWidget(this->memoryUsage);

        this->memoryUsageTimer = new QTimer(this);
        this->processingTimer = new QTimer(this);

        connect(this->memoryUsageTimer, &QTimer::timeout, this, &UtilityToolBar::updateMemoryUsage);
        connect(this->processingTimer, &QTimer::timeout, this, &UtilityToolBar::updateProcessingBar);

        this->memoryUsageTimer->setInterval(500);
        this->memoryUsageTimer->start();

        this->processingTimer->setInterval(100);
        this->processingTimer->start();
    }

signals:

    void completeProcessing(void);

public slots:
    void setTotalOperations(uint32_t opsCount) {
        totalOps = opsCount;
    }

    void resetProgress(void) {
        this->processingBar->setValue(0);
        totalOps = 0;
        currentOpsDone.store(0);
    }

    void increaseProgress(void) {
        currentOpsDone.fetch_add(1, std::memory_order_relaxed);
    }

    void setMode(uint16_t newMode) {
        if (newMode > UtilityToolBar_Progress_Mode_ColorMapCreating) {
            this->mode = UtilityToolBar_Progress_Mode_ColorMapCreating;
        }
        this->mode = newMode;
    }

protected slots:
    void updateMemoryUsage(void) {
        this->memoryUsage->setValue(this->getMemoryLoad());
    }

    void updateProcessingBar(void) {
        if (totalOps != 0) {
            this->processingBar->setValue((int)((double)currentOpsDone / (double)totalOps * 100.0));
            if ((this->currentOpsDone.load() == this->totalOps) && (this->mode == UtilityToolBar_Progress_Mode_DataProcessing)) {
                emit this->completeProcessing();
                this->resetProgress();
            }
        }
    }
};

#endif // UTILITYTOOLBAR_H
