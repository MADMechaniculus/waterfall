#include "waterfallviewer.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    WaterfallViewer w;
    w.show();
    return a.exec();
}
