#include "mtstatus.h"
#include "ui_mtstatus.h"

MTStatus::MTStatus(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MTStatus)
{
    ui->setupUi(this);
}

MTStatus::~MTStatus()
{
    delete ui;
}
