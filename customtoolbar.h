#ifndef CUSTOMTOOLBAR_H
#define CUSTOMTOOLBAR_H

#include <QObject>
#include <QToolBar>
#include <QLabel>
#include <QLineEdit>

class CustomToolBar : public QObject {
    Q_OBJECT

    QLineEdit * sampleRate;
    QLineEdit * fftOrder;
    QLineEdit * scaleFactor;

public:
    CustomToolBar(QObject * parent);
    ~CustomToolBar();

    void draw(QToolBar * rootBar);

    void emitAll(void);

signals:
    void onSampleRate_TextChanged(const QString & text);
    void onFFTOrder_TextChanged(const QString & text);
    void onScaleFactor_TextChanged(const QString & text);

protected:

};

#endif // CUSTOMTOOLBAR_H
