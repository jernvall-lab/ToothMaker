/**
 * @class BinaryHandler
 * @brief Handler for binary/script models.
 *
 * Takes care of starting and killing models and tracks their progress.
 *
 * Calling run_binary() executes the binary/script and start progress tracker.
 *
 */

#include <iostream>
#include <sstream>
#include <cmath>
#include <ctime>

#include <QDir>
#include <QCoreApplication>
#include <QTextStream>
#include <QTime>
#include <QDebug>

#include "misc/binaryhandler.h"
#include "utils/writeparameters.h"
#include "readdata.h"
#include "morphomaker.h"


BinaryHandler::BinaryHandler() : Model()
{
    connect(&m_process, SIGNAL(finished(int)), this, SLOT(binaryFinished_()));
    connect(&m_process, SIGNAL(errorOccurred(QProcess::ProcessError)), this,
            SLOT(binaryError_(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(started()), this, SLOT(start()));

    m_timeLimit = -1;   // by default allowing the binary to run forever (-1)
    m_id = 0;
    m_toothLife = nullptr;
}


BinaryHandler::~BinaryHandler()
{
}


/**
 * @brief Initialize binary model.
 * @param temp_path     System temporary folder.
 * @param max_cores     Maximum number of CPU cores (not used).
 * @param tlife         Toothlife object for results.
 * @param num_iter      Number of iterations.
 * @param step_size     Step size.
 * @param id            Unique run ID.
 * @return              0 if success, else -1.
 */
int BinaryHandler::init_model(const QString& temp_path, const int max_cores,
                              ToothLife& tlife, const int num_iter,
                              const int step_size, const int id, const int timeLimit)
{
    (void)max_cores;
    m_binary = QString(modelBin.c_str());
    m_id = id;
    m_timeLimit = timeLimit;
    m_toothLife = &tlife;
    systemTempPath = temp_path;

    setTempEnv_(temp_path);

    QDir qdir;
    qdir.setCurrent(temp_path);
    QString run_folder = QString::number(m_id);
    qdir.mkdir(run_folder);
    qdir.setCurrent(run_folder);

    QString parfile;
    QTextStream str;
    str.setString(&parfile);
    str << temp_path << "/" << run_folder << "/mpar_" << m_id << ".txt";
    int rv = morphomaker::Export_parameters( parameters, parfile.toStdString(),
                                             inputStyle );
    if (rv) {
        return -1;
    }
    stepSize = step_size;
    nIter = num_iter;

    // Can't send the parameter file with the full path to the binary,
    // as some programs have difficulties with long arguments.
    parfile = "";
    str << "mpar_" << m_id << ".txt";
    setBinSettings_( parfile, num_iter, step_size );

    return 0;
}



/**
 * @brief Call to start the model.
 * @param timeLimit     Maximum run time in ms after which the binary will be killed.
 * @return              Starting time or -1 if errors.
 */
int BinaryHandler::start_model()
{
    if (m_process.state() != QProcess::NotRunning) {
        return -1;
    }

    retval = 0;
    m_process.setProcessChannelMode( QProcess::ForwardedChannels );
    m_killedByUser = false;
    qDebug().nospace() << "Executing " << m_cmd;
    auto args = QProcess::splitCommand(m_cmd);
    m_process.start(args.takeFirst(), args);

    m_killTimer.setInterval(m_timeLimit);
    QObject::connect(&m_killTimer, &QTimer::timeout, [&]() {
        if (m_process.state() == QProcess::Running) {
            stop_model();
        }
    });
    m_killTimer.start();


    return time(NULL);
}



/**
 * @brief Call to kill the running model.
 */
void BinaryHandler::stop_model()
{
    if ( m_process.state() == QProcess::NotRunning ) {
        return;
    }

    m_killedByUser = true;

    qDebug().nospace() << "Asking " << m_process.program() << " to exit.";
    int timeout = 100;
    m_process.terminate();
    bool state = m_process.waitForFinished( timeout );
    if (state) {
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
 * @brief Given a tooth object returns mesh with colors according to the current
 * view mode and view threshold.
 * @param tooth     Tooth object.
 * @return          Mesh with updated colors.
 */
Mesh& BinaryHandler::fill_mesh( Tooth& tooth )
{
    Mesh& mesh = tooth.get_mesh();
    if (outputStyle != "Humppa") {
        return mesh;
    }

    // The following is specific to Humppa.
    // For view_mode=0 use the default tooth color, view_mode=1 uses the vertex
    // colors given in the output .off file, view_mode>1 use the morphogen
    // concentrations stored as cell data.

    int view_mode = atof( parameters->getKey(PARKEY_VIEWMODE).c_str() );
    double view_thresh = atof( parameters->getKey(PARKEY_VIEWTHRESH).c_str() );
    // Get the original vertex colors stored in alt_colors ('1' for argument).
    auto& colors = mesh.get_vertex_colors(1);
    auto& cell_data = tooth.get_cell_data();    // Morphogen concentrations.

    for ( uint32_t i=0; i<colors.size(); i++ ) {
        mesh::vertex_color color = { DEFAULT_TOOTH_COL, DEFAULT_TOOTH_COL,
                                     DEFAULT_TOOTH_COL, 1.0 };

        if ( view_mode == 0 ) {     // Mode: Shape only
        }

        if ( view_mode == 1 ) {     // Mode: Diff & knots.
            // White for differentiated cells.
            if ( colors.at(i).a > 0.0 && colors.at(i).a < 0.6 ) {
                color = { 1.0, 1.0, 1.0, 1.0 };
            }
            // Keep knots colored as in the .off file (yellow).
            if ( colors.at(i).a >= 0.6 ) {
                color = { colors.at(i).r, colors.at(i).g,
                          colors.at(i).b, colors.at(i).a };
            }
        }

        if ( cell_data.size() > i && view_mode > 1 ) {
            // Red for morphogen levels above view_thresh.
            auto data = cell_data.at(i);
            uint16_t j = view_mode-2;
            if ( data.size() > j && data.at(j) > view_thresh ) {
                color = { 1.0, 0.0, 0.0, 1.0 };
            }
        }

        mesh.set_vertex_color( i, color );
    }

    return mesh;
}



/**
 * @brief Apply output parsers, return the next expected model output file name(s).
 * @param step          Step number to search the files for.
 * @param test_only     If true, only tests if the expected output file exists.
 * @return              Vector containing the output file name(s).
 */
std::vector<std::string> BinaryHandler::getDataFilenames_( int step,
                                                           bool test_only )
{
    std::vector<std::string> output_files;
    QString run_id = QString::number( m_toothLife->getID() );
    QString run_path = systemTempPath + "/" + run_id + "/";
    QDir qdir(run_path);

    std::string ext = "";
    if (outputStyle == "PLY" || outputStyle == "")
        ext = ".ply";
    else if (outputStyle == "Matrix")
        ext = ".txt";
    else if (outputStyle == "Humppa")
        ext = ".off";
    else
        return output_files;

    //
    // TODO: Imnplement control of output file names.
    //

    // Note: allowing for some room in the input file name:
    int iter = step*stepSize;
    QString target = QString::number(iter) + "*" + run_id + "*"
                     + QString(ext.c_str());
    QStringList filter;
    filter << target;
    QFileInfoList files = qdir.entryInfoList( filter, QDir::Files );

    if (files.size() == 0)
        return output_files;

    if (test_only) {
        for (auto file : files) {
            output_files.push_back( file.fileName().toStdString() );
        }
        return output_files;
    }

/*
    std::cout << std::endl;
    std::cout << "** Running parsers in " << run_path.toStdString() << std::endl;
    std::cout << "** Parser target " << target.toStdString() << std::endl;
    std::cout << "** Number of files to be parsed: " << files.size() << std::endl;
*/

    // If no parsers are configured, use the original file directly
    if (outputParsers.empty()) {
        output_files.push_back(files.at(0).fileName().toStdString());
        return output_files;
    }

    // Apply parsers
    for (int i=0; i<files.size(); i++) {
        QString file = files.at(i).fileName();

        for (auto& parser : outputParsers) {
            QString parser_out = "parser_tmp_" + run_id + ".txt";
            QString parserPath = QDir::toNativeSeparators(QDir("../bin").filePath(parser));
            QString cmd = parserPath + " " + file + " " + parser_out;

            QProcess process;
            auto args = QProcess::splitCommand(cmd);
            process.start(args.takeFirst(), args);
            if(!process.waitForFinished( PARSER_TIMEOUT )) {
               qDebug() << "Error: Parser" << parser << "timed out after"
                        << PARSER_TIMEOUT << "ms on file" << file;
               continue;
            }
            if (process.exitCode() != 0) {
                qDebug() << "Warning: Parser" << parser << "returned error code"
                         << process.exitCode() << "on file" << file;
            }

            // Replace the input file with the parser output if applicable.
            // Note: dad_to_polygons writes its own output file (removes underscore
            // from filename), so parser_out won't exist for it.
            if (QFile::exists(parser_out)) {
                QFile::remove(file);
                QFile::copy(parser_out, file);
                QFile::remove(parser_out);
            }
        }
    }

    // Return the filename that matches what dad_to_polygons parser outputs.
    // The parser transforms "ITER_RUNID_.off" to "ITER_RUNID.off" (removes underscore).
    std::string outfile = std::to_string(iter) + "_" + run_id.toStdString() + ext;

    // Check if the expected output file exists
    QString expectedPath = run_path + QString::fromStdString(outfile);
    if (!QFile::exists(expectedPath)) {
        qDebug() << "Warning: Expected parser output" << expectedPath << "not found";
        // Fall back to returning the original file if parser output doesn't exist
        if (files.size() > 0) {
            outfile = files.at(0).fileName().toStdString();
            qDebug() << "Falling back to original file:" << QString::fromStdString(outfile);
        }
    }

    output_files.push_back( outfile );

    return output_files;
}



/**
 * @brief Adds an object to toothLife.
 *        Called from run() when a new step available.
 * @param stepTest
 * @return      0 if success (no error handling).
 */
int BinaryHandler::addTooth_(const int step_test)
{
    Tooth *tooth = new Tooth( renderMode );

    // Get the output file names, apply parsers:
    auto output_files = getDataFilenames_( step_test, false );
    if (output_files.size() == 0) {
        m_toothLife->addTooth(tooth);
        return 0;
    }
    std::string fname = output_files.at(0);     // Yes, this is on purpose...

    // Incomplete data files are not considered fatal errors, but the won't get
    // added to ToothLife. This may cause the object indices to be incorrectly
    // assigned if the model skips over result files.
    if (outputStyle == "PLY" || outputStyle == "") {
        if (morphomaker::Read_PLY_file( fname, *tooth )) {
            delete tooth;
            return -1;
        }
    }
    else if (outputStyle == "Matrix") {
        if (morphomaker::Read_BIN_matrix( fname, *tooth )) {
            delete tooth;
            return -1;
        }
    }
    else if (outputStyle == "Humppa") {
        if (morphomaker::Read_OFF_file( fname, *tooth )) {
            delete tooth;
            return -1;
        }
        if (morphomaker::Read_Humppa_DAD_file( step_test, stepSize, m_id, *tooth )) {
            delete tooth;
            return -1;
        }
    }
    else {}

    m_toothLife->addTooth(tooth);

    return 0;
}



/**
 * @brief Copies model binary to the temporary folder.
 * @param temp_path     System temporary folder.
 * @return              0 if success.
 */
int BinaryHandler::setTempEnv_(const QString& temp_path)
{
    QDir qdir;
    qdir.setCurrent(temp_path);

    // Set up a bin directory where to move the model binaries.
    if (!qdir.exists("bin")) {
        qdir.mkdir("bin");
    }
    QString temp_bin_path = temp_path + "/bin";
    qdir.setCurrent(temp_bin_path);

    // Assuming the model binaries reside under ../Resources/bin/ relative
    // to the app. dir.
    QDir resources(QCoreApplication::applicationDirPath());
    resources.cd(RESOURCES);
    resources.cd("bin");

    if (DEBUG_MODE) {
        fprintf(stderr, "Model resources directory: %s\n",
                resources.path().toStdString().c_str());
        fprintf(stderr, "Application directory: %s\n",
                QCoreApplication::applicationDirPath().toStdString().c_str());
        fprintf(stderr, "Current directory: '%s'\n",
                qdir.currentPath().toStdString().c_str());
    }

    QStringList files = resources.entryList(QDir::Files);
    for (auto& f : files) {
        auto dest = temp_bin_path+"/"+f;
        if (QFile::exists(dest)) {
            QFile::remove(dest);
        }
        QFile::copy(resources.path()+"/"+f, dest);
    }

    return 0;
}



/**
 * @brief Constructs binary command line.
 * @param parfile       Parameter file.
 * @param num_iter      Number of iterations.
 * @param step_size     Step size.
 * @return      0 if success, else -1.
 */
int BinaryHandler::setBinSettings_(const QString& parfile, const int num_iter,
                                     const int step_size)
{
    QString fname = "progress_" + QString::number(m_id) + ".txt";
    if (outputStyle == "Humppa") {
        fname = QString::number(m_id) + "______progressbar.txt";
    }
    m_progressFile.setFileName(fname);

    m_cmd = "";
    QTextStream str;
    str.setString(&m_cmd);

    // Check if we're dealing with a Python script.
    // It is users responsibility to make sure Python is available!
    QStringList blist = m_binary.split(".");
    if (blist.size() > 1 && blist.at(1) == "py") {
        str << "python ";
    }

    QString binaryPath = QDir::toNativeSeparators(QDir("../bin").filePath(m_binary));
    str << binaryPath << " ";
    if (inputStyle == "MorphoMaker" || inputStyle == "") {
        str << "--param " << parfile << " --id " << m_id << " --step "
            << step_size << " --niter " << num_iter;
    }
    else if (inputStyle == "Humppa") {
        str << parfile << " " << m_id << " " << step_size << " "
            << num_iter/step_size;
    }
    else {
        fprintf(stderr, "Invalid argument style: %s\n",
                inputStyle.toStdString().c_str());
        return -1;
    }    

    if (DEBUG_MODE) fprintf(stderr, "cmd: %s\n", m_cmd.toStdString().c_str());

    return 0;
}



/**
 * @brief Returns the last number in a file with numbers running from 1 to n
 *        based on the file size.
 * @param size              File size in bytes.
 * @param cat               Progress file sizes per category (see run() for details).
 * @param trail_size        Number of extra white spaces on a progress file line.
 * @return                  Last number.
 */
int BinaryHandler::calcProgress_( int size, std::vector<long>& cat,
                                   int trail_size )
{
    // Find the correct category.
    uint pos = 0;
    for (auto& i : cat) {
        if (size > i) {
            pos++;
        }
    }
    if (pos >= cat.size()) {
        return -1;
    }

    // The last number in the file is given by the distance to the next size category.
    long l = cat.at(pos) - size;
    int last_num = (int)pow(10,pos) - (double)l/(pos+trail_size+1) - 1;

    return last_num;
}



/**
 * @brief Slot for process signal 'finished()'.
 *
 * Triggers updateModel() in Hampu, so this needs to be called always when exiting,
 * including crash.
 *
 */
void BinaryHandler::binaryFinished_()
{
    // Wait till run() has returned, which means exec() has returned.
    wait();
    emit finished();
}



/**
 * @brief Slot for process signal 'error()'.
 */
void BinaryHandler::binaryError_(QProcess::ProcessError err)
{
    if (m_killedByUser)
        return;

    QString msg = "Fatal error:";
    if (err == QProcess::FailedToStart)
        msg = QString("Fatal error: Failed to start binary '%1'.").arg(m_binary);
    else if (err == QProcess::Crashed)
        msg = QString("Fatal error: Binary '%1' crashed.").arg(m_binary);
    else if (err == QProcess::Timedout)
        msg += " Binary wait timeout.";
    else if (err == QProcess::WriteError)
        msg += " Cannot write process.";
    else if (err == QProcess::ReadError)
        msg += " Cannot read process.";
    else if (err == QProcess::UnknownError)
        msg += " Unknown error.";
    retval = 1;

    qDebug() << msg;
    emit msgStatusBar(msg.toStdString());

    // If the binary failed to start at all, _binary_finished() was never called.
    if (err == QProcess::FailedToStart)
        binaryFinished_();
}


/**
 * @brief Check if output file(s) exist for a given step.
 * @param step      Step number.
 * @return          True if file(s) exist.
 */
bool BinaryHandler::fileExistsForStep_(int step)
{
    auto files = getDataFilenames_(step, true);
    return files.size() > 0;
}



/**
 * @brief Find the last step that has output files.
 * @return          Last existing step number, or -1 if none found.
 */
int BinaryHandler::findLastExistingStep_()
{
    int totalSteps = nIter / stepSize;
    for (int s = totalSteps - 1; s >= 0; s--) {
        if (fileExistsForStep_(s)) {
            return s;
        }
    }
    return -1;
}



/**
 * @brief Read output for a given step and add to toothLife.
 * @param step      Step number.
 * @return          True if successful, false on parse error.
 */
bool BinaryHandler::readStep_(int step)
{
    return addTooth_(step) == 0;
}



/**
 * @brief Main binary tracker loop.
 *
 * Phase 1: While model is running, use look-ahead pattern to avoid reading
 *          files that are still being written.
 * Phase 2: After model exits, read remaining files with timeout for each step.
 *          If model crashed, skip the last (potentially incomplete) file.
 */
void BinaryHandler::run()
{
    const int totalSteps = nIter / stepSize;
    const int timeoutMs = 3000;
    int step = 0;

    // Phase 1: While model is running, use look-ahead.
    while (m_process.state() == QProcess::Running) {
        msleep(UPDATE_INTERVAL);

        // Only read current step if next step exists (ensures current is complete).
        if (fileExistsForStep_(step + 1)) {
            if (readStep_(step)) {
                step++;
            }
        }

        currentIter = (step == 0) ? 0 : (step - 1) * stepSize;
    }

    // Phase 2: Process has exited - read remaining files.
    bool crashed = (m_process.exitStatus() == QProcess::CrashExit);

    // If crashed, skip the last file (may be incomplete). Otherwise read all.
    int maxStep = crashed ? findLastExistingStep_() - 1 : totalSteps;
    if (maxStep < step) {
        maxStep = step - 1;  // Don't go backwards.
    }

    while (step <= maxStep) {
        int waited = 0;

        // Wait for file to appear, with timeout.
        while (!fileExistsForStep_(step) && waited < timeoutMs) {
            msleep(UPDATE_INTERVAL);
            waited += UPDATE_INTERVAL;
        }

        if (!fileExistsForStep_(step)) {
            emit msgStatusBar("Error: Timed out waiting for model output at step "
                              + std::to_string(step) + ".");
            return;
        }

        if (!readStep_(step)) {
            emit msgStatusBar("Error: Failed to parse model output at step "
                              + std::to_string(step) + ".");
            return;
        }

        step++;
        currentIter = step * stepSize;
    }

    // Warn about non-zero exit code (but not crash - that's handled by binaryError_).
    if (m_process.exitCode() != 0 && !crashed) {
        emit msgStatusBar("Warning: Model exited with error code "
                          + std::to_string(m_process.exitCode()) + ".");
    }
}
