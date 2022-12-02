#ifndef CONSOLEFORM_H
#define CONSOLEFORM_H

#include <QDialog>

namespace Ui {
class ConsoleForm;
}

class ConsoleForm : public QDialog
{
    Q_OBJECT

public:
    explicit ConsoleForm(QWidget *parent = nullptr);
    ~ConsoleForm();

public slots:
    void appendConsole(QString message);

private:
    Ui::ConsoleForm *ui;
};

#endif // CONSOLEFORM_H
