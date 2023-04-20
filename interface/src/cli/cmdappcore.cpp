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
#include <QDir>

#include "cli/cmdappcore.h"
#include "misc/binaryhandler.h"
#include "utils/readparameters.h"
#include "utils/readxml.h"
#include "utils/writedata.h"
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
        connect(models.at(i), SIGNAL(msgStatusBar(std::string)), this,
                SLOT(writeStatusBar(std::string)));
        connect(models.at(i), SIGNAL(finished()), this, SLOT(updateModel()));
    }

    runDir = QDir::currentPath();

    // Initializing temporary folder:
    QDir *qdir = new QDir();
    QString tempPath = qdir->tempPath();
    char tmp[1024];
    int pid = (int)QCoreApplication::applicationPid();
    sprintf(tmp, "%s/%s_%d", tempPath.toStdString().c_str(), PROGRAM_NAME, pid);
    systemTempPath = tmp;
    if (!qdir->exists(QString(systemTempPath.c_str()))) {
        qdir->mkdir(QString(systemTempPath.c_str()));
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
        char tmp[256];
        int stepsize = models.at(modelId)->getStepSize();
        sprintf(tmp, "%s_%.10d.png", PROGRAM_NAME, (i+1)*stepsize);
        QString target = runDir + "/images/" + QString(tmp);
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
    char timeMsg[265];
    sprintf(timeMsg, "Finished after %.2d:%.2d:%.2d.", hours, mins, secs);
    writeStatusBar(timeMsg);
    fprintf(stdout, "\n");

    Model* model = models.at( modelId );
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

    if (model->getRenderMode() == RENDER_HUMPPA) {
        // TODO: Model specific stuff like the following belongs to
        // result parsers, not here.
        Tooth* tooth = toothLife->getTooth( toothLife->getLifeSize()-1 );

        QString file = runDir + "/local_maxima.txt";
        morphomaker::Export_local_maxima( *tooth, file.toStdString(),
                                          par_id.toStdString() );
        file = runDir + "/cuspA_baseline.txt";
        morphomaker::Export_main_cusp_baseline( *tooth, file.toStdString(),
                                                par_id.toStdString() );
    }

    // Apply result parsers on the output files at the export folder.
    model->runResultParsers( runDir );

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

    // NOTE: Model/run ID is set as time(NULL), meaning that if two consequtive
    // model runs occur within one second they are assigned the same ID, leading
    // to undefined behaviour!
    int run_id = time(NULL);
    toothLife = new ToothLife(0, run_id);

    Model* model = models.at(modelId);
    model->setParameters(parameters);
    int stepsize = model->getStepSize();

    models.at(modelId)->init_model( QString(systemTempPath.c_str()), 1,
                                    *toothLife, nIter, stepsize, time(NULL), -1 );
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
    char tmp[1024];
    QDir *qdir = new QDir();
    sprintf(tmp, "%s/%s", runDir.toStdString().c_str(), SSHOT_SAVE_DIR);
    if (!qdir->exists(QString(tmp))) {
        qdir->mkdir(QString(tmp));
    }
    // For 3D models set separate folders for storing objects:
    if (models.at(modelId)->getRenderMode() == RENDER_HUMPPA) {
        sprintf(tmp, "%s/%s", runDir.toStdString().c_str(), DATA_SAVE_DIR);
        if (!qdir->exists(QString(tmp))) {
            qdir->mkdir(QString(tmp));
        }
    }

    if (expImg) {
        sprintf(tmp, "%s/images", runDir.toStdString().c_str());
        if (!qdir->exists(QString(tmp))) {
            qdir->mkdir(QString(tmp));
        }
        progressTimer = new QTimer(this);
        progressTimer->setInterval(1000);
        connect(progressTimer, SIGNAL(timeout()), this, SLOT(updateProgress()));
        progressTimer->start();
    }

    scanParameters();

    return 0;
}
