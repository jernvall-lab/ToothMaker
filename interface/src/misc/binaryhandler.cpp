/**
 * @class BinaryHandler
 * @brief Handler for binary/script models.
 *
 * Manages simulation binary execution, output file reading, and result parsing.
 * Uses an OutputReader helper for file I/O and a state machine (RunState) to
 * track execution progress.
 *
 * Threading:
 * - init_model(), start_model(), stop_model() run in the main thread.
 * - run() runs in a worker thread (QThread).
 * - binaryFinished_() and binaryError_() are slots called in the main thread.
 *
 * Note: init_model() changes the process-wide working directory to the run folder
 * because Read_Humppa_DAD_file() and Read_OFF_file() use CWD-relative paths.
 */

#include <iostream>
#include <ctime>

#include <QDir>
#include <QCoreApplication>
#include <QTextStream>
#include <QDebug>

#include "misc/binaryhandler.h"
#include "utils/writeparameters.h"
#include "readdata.h"
#include "morphomaker.h"



// ============================================================================
//  OutputReader
// ============================================================================

void OutputReader::configure(const QString& runPath, int runId, int stepSize,
                             int renderMode, const QString& outputStyle,
                             const std::vector<QString>& parsers)
{
    m_runPath = runPath;
    m_runId = runId;
    m_stepSize = stepSize;
    m_renderMode = renderMode;
    m_outputStyle = outputStyle;
    m_parsers = parsers;
}



QString OutputReader::getExtension_() const
{
    if (m_outputStyle == "PLY" || m_outputStyle == "")
        return ".ply";
    if (m_outputStyle == "Matrix")
        return ".txt";
    if (m_outputStyle == "Humppa")
        return ".off";
    return "";
}



/**
 * @brief Find model output files matching the expected pattern for a step.
 * @param step  Step number (1-based; iteration = step * stepSize).
 * @return List of matching files.
 */
QFileInfoList OutputReader::findOutputFiles_(int step) const
{
    QString ext = getExtension_();
    if (ext.isEmpty())
        return {};

    int iter = step * m_stepSize;
    QString pattern = QString::number(iter) + "*"
                      + QString::number(m_runId) + "*" + ext;
    QDir qdir(m_runPath);
    return qdir.entryInfoList(QStringList(pattern), QDir::Files);
}



bool OutputReader::fileExistsForStep(int step) const
{
    return !findOutputFiles_(step).isEmpty();
}



/**
 * @brief Find the last step that has output files.
 * @param totalSteps    Total expected steps (nIter / stepSize).
 * @return Step number of last existing output, or -1 if none found.
 */
int OutputReader::findLastExistingStep(int totalSteps) const
{
    for (int s = totalSteps; s >= 1; s--) {
        if (fileExistsForStep(s))
            return s;
    }
    return -1;
}



/**
 * @brief Apply output parsers to a model output file.
 *
 * Runs each parser in sequence. Parsers that write to the specified output
 * file have their output substituted in. For parsers like dad_to_polygons
 * that produce their own output filename (ITER_RUNID_.off -> ITER_RUNID.off),
 * the transformed filename is detected and returned.
 *
 * @param originalFile  Original output filename (relative to run path).
 * @param step          Step number.
 * @return Final filename after parsers, or empty string on failure.
 */
QString OutputReader::applyParsers_(const QString& originalFile, int step)
{
    if (m_parsers.empty())
        return originalFile;

    QString currentFile = originalFile;

    for (auto& parser : m_parsers) {
        QString parserOut = "parser_tmp_" + QString::number(m_runId) + ".txt";
        QString parserPath = QDir::toNativeSeparators(
            QDir(m_runPath + "/../bin").filePath(parser));
        QString cmd = "\"" + parserPath + "\" " + currentFile + " " + parserOut;

        QProcess process;
        process.setWorkingDirectory(m_runPath);
        auto args = QProcess::splitCommand(cmd);
        process.start(args.takeFirst(), args);

        if (!process.waitForFinished(PARSER_TIMEOUT)) {
            qWarning() << "Parser" << parser << "timed out on" << currentFile;
            continue;
        }
        if (process.exitCode() != 0) {
            qWarning() << "Parser" << parser << "returned error code"
                       << process.exitCode() << "on" << currentFile;
        }

        // If parser wrote to our specified output file, swap it in.
        // Note: dad_to_polygons ignores the output argument and writes its own
        // file (removes trailing underscore), so parserOut won't exist for it.
        QString parserOutPath = m_runPath + "/" + parserOut;
        if (QFile::exists(parserOutPath)) {
            QFile::remove(m_runPath + "/" + currentFile);
            QFile::copy(parserOutPath, m_runPath + "/" + currentFile);
            QFile::remove(parserOutPath);
        }
    }

    // Check for dad_to_polygons-style output: "ITER_RUNID_.off" -> "ITER_RUNID.off".
    int iter = step * m_stepSize;
    QString ext = getExtension_();
    QString transformedFile = QString::number(iter) + "_"
                              + QString::number(m_runId) + ext;
    if (QFile::exists(m_runPath + "/" + transformedFile))
        return transformedFile;

    // Fall back to original if no transformation happened.
    if (QFile::exists(m_runPath + "/" + currentFile))
        return currentFile;

    qWarning() << "No valid output file after parsers for step" << step;
    return "";
}



