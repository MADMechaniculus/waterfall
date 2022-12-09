#include "customtoolbar.h"

CustomToolBar::CustomToolBar(QObject *parent) : QObject(parent) {
    this->sampleRate = new QLineEdit();
    this->sampleRate->setText("1100e6");
    this->fftOrder = new QLineEdit();
    this->fftOrder->setText("10");
    this->scaleFactor = new QLineEdit();
    this->scaleFactor->setText("8");

    connect(sampleRate, &QLineEdit::textChanged, this, &CustomToolBar::onSampleRate_TextChanged);
    connect(fftOrder, &QLineEdit::textChanged, this, &CustomToolBar::onFFTOrder_TextChanged);
    connect(scaleFactor, &QLineEdit::textChanged, this, &CustomToolBar::onScaleFactor_TextChanged);
}

CustomToolBar::~CustomToolBar() {}

void CustomToolBar::draw(QToolBar *rootBar) {
    rootBar->addWidget(new QLabel("Sample rate"));
    rootBar->addWidget(this->sampleRate);
    rootBar->addSeparator();
    rootBar->addWidget(new QLabel("FFT order"));
    rootBar->addWidget(this->fftOrder);
    rootBar->addSeparator();
    rootBar->addWidget(new QLabel("Scale factor"));
    rootBar->addWidget(this->scaleFactor);
}

void CustomToolBar::emitAll() {
    emit this->sampleRate->textChanged(sampleRate->text());
    emit this->fftOrder->textChanged(fftOrder->text());
    emit this->scaleFactor->textChanged(scaleFactor->text());
}
