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
    uint32_t currentOpsDone;

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

#endif

public:
    explicit UtilityToolBar(QToolBar * parentToolBar, QObject *parent = nullptr) : \
        parentToolBar(parentToolBar), QObject(parent) {

        this->processingBar = new QProgressBar();
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
    }

signals:

public slots:
    void setTotalOperations(uint32_t opsCount) {
        totalOps = opsCount;
    }

    void resetProgress(void) {
        this->processingBar->reset();
        this->processingTimer->stop();
    }

    void increaseProgress(void) {
        currentOpsDone++;
    }

protected slots:
    void updateMemoryUsage(void) {
        this->memoryUsage->setValue(this->getMemoryLoad());
    }

    void updateProcessingBar(void) {
        this->processingBar->setValue((int)((double)currentOpsDone / (double)totalOps * 100.0));
    }
};

#endif // UTILITYTOOLBAR_H