/**
 * @brief Read output file(s) for a step and create a Tooth object.
 * @param step  Step number (1-based; iteration = step * stepSize).
 * @return Result with success flag, Tooth pointer (caller owns), and error message.
 */
OutputReader::Result OutputReader::readStep(int step)
{
    Result result = { false, nullptr, "" };

    QFileInfoList files = findOutputFiles_(step);
    if (files.isEmpty()) {
        result.errorMsg = QString("No output file for step %1 (iter %2)")
                          .arg(step).arg(step * m_stepSize);
        return result;
    }

    QString fname = files.at(0).fileName();

    // Apply parser pipeline if configured.
    if (!m_parsers.empty()) {
        fname = applyParsers_(fname, step);
        if (fname.isEmpty()) {
            result.errorMsg = "Parser pipeline failed at step "
                              + QString::number(step);
            return result;
        }
    }

    // Create tooth and read data.
    Tooth* tooth = new Tooth(m_renderMode);

    if (m_outputStyle == "PLY" || m_outputStyle == "") {
        if (morphomaker::Read_PLY_file(fname.toStdString(), *tooth)) {
            delete tooth;
            result.errorMsg = "Failed to read PLY file: " + fname;
            return result;
        }
    }
    else if (m_outputStyle == "Matrix") {
        if (morphomaker::Read_BIN_matrix(fname.toStdString(), *tooth)) {
            delete tooth;
            result.errorMsg = "Failed to read matrix file: " + fname;
            return result;
        }
    }
    else if (m_outputStyle == "Humppa") {
        if (morphomaker::Read_OFF_file(fname.toStdString(), *tooth)) {
            delete tooth;
            result.errorMsg = "Failed to read OFF file: " + fname;
            return result;
        }
        if (morphomaker::Read_Humppa_DAD_file(step, m_stepSize, m_runId, *tooth)) {
            delete tooth;
            result.errorMsg = "Failed to read DAD file at step "
                              + QString::number(step);
            return result;
        }
    }
    else {
        delete tooth;
        result.errorMsg = "Unknown output style: " + m_outputStyle;
        return result;
    }

    result.success = true;
    result.tooth = tooth;
    return result;
}



// ============================================================================
//  BinaryHandler
// ============================================================================

