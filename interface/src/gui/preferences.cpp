/**
 * @class Preferences
 * @brief Preferences window.
 *
 * ** Current disabled. **
 */

#include "preferences.h"



Preferences::Preferences()
{    
    maxThreads = DEFAULT_CORES;

    QPushButton *closeButton = new QPushButton(tr("Close"));

    connect(closeButton, &QPushButton::clicked, this, &Preferences::close);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    buttonsLayout->addStretch(1);
    buttonsLayout->addWidget(closeButton);

    QLabel *threadsLabel = new QLabel("Max. CPU cores:", this);
    QSpinBox *threads = new QSpinBox(this);
    // qOverload: valueChanged has int+QString overloads in Qt5, int only in Qt6.
    connect(threads, qOverload<int>(&QSpinBox::valueChanged), this, &Preferences::readMaxThreads);
    threads->setSingleStep(1);
    threads->setValue(maxThreads);
    threads->setMinimum(1);
    threads->setMaximum(3);
    threads->setMaximumWidth(50);

    QGroupBox *system = new QGroupBox(tr("System"));
    QHBoxLayout *systemLayout = new QHBoxLayout;
    systemLayout->addWidget(threadsLabel);
    systemLayout->addWidget(threads);
    system->setLayout(systemLayout);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(system);
   // mainLayout->addStretch(1);
    mainLayout->addSpacing(12);
    mainLayout->addLayout(buttonsLayout);
    setLayout(mainLayout);

    setWindowTitle(tr("Preferences"));
}



void Preferences::readMaxThreads(int val)
{
    maxThreads = val;
}



System::System(QWidget *parent) : QWidget(parent)
{
    QLabel *threadsLabel = new QLabel("Max. CPU cores:", this);
    QSpinBox *threads = new QSpinBox(this);
    // NOTE: Old SIGNAL/SLOT connect to readMaxThreads() was a silent no-op
    // (System doesn't have that slot). This class appears unused.
    threads->setSingleStep(1);
    threads->setMinimum(1);
    threads->setMaximum(3);
    threads->setMaximumWidth(50);

    QGroupBox *system = new QGroupBox(tr("System"));
    QHBoxLayout *systemLayout = new QHBoxLayout;
    systemLayout->addWidget(threadsLabel);
    systemLayout->addWidget(threads);
    system->setLayout(systemLayout);

    QVBoxLayout  *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(system);
    setLayout(mainLayout);
}
