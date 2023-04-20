#pragma once

#include <QProcess>
#include <QThread>
#include <QFile>
#include <QTimer>
#include "model.h"

#define DEFAULT_TOOTH_COL 0.5   // Default tooth color. 0.5 means middle gray.

class BinaryHandler : public Model
{
Q_OBJECT

public:
    BinaryHandler();
    ~BinaryHandler();
    int init_model(const QString&, const int, ToothLife&, const int, const int,
                   const int, const int);
    int start_model();
    void stop_model();
    Mesh& fill_mesh(Tooth&);


private:
    std::vector<std::string> getDataFilenames_(int, bool);
    int addTooth_(const int);
    int setTempEnv_(const QString&);
    int setBinSettings_(const QString&, const int, const int);
    int calcProgress_(int, std::vector<long>&, int);

    QProcess m_process;             // model binary process
    QTimer m_killTimer;             // timer for killing the binary after a user-defined limit
    QFile m_progressFile;           // model progress tracking file
    QString m_binary;               // model binary name
    QString m_cmd;                  // command line string to execute
    bool m_killedByUser;

    int m_timeLimit;                // time in ms after which the binary is killed if still running
    int m_id;                       // simulation run ID
    ToothLife* m_toothLife;         // simulation history


private slots:
    void binaryFinished_();
    void binaryError_(QProcess::ProcessError);


protected:
    void run();
};
