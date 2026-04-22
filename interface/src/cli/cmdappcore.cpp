/**
 *  @class CmdAppCore
 *  @brief Main CLI application.
 *
 *  Overview:
 *  1) User calls startParameterScan(), which sets up the scan queue and calls
 *     scanParameters().
 *  2) scanParameters() picks the first item in scan queue & calls runModel().
 *  3) Upon model exit updateModel() gets called, which stores the results.
 *  4) updateModel() calls scanParameters(), i.e. back to 2), until the scan
 *     queue is empty and the program exits.
 *
 */

#include <ctime>
#include <iostream>
#include <QDateTime>
#include <QDir>
#include <QApplication>

#include "cli/cmdappcore.h"
#include "misc/binaryhandler.h"
#include "utils/readparameters.h"
#include "utils/readxml.h"
#include "misc/loader.h"



CmdAppCore::CmdAppCore(int & argc, char ** argv) : QCoreApplication(argc, argv)
{
    // Load available models.
    morphomaker::Load_models(models);

    // Rendering engine:
    glengine = new GLEngine;

    // Signals with models/progress monitoring:
    qRegisterMetaType<std::string>("std::string");
    unsigned int i;
    for (i=0; i<models.size(); i++) {
        connect(models.at(i), &Model::msgStatusBar, this, &CmdAppCore::writeStatusBar);
        connect(models.at(i), &Model::finished, this, &CmdAppCore::updateModel);
    }

    runDir = QDir::currentPath();

    // Initializing temporary folder:
    QDir qdir;
    QString tempPath = QDir::tempPath();
    int pid = (int)QCoreApplication::applicationPid();
    QString tmpPath = QString("%1/%2_%3").arg(tempPath, PROGRAM_NAME).arg(pid);
    systemTempPath = tmpPath.toStdString();
    if (!qdir.exists(tmpPath)) {
        qdir.mkdir(tmpPath);
    }
    std::cout << "Temp. folder: " << systemTempPath << std::endl;

    fileIndex = 0;
}



/**
 * @brief Writes text to stdout with carriage return.
 * @param msg   Text to print.
 */
void CmdAppCore::writeStatusBar(std::string msg="")
{
    if (msg.empty()) return;
    fprintf(stdout, "\r%s", msg.c_str());
    fflush(stdout);
}



/**
 * @brief Updates model view window & development slider position.
 * - Called by a QTimer set in the constructor().
 */
void CmdAppCore::updateProgress()
{
    int i;

    for (i=fileIndex; i<toothLife->getLifeSize(); i++) {
        glengine->setRenderMode( models.at(modelId)->getRenderMode() );
        glengine->setVisualData( toothLife, i+1, models.at(modelId) );

        QImage img = glengine->screenshotGL();
        int stepsize = models.at(modelId)->getStepSize();
        QString filename = QString("%1_%2.png")
                           .arg(PROGRAM_NAME)
                           .arg((i+1)*stepsize, 10, 10, QChar('0'));
        QString target = runDir + "/images/" + filename;
        img.save(target);
    }

    fileIndex = i;
}



/**
 *  @brief Called whenever model has finished/exited.
 *  - Updates status bar etc.
 */
void CmdAppCore::updateModel()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    // Reports total running time.
    int timeDiff = time(NULL)-timeStart;
    int hours = timeDiff/(3600);
    int mins = (timeDiff-(hours*3600)) / 60;
    int secs = timeDiff - (hours*3600) - (mins*60);
    QString timeMsg = QString("Finished after %1:%2:%3.")
                      .arg(hours, 2, 10, QChar('0'))
                      .arg(mins, 2, 10, QChar('0'))
                      .arg(secs, 2, 10, QChar('0'));
    writeStatusBar(timeMsg.toStdString());
    fprintf(stdout, "\n");

    Model* model = models.at( modelId );

    if (toothLife->getLifeSize() == 0) {
        fprintf(stderr, "Error: No simulation data available. Skipping rendering.\n");
        delete toothLife;
        scanParameters();
        return;
    }

    glengine->setRenderMode( model->getRenderMode() );
    glengine->setVisualData( toothLife, toothLife->getLifeSize(), model );

    //
    // Render images
    //

    // List of recognized orientations (names + angles)
    std::vector<model::orientation> orients = model->getOrientations();
    // List of requested orientations (names only)
    std::vector<std::string>& req_orients = scanList->getOrientations();

    QString par_id = QString::fromStdString( parameters->getID() );
    QString run_id = QString::number( toothLife->getID() );

    // Save images at the requested orientations, or do nothing node given.
    for (auto orient : req_orients) {
        uint32_t i;
        for (i=0; i<orients.size(); i++) {
            if (!orients.at(i).name.compare( orient )) {
                break;
            }
        }
        if (i == orients.size()) continue;  // Unrecognized orientation requested.

        glengine->setViewOrientation( orients.at(i).rotx, orients.at(i).roty );
        QImage img = glengine->screenshotGL();
        QString target = runDir + "/" + SSHOT_SAVE_DIR + "/" + PROGRAM_NAME
                         + "_" + par_id + "_" + QString::number(i) + ".png";
        std::cout << "Image saved, size " << img.size().height() << "x"
                  << img.size().width() << ", orientation " << orient << std::endl;
        img.save(target);
    }

    //
    // Export data files
    //

    // Create main data folder
    QString folder = runDir + "/" + DATA_SAVE_DIR;
    QDir qdir(folder);
    if (!qdir.exists()) {
        qdir.mkdir(folder);
    }

    // Create an additional subfolder to distiguish between different runs by
    // parameter ID.
    qdir.mkdir(par_id);
    folder = folder + "/" + par_id;

    // Copy simulation output files to the target folder.
    model->exportData( run_id, folder );

    // Run result parsers (cusp analysis, etc.) on the exported data.
    model->runResultParsers( runDir, par_id, folder );

    if (expImg) {
        progressTimer->stop();
        updateProgress();
    }

    // All done, clean up for next run:
    delete toothLife;

    scanParameters();
}



