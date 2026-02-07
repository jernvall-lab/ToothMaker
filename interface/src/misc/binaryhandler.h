#pragma once

#include <atomic>
#include <QDir>
#include <QProcess>
#include <QThread>
#include <QTimer>
#include <vector>
#include <string>
#include "model.h"

#define DEFAULT_TOOTH_COL 0.5   // Default tooth color. 0.5 means middle gray.


enum class RunState {
    Idle,           // Not running, ready to start
    Running,        // Process running, actively reading output
    Draining,       // Process exited, reading remaining output
    Completed,      // All output read successfully
    Failed,         // Error occurred (see RunError)
    Cancelled       // User requested stop
};

enum class RunError {
    None,
    FailedToStart,      // Process couldn't start
    ProcessCrashed,     // Process crashed during run
    ProcessTimeout,     // Killed by time limit
    OutputTimeout,      // File didn't appear in time
    ParseError          // Couldn't parse output file
};



/**
 * @class OutputReader
 * @brief Reads model output files and creates Tooth objects.
 *
 * Handles file existence checks, output parser pipeline, and file parsing.
 * Uses a single run ID throughout, eliminating the ID fragility of the old design.
 *
 * Note: File reading functions (Read_OFF_file, Read_Humppa_DAD_file) use
 * CWD-relative paths. The caller must ensure CWD is set to the run folder.
 */
class OutputReader
{
public:
    struct Result {
        bool success;
        Tooth* tooth;       // Caller takes ownership. nullptr on failure.
        QString errorMsg;
    };

    void configure(const QString& runPath, int runId, int stepSize,
                   int renderMode, const QString& outputStyle,
                   const std::vector<QString>& parsers);

    bool fileExistsForStep(int step) const;
    int findLastExistingStep(int totalSteps) const;
    Result readStep(int step);

private:
    QFileInfoList findOutputFiles_(int step) const;
    QString getExtension_() const;
    QString applyParsers_(const QString& originalFile, int step);

    QString m_runPath;
    int m_runId;
    int m_stepSize;
    int m_renderMode;
    QString m_outputStyle;
    std::vector<QString> m_parsers;
};



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

    RunState getRunState() const    { return m_state; }
    RunError getRunError() const    { return m_error; }


private:
    int setTempEnv_(const QString& temp_path);
    int setBinSettings_(const QString& parfile, const int num_iter,
                        const int step_size);
    void drainRemaining_(bool skipLast, int step, int totalSteps);

    QProcess m_process;
    QTimer m_killTimer;
    QString m_binary;               // Model binary name.
    QString m_cmd;                  // Command line string to execute.
    QString m_runPath;              // Run folder: systemTempPath/m_id/
    bool m_killedByUser;

    int m_timeLimit;                // Kill timer limit in ms (-1 = disabled).
    int m_id;                       // Simulation run ID.
    ToothLife* m_toothLife;         // Simulation results.

    std::atomic<RunState> m_state;
    std::atomic<RunError> m_error;
    OutputReader m_reader;


private slots:
    void binaryFinished_();
    void binaryError_(QProcess::ProcessError);


protected:
    void run();
};