BinaryHandler::BinaryHandler() : Model()
{
    connect(&m_process, SIGNAL(finished(int)), this, SLOT(binaryFinished_()));
    connect(&m_process, SIGNAL(errorOccurred(QProcess::ProcessError)), this,
            SLOT(binaryError_(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(started()), this, SLOT(start()));

    // Kill timer: connected once here, started/stopped per run.
    connect(&m_killTimer, &QTimer::timeout, this, [this]() {
        if (m_process.state() == QProcess::Running) {
            m_error = RunError::ProcessTimeout;
            stop_model();
        }
    });

    m_timeLimit = -1;
    m_id = 0;
    m_toothLife = nullptr;
    m_killedByUser = false;
    m_state = RunState::Idle;
    m_error = RunError::None;
}



BinaryHandler::~BinaryHandler()
{
}



/**
 * @brief Initialize binary model.
 *
 * Sets up the temporary environment, exports parameters, configures the
 * command line, and prepares the OutputReader.
 *
 * Changes the process-wide working directory to the run folder. This is
 * required because Read_Humppa_DAD_file() and Read_OFF_file() resolve
 * filenames relative to CWD.
 */
int BinaryHandler::init_model(const QString& temp_path, const int max_cores,
                              ToothLife& tlife, const int num_iter,
                              const int step_size, const int id,
                              const int timeLimit)
{
    (void)max_cores;
    m_binary = QString(modelBin.c_str());
    m_id = id;
    m_timeLimit = timeLimit;
    m_toothLife = &tlife;
    systemTempPath = temp_path;
    m_state = RunState::Idle;
    m_error = RunError::None;

    setTempEnv_(temp_path);

    // Create run folder.
    QString run_folder = QString::number(m_id);
    m_runPath = temp_path + "/" + run_folder;
    QDir().mkpath(m_runPath);

    // Set CWD to run folder (see class doc for why).
    QDir::setCurrent(m_runPath);

    // Export parameters to the run folder.
    QString parfile = m_runPath + "/mpar_" + QString::number(m_id) + ".txt";
    int rv = morphomaker::Export_parameters(parameters, parfile.toStdString(),
                                            inputStyle);
    if (rv) {
        return -1;
    }
    stepSize = step_size;
    nIter = num_iter;

    // Build command line (uses short par filename; binary runs in CWD).
    QString shortParfile = "mpar_" + QString::number(m_id) + ".txt";
    setBinSettings_(shortParfile, num_iter, step_size);

    // Configure the output reader with a single, consistent run ID.
    m_reader.configure(m_runPath, m_id, step_size, renderMode,
                       outputStyle, outputParsers);

    return 0;
}



/**
 * @brief Start the model binary.
 * @return Start time (unix timestamp), or -1 on error.
 */
int BinaryHandler::start_model()
{
    if (m_process.state() != QProcess::NotRunning) {
        return -1;
    }

    retval = 0;
    m_state = RunState::Running;
    m_error = RunError::None;
    m_killedByUser = false;

    m_process.setProcessChannelMode(QProcess::ForwardedChannels);
    m_process.setWorkingDirectory(m_runPath);

    qDebug().nospace() << "Executing " << m_cmd;
    auto args = QProcess::splitCommand(m_cmd);
    m_process.start(args.takeFirst(), args);

    // Start kill timer only if a positive time limit is set.
    if (m_timeLimit > 0) {
        m_killTimer.setInterval(m_timeLimit);
        m_killTimer.start();
    }

    return time(nullptr);
}



/**
 * @brief Kill the running model.
 */
void BinaryHandler::stop_model()
{
    if (m_process.state() == QProcess::NotRunning) {
        return;
    }

    m_killedByUser = true;
    m_state = RunState::Cancelled;

    qDebug().nospace() << "Asking " << m_process.program() << " to exit.";
    int timeout = 100;
    m_process.terminate();
    if (m_process.waitForFinished(timeout)) {
        qDebug() << m_process.program() << "exited gracefully.";
        return;
    }

    qDebug().nospace() << m_process.program() << " still running after "
                       << timeout << "ms.";
    m_process.kill();
    m_process.waitForFinished(100);
    qDebug().nospace() << "Killing " << m_process.program() << ".";
}



/**
 * @brief Returns mesh with view-mode-appropriate vertex colors (Humppa models).
 */
Mesh& BinaryHandler::fill_mesh(Tooth& tooth)
{
    Mesh& mesh = tooth.get_mesh();
    if (outputStyle != "Humppa") {
        return mesh;
    }

    // View mode: 0 = shape only, 1 = differentiation & knots, 2+ = morphogen.
    int view_mode = atof(parameters->getKey(PARKEY_VIEWMODE).c_str());
    double view_thresh = atof(parameters->getKey(PARKEY_VIEWTHRESH).c_str());
    auto& colors = mesh.get_vertex_colors(1);   // Original vertex colors.
    auto& cell_data = tooth.get_cell_data();

    for (uint32_t i = 0; i < colors.size(); i++) {
        mesh::vertex_color color = { DEFAULT_TOOTH_COL, DEFAULT_TOOTH_COL,
                                     DEFAULT_TOOTH_COL, 1.0 };

        if (view_mode == 1) {
            // White for differentiated cells.
            if (colors.at(i).a > 0.0 && colors.at(i).a < 0.6) {
                color = { 1.0, 1.0, 1.0, 1.0 };
            }
            // Keep knots colored as in the .off file (yellow).
            if (colors.at(i).a >= 0.6) {
                color = { colors.at(i).r, colors.at(i).g,
                          colors.at(i).b, colors.at(i).a };
            }
        }

        if (cell_data.size() > i && view_mode > 1) {
            // Red for morphogen levels above view_thresh.
            auto data = cell_data.at(i);
            uint16_t j = view_mode - 2;
            if (data.size() > j && data.at(j) > view_thresh) {
                color = { 1.0, 0.0, 0.0, 1.0 };
            }
        }

        mesh.set_vertex_color(i, color);
    }

    return mesh;
}



/**
 * @brief Copy model binaries to the temporary folder.
 */
int BinaryHandler::setTempEnv_(const QString& temp_path)
{
    QString temp_bin_path = temp_path + "/bin";
    QDir().mkpath(temp_bin_path);

    // Model binaries reside under ../Resources/bin/ relative to the app.
    QDir resources(QCoreApplication::applicationDirPath());
    resources.cd(RESOURCES);
    resources.cd("bin");

    if (DEBUG_MODE) {
        fprintf(stderr, "Model resources directory: %s\n",
                resources.path().toStdString().c_str());
        fprintf(stderr, "Application directory: %s\n",
                QCoreApplication::applicationDirPath().toStdString().c_str());
    }

    QStringList files = resources.entryList(QDir::Files);
    for (auto& f : files) {
        auto dest = temp_bin_path + "/" + f;
        if (QFile::exists(dest)) {
            QFile::remove(dest);
        }
        if (!QFile::copy(resources.path() + "/" + f, dest)) {
            qWarning() << "Failed to copy" << f << "to" << temp_bin_path;
        }
    }

    return 0;
}



/**
 * @brief Construct binary command line.
 */
int BinaryHandler::setBinSettings_(const QString& parfile, const int num_iter,
                                   const int step_size)
{
    m_cmd = "";
    QTextStream str;
    str.setString(&m_cmd);

    // Check if we're dealing with a Python script.
    QStringList blist = m_binary.split(".");
    if (blist.size() > 1 && blist.at(1) == "py") {
        str << "python ";
    }

    QString binaryPath = QDir::toNativeSeparators(
        QDir("../bin").filePath(m_binary));
    str << "\"" << binaryPath << "\" ";

    if (inputStyle == "MorphoMaker" || inputStyle == "") {
        str << "--param " << parfile << " --id " << m_id << " --step "
            << step_size << " --niter " << num_iter;
    }
    else if (inputStyle == "Humppa") {
        str << parfile << " " << m_id << " " << step_size << " "
            << num_iter / step_size;
    }
    else {
        fprintf(stderr, "Invalid argument style: %s\n",
                inputStyle.toStdString().c_str());
        return -1;
    }

    return 0;
}



/**
 * @brief Slot for QProcess 'finished' signal.
 * Waits for the worker thread to complete, then emits finished().
 */
void BinaryHandler::binaryFinished_()
{
    m_killTimer.stop();
    wait();
    emit finished();
}



/**
 * @brief Slot for QProcess 'errorOccurred' signal.
 */
void BinaryHandler::binaryError_(QProcess::ProcessError err)
{
    if (m_killedByUser)
        return;

    QString msg = "Fatal error:";
    if (err == QProcess::FailedToStart) {
        msg = QString("Fatal error: Failed to start binary '%1'.").arg(m_binary);
        m_error = RunError::FailedToStart;
    }
    else if (err == QProcess::Crashed) {
        msg = QString("Fatal error: Binary '%1' crashed.").arg(m_binary);
        m_error = RunError::ProcessCrashed;
    }
    else if (err == QProcess::Timedout)
        msg += " Binary wait timeout.";
    else if (err == QProcess::WriteError)
        msg += " Cannot write process.";
    else if (err == QProcess::ReadError)
        msg += " Cannot read process.";
    else if (err == QProcess::UnknownError)
        msg += " Unknown error.";

    retval = 1;
    m_state = RunState::Failed;

    qDebug() << msg;
    emit msgStatusBar(msg.toStdString());

    // If the binary failed to start at all, binaryFinished_() was never called.
    if (err == QProcess::FailedToStart)
        binaryFinished_();
}



// ============================================================================
//  Worker thread
// ============================================================================

/**
 * @brief Read remaining output files after the process has exited.
 *
 * On parse failures, logs a warning and continues to the next step.
 * Partial results are better than none.
 *
 * @param skipLast      Skip the last existing step (for crash recovery).
 * @param step          Step to resume from.
 * @param totalSteps    Total expected steps (nIter / stepSize).
 */
void BinaryHandler::drainRemaining_(bool skipLast, int step, int totalSteps)
{
    int maxStep = skipLast
        ? m_reader.findLastExistingStep(totalSteps) - 1
        : totalSteps;

    if (maxStep < step)
        return;

    const int timeoutMs = 3000;

    while (step <= maxStep) {
        // Brief wait for filesystem latency.
        int waited = 0;
        while (!m_reader.fileExistsForStep(step) && waited < timeoutMs) {
            msleep(UPDATE_INTERVAL);
            waited += UPDATE_INTERVAL;
        }

        if (!m_reader.fileExistsForStep(step)) {
            qWarning() << "Timed out waiting for output at step" << step
                       << "(iter" << step * stepSize << ")";
            step++;
            continue;   // Skip missing step, don't abort.
        }

        // Small delay to ensure file is fully flushed. In Phase 2 there is no
        // look-ahead (no step N+1 to confirm N is complete). The process has
        // exited so writes should be done, but filesystem metadata updates
        // (size, timestamps) can lag behind on some systems.
        msleep(UPDATE_INTERVAL);

        auto result = m_reader.readStep(step);
        if (!result.success) {
            qWarning() << "Failed to read step" << step
                       << ":" << result.errorMsg;
            // Continue - partial results better than none.
        } else {
            m_toothLife->addTooth(result.tooth);
        }

        step++;
        currentIter = (step - 1) * stepSize;
    }
}



/**
 * @brief Main worker thread loop.
 *
 * Phase 1: While the model binary is running, read output files using a
 *          look-ahead pattern (read step N only when step N+1 exists,
 *          ensuring step N is fully written).
 *
 * Phase 2: After the binary exits, drain remaining output files. On crash,
 *          skip the last file (may be incomplete). Parse failures are logged
 *          but don't abort - partial results are preserved.
 */
void BinaryHandler::run()
{
    const int totalSteps = nIter / stepSize;
    const int MAX_READ_RETRIES = 3;
    const int READ_RETRY_DELAY = 100;   // ms between retries
    int readRetries = 0;
    int step = 1;   // First output at iteration = stepSize.

    // Add an empty tooth for iteration 0. Models don't output at iteration 0,
    // but the UI development slider starts from 0 and expects an entry there.
    Tooth* emptyTooth = new Tooth(renderMode);
    m_toothLife->addTooth(emptyTooth);

    // Phase 1: Read files while model is running.
    while (m_process.state() == QProcess::Running
           && m_state == RunState::Running) {
        msleep(UPDATE_INTERVAL);

        // Look-ahead: only read step N when step N+1 exists.
        if (m_reader.fileExistsForStep(step + 1)) {
            auto result = m_reader.readStep(step);
            if (result.success) {
                m_toothLife->addTooth(result.tooth);
                step++;
                readRetries = 0;
            } else {
                readRetries++;
                if (readRetries >= MAX_READ_RETRIES) {
                    qWarning() << "Skipping step" << step << "after"
                               << MAX_READ_RETRIES << "read failures:"
                               << result.errorMsg;
                    step++;
                    readRetries = 0;
                } else {
                    msleep(READ_RETRY_DELAY);
                }
            }
        }

        currentIter = (step - 1) * stepSize;
    }

    // User cancelled?
    if (m_state == RunState::Cancelled) {
        return;
    }

    // Phase 2: Drain remaining files.
    bool crashed = (m_process.exitStatus() == QProcess::CrashExit);
    if (crashed) {
        m_state = RunState::Failed;
        m_error = RunError::ProcessCrashed;
    } else {
        m_state = RunState::Draining;
    }

    drainRemaining_(crashed, step, totalSteps);

    if (m_state == RunState::Draining) {
        m_state = RunState::Completed;
    }

    // Warn about non-zero exit code (distinct from crash).
    if (m_process.exitCode() != 0 && !crashed) {
        emit msgStatusBar("Warning: Model exited with error code "
                          + std::to_string(m_process.exitCode()) + ".");
    }
}
