#ifndef MTSTATUS_H
#define MTSTATUS_H

#include <QWidget>

namespace Ui {
class MTStatus;
}

class MTStatus : public QWidget
{
    Q_OBJECT

public:
    explicit MTStatus(QWidget *parent = nullptr);
    ~MTStatus();

private:
    Ui::MTStatus *ui;
};

#endif // MTSTATUS_H
