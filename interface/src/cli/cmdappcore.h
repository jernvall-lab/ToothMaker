#ifndef CMDAPPCORE_H
#define CMDAPPCORE_H

#include <QCoreApplication>
#include <cli/glengine.h>
#include <readdata.h>
#include <misc/scanlist.h>
#include <parameters.h>
#include <tooth.h>
#include <toothlife.h>
#include <model.h>
#include <morphomaker.h>


class CmdAppCore : public QCoreApplication
{
    Q_OBJECT

    public:
        CmdAppCore(int & argc, char ** argv);
        int startParameterScan(int, char *, char *, int, int, int);

    private slots:
        void writeStatusBar(std::string);
        void updateProgress();
        void updateModel();

    private:
        void runModel();
        void scanParameters();
        int setModel(char *);

        GLEngine *glengine;
        ScanList *scanList;
        Parameters *parameters;
        ToothLife *toothLife;

        std::vector<Model*> models;
        QTimer *progressTimer;

        QString runDir;
        std::string systemTempPath;
        int nIter;
        int expImg;
        int currentScanItem;
        int timeStart;
        int modelId;
        // A general purpose "file" index that starts from zero at the start of
        // the program, and increases when files are saved etc.
        int fileIndex;
};

#endif // CMDAPPCORE_H
