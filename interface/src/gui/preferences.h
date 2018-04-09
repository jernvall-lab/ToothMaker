#ifndef PREFERENCES_H
#define PREFERENCES_H

#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#include <QDialog>
#include <QtGui>
#include "morphomaker.h"


class Preferences : public QDialog
{
    Q_OBJECT

    public:
        Preferences();

    private:
        int maxThreads;


    private slots:
        void readMaxThreads(int);

};


class System : public QWidget
{
    public:
        System(QWidget *parent=0);
};


#endif // PREFERENCES_H
