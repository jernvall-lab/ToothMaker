#pragma once

#include <QWidget>
#include <QDialog>
#include <QPainter>
#include <QTableWidget>
#include <QCheckBox>
#include <QLabel>
#include <QSpinBox>
#include <QPushButton>
#include <QDir>
#include <QFileDialog>
#include <QSignalMapper>
#include "misc/scanlist.h"


// These are experimentally determined values, subject to font size etc.(?)
// TODO: Find a way to set them automatically.
#define HEADER_HEIGHT 26
#define ROW_HEIGHT 30
#define ROW_WIDTH 344

#define MAX_TABLE_HEIGHT 326

#define STATUS_BAR_X    90
#define STATUS_BAR_Y    384
#define NJOBS_X 508
#define NJOBS_Y 213


class ScanWindow : public QDialog
{
    Q_OBJECT

    public:
        ScanWindow(QWidget *parent=0);
        void setParameters(Parameters *);
        void resetScanList();
        ScanList *getScanList();
        QString getResultsFolder();
        void updateScanStatus(QString);
        int calcPermutations();
        int storeModelSteps();
        int storeOrientations();

    private slots:
        void selectStorageFolder();
        void handleStartButton();

        void scanCombinations();
        void exportModelData();
        void exportIntSteps();
        void cellValueChanged(int, int);

    signals:
        void startScan();
        void stopScan();

    private:
        QTableWidget *table;
        std::vector<std::string> *parNames;
        std::vector<double> *parValues;
        ScanList *scanList;
        int combScanning, exportData;
        QString resultsFolder;
        QString scanStatus;
        QLabel *resLabel;
        std::string printTextMsg, njobs_msg;
        bool tableSet;
        QCheckBox *combCheckbox, *exportCheckbox, *stepsCheckbox, *orientCheckbox;
        QPushButton *startButton;

        ScanItem *createScanItem(int);
        void writeStatusBar(std::string);
        void printNofJobs(int);
        void addParameterRow(QString);
        void paintEvent(QPaintEvent*);
        QPushButton *createButton(int, int, const QString &, const char *);
};
