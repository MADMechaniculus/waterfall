#include "consoleform.h"
#include "ui_consoleform.h"

ConsoleForm::ConsoleForm(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConsoleForm)
{
    ui->setupUi(this);
}

ConsoleForm::~ConsoleForm()
{
    delete ui;
}

void ConsoleForm::appendConsole(QString message)
{
    this->ui->plainTextEdit->appendPlainText("> " + message);
}
