#include "mtstatus.h"
#include "ui_mtstatus.h"

#include <QProgressBar>

MTStatus::MTStatus(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::MTStatus)
{
    ui->setupUi(this);
    ui->verticalLayout->addWidget(new QProgressBar());
    ui->verticalLayout->addWidget(new QProgressBar());
    ui->verticalLayout->addWidget(new QProgressBar());
}

MTStatus::~MTStatus()
{
    delete ui;
}