/**
 * @brief Starts the model.
 */
void CmdAppCore::runModel()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    glengine->clearScreen();

    // Run ID uses millisecond timestamp to avoid collisions between consecutive runs.
    // Formula guarantees exactly 10 digits within int32 range [1000000000, 2147483647].
    int run_id = 1000000000 + static_cast<int>(QDateTime::currentMSecsSinceEpoch() % 1147483648LL);
    toothLife = new ToothLife(0, run_id);

    Model* model = models.at(modelId);
    model->setParameters(parameters);
    int stepsize = model->getStepSize();

    models.at(modelId)->init_model( QString(systemTempPath.c_str()), 1,
                                    *toothLife, nIter, stepsize, run_id, -1 );
    timeStart = model->start_model();
}



/**
 * @brief Picks the next item in the scan queue & call runModel().
 */
void CmdAppCore::scanParameters()
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    int nScanItems = scanList->getScanQueueSize();

    parameters = scanList->getScanItem(currentScanItem);
    if (parameters==NULL) {
        fprintf(stdout, "Scanning finished.\n");
        QApplication::exit();
        return;
    }

    fprintf(stdout, "\n*** Scanning item %d/%d (%s), %d iterations ***\n",
            currentScanItem+1, nScanItems, parameters->getID().c_str(), nIter);
    runModel();
    currentScanItem++;
}



/**
 * @brief Determines the model to be used by reading the parameters file.
 * @param pfile     Parameters file.
 * @return
 */
int CmdAppCore::setModel(char *pfile)
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    // Check the model presence & set model ID.
    QString file = runDir + "/" + QString(pfile);
    // NOTE: Running Import_parameters() on an empty Parameters object only
    // reads the keys words and values! The actual parameters are read later
    // once we know the target model.
    Parameters par;
    morphomaker::Import_parameters(file.toStdString(), &par);
    std::string modelName = par.getKey(PARKEY_MODEL);

    modelId = -1;
    for (uint32_t i=0; i<models.size(); i++) {
        if (!modelName.compare(models.at(i)->getModelName())) modelId = i;
    }
    if (modelId==-1) {
        fprintf(stderr, "Error: Unknown model '%s'. Aborted.\n", modelName.c_str());
        return -1;
    }

    // Read model parameters from the par. file:
    morphomaker::Import_parameters(file.toStdString(),
                                   models.at(modelId)->getParameters());
    parameters = new Parameters(models.at(modelId)->getParameters());

    return 0;
}



/**
 * @brief Starts parameters scanning, called by the user.
 * @param niter     Number of iterations to run
 * @param param     Parameters file name
 * @param scanfile  Scan list file name
 * @param step      Step size for storing intermediate results (DISABLED)
 * @param expimg    1 to store images
 * @param res       Image resolution width & height (single value!)
 * @return          -1 if errors, else 0
 */
int CmdAppCore::startParameterScan(int niter, char *param, char *scanfile,
                                   int step, int expimg, int res)
{
    if (DEBUG_MODE) fprintf(stderr, "%s():\n", __FUNCTION__);

    (void)step;

    int rv = glengine->createGLContext();  // Creates off-screen GL context.
    if (rv) {
        QApplication::exit();
        return -1;
    }
    glengine->setScreenResolution(res, res);
    glengine->initializeGL();
    glengine->resizeGL(res, res);

    // Check & set all model related stuff.
    if (setModel(param)) return -1;

    // Read & populate scan list.
    QString source = runDir + "/" + QString(scanfile);
    scanList = morphomaker::Read_scanlist(source.toStdString());
    if (scanList==NULL) {
        fprintf(stderr, "Error: Couldn't construct parameter scan queue.\n");
        return -1;
    }

    QString target = runDir + "/" + SCAN_LIST;
    scanList->setBaseParameters(parameters);
    scanList->populateScanQueue(target.toStdString());
    glengine->setViewMode(scanList->getViewMode());

    nIter = niter;
    expImg = expimg;
    currentScanItem = 0;

    // Create folders for storing model output:
    QDir qdir;
    QString sshotPath = QDir(runDir).filePath(SSHOT_SAVE_DIR);
    if (!qdir.exists(sshotPath)) {
        qdir.mkdir(sshotPath);
    }
    // For 3D models set separate folders for storing objects:
    if (models.at(modelId)->getRenderMode() == RENDER_HUMPPA) {
        QString dataPath = QDir(runDir).filePath(DATA_SAVE_DIR);
        if (!qdir.exists(dataPath)) {
            qdir.mkdir(dataPath);
        }
    }

    if (expImg) {
        QString imagesPath = QDir(runDir).filePath("images");
        if (!qdir.exists(imagesPath)) {
            qdir.mkdir(imagesPath);
        }
        progressTimer = new QTimer(this);
        progressTimer->setInterval(1000);
        connect(progressTimer, &QTimer::timeout, this, &CmdAppCore::updateProgress);
        progressTimer->start();
    }

    scanParameters();

    return 0;
}
