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
    connect(&m_process, SIGNAL(error(QProcess::ProcessError)), this,
            SLOT(binaryError_(QProcess::ProcessError)));
    connect(&m_process, SIGNAL(started()), this, SLOT(start()));

    m_id = 0;
    m_toothLife = NULL;
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
                              const int step_size, const int id)
{
    (void)max_cores;
    m_binary = QString(modelBin.c_str());
    m_id = id;
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
 * @return      Starting time or -1 if errors.
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
    m_process.start(m_cmd);

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

    // Apply parsers
    for (int i=0; i<files.size(); i++) {
        QString file = files.at(i).fileName();

        for (auto& parser : outputParsers) {
            QString path_style = "..\bin\\";
            #if defined(__linux__) || defined(__APPLE__)
            path_style = "../bin/";
            #endif
            QString parser_out = "parser_tmp_" + run_id + ".txt";
            QString cmd = path_style + parser + " " + file + " "
                          + parser_out;

            QProcess process;
            process.start(cmd);
            if(!process.waitForFinished( PARSER_TIMEOUT )) {
                // TODO: Add checks for other errors, e.g., does the parser exist.
               qDebug() << "Error: Parser" << parser << "failed to finish in"
                        << PARSER_TIMEOUT << "msecs on file" << file <<". SKipping.";
               continue;
            }

            // Replace the input file with the parser output if applicable.
            if (QFile::exists(parser_out)) {
                QFile::remove(file);
                QFile::copy(parser_out, file);
                QFile::remove(parser_out);
            }
        }
    }

    // Assuming a fixed output file name for now.
    std::string outfile = std::to_string(iter) + "_" + run_id.toStdString() + ext;
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
            return -1;
        }
    }
    else if (outputStyle == "Matrix") {
        if (morphomaker::Read_BIN_matrix( fname, *tooth )) {
            return -1;
        }
    }
    else if (outputStyle == "Humppa") {
        morphomaker::Read_OFF_file( fname, *tooth );
        if (morphomaker::Read_Humppa_DAD_file( step_test, stepSize, m_id, *tooth )) {
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

    QString path_style = "..\bin\\";
    #if defined(__linux__) || defined(__APPLE__)
    path_style = "../bin/";
    #endif

    m_cmd = "";
    QTextStream str;
    str.setString(&m_cmd);

    // Check if we're dealing with a Python script.
    // It is users responsibility to make sure Python is available!
    QStringList blist = m_binary.split(".");
    if (blist.size() > 1 && blist.at(1) == "py") {
        str << "python ";
    }

    str << path_style << m_binary << " ";
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

    char bin[256];
    strcpy(bin, m_binary.toStdString().c_str());

    char msg[256], type[32] = "Fatal error:";
    if (err == QProcess::FailedToStart)
        sprintf(msg, "%s Failed to start binary '%s'.", type, bin);
    if (err == QProcess::Crashed)
        sprintf(msg, "%s Binary '%s' crashed.", type, bin);
    if (err == QProcess::Timedout)
        strcat(msg, "Binary wait timeout.");
    if (err == QProcess::WriteError)
        strcat(msg, "Cannot write process.");
    if (err == QProcess::ReadError)
        strcat(msg, "Cannot read process.");
    if (err == QProcess::UnknownError)
        strcat(msg, "Unknown error.");
    retval = 1;

    qDebug() << msg;
    emit msgStatusBar(msg);

    // If the binary failed to start at all, _binary_finished() was never called.
    if (err == QProcess::FailedToStart)
        binaryFinished_();
}


/**
 * @brief Main binary tracker loop.
 *
 */
void BinaryHandler::run()
{

    // Per-iteration progress tracking disabled; causes issues with Humppa's
    // various incarnations which write the progress file differently.
/*
    // Pre-calculate the file size categories for calcProgress_.
    // Category n indicates the size of the progress files containing integers
    // in range [1, (10^n)-1].
    // NOTE: This assumes the newline character is 1 byte in size. Windows?
    int trail_size = 0;
    if (outputStyle == "Humppa") {
        trail_size = 1;     // Humppa adds an extra white space to every line in progress file.
    }
    long l = 0;
    std::vector<long> cat = {l};
    for (int i=0; i<10; i++) {      // 10 is just a number 'large enough'.
        l = l + (9 * pow(10,i) * (i+trail_size+2));
        cat.push_back(l);
    }
*/

    int step = 0;   // simulation step currently being processed

    // Process tracking loop.
    while (m_process.state()==QProcess::Running) {
        msleep(UPDATE_INTERVAL);

        // Testing for the presence of the next step here, and then reading the
        // current step only if the next already available. This to avoid reading
        // files that are still being written.
        auto output_files = getDataFilenames_( step+1, true );
        if (output_files.size() > 0) {
            addTooth_(step);
            step++;
        }

        // Per-step progress tracking:
        currentIter = (step == 0) ? 0 : (step-1)*stepSize;
        // Per-iteration progress tracking (see above):
        // currentIter = calcProgress_(m_progressFile.size(), cat, trail_size);
    }

    // Get the rest of the result files still in the sequence.
    while (1) {
        msleep(UPDATE_INTERVAL);

        addTooth_(step);
        // Again, just testing if the files exist; addTooth_() actually reads.
        auto output_files = getDataFilenames_( step+1, true );
        if (output_files.size() > 0) {
            step++;
        }
        else {
            break;
        }
    }
}
