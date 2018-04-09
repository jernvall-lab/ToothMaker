/**
 * @file main.cpp
 * @brief The program main.
 *
 * Prints help and calls either GUI or CLI as requested.
 */

#include <QApplication>
#include "gui/hampu.h"
#include "cli/cmdappcore.h"



/**
 * @brief Prints help to command line.
 *
 * --step and --export-images are currently broken; hiding them from the help.
 *
 */
void printHelp()
{
    printf("** MorphoMaker %s **\n", MMAKER_VERSION);
    printf("\n");
    printf("'--help' : This help.\n");
    printf("'--version' : Program version.\n");
    printf("'--niter [no. iter.]' : Number of iterations. Defaults to 10000.\n");
    printf("'--param [par. file]' : A .txt file containing the run-time parameters.\n");
    printf("'--scan [file]' : A .txt listing parameters to scan. Requires a separate\n");
    printf("                  parameters file (--param).\n");
    // printf("'--export-images' : Stores images of the model into ./images/ by every N\n");
    // printf("                    iterations set by --step.'\n");
    // printf("'--step N' : Step size. Use with '--export-images' to store intermedia results\n");
    // printf("             from the model every N iterations.\n");
    printf("'--resolution [pixels]' : Pixel width/height of rendered square images.\n");
    printf("                          Defaults to %d.", SQUARE_WIN_SIZE);
    printf("\n");
}



/**
 * @brief Prints program version to command line.
 */
void printVersion()
{
    printf("%s\n", MMAKER_VERSION);
}



/**
 * @brief Handle command line arguments.
 * @param argc      Number of arguments.
 * @param argv      Array of arguments.
 * @param niter     Number of iterations.
 * @param parfile   Parameter file.
 * @param scanfile  Scan file.
 * @param step      Step size.
 * @param expimg    Export images (1/0).
 * @param res       Resolution for square domain.
 * @return          1 if requested version or help, else 0.
 */
int handleArguments(int argc, char **argv, int *niter, int *parfile, int *scanfile,
                    int *step, int *expimg, int *res)
{
    int i;

    if (!strcmp(argv[1], "--help")) {
        printHelp();
        return 1;
    }
    if (!strcmp(argv[1], "--version")) {
        printVersion();
        return 1;
    }

    for (i=1; i<argc; i++) {
        if (!strcmp(argv[i], "--param")) *parfile=i+1;
        if (!strcmp(argv[i], "--scan")) *scanfile=i+1;
        if (!strcmp(argv[i], "--niter")) *niter=atoi(argv[i+1]);
        if (!strcmp(argv[i], "--step")) *step=atoi(argv[i+1]);
        if (!strcmp(argv[i], "--export-images")) *expimg=1;
        if (!strcmp(argv[i], "--resolution")) *res=atoi(argv[i+1]);
    }

    return 0;
}



/**
 * @brief Message handler for capturing qDebug(), qWarning() etc.
 *        Currently only qDebug is used.
 * @param type      Message type (e.g. qDebug()).
 * @param context   Context.
 * @param msg       Message.
 */
void message_output(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    (void)context;

    QByteArray localMsg = msg.toLocal8Bit();
    QTime t = QTime::currentTime();
    std::string timestamp = t.toString().toStdString();

    if (type == QtDebugMsg) {
        std::cout << "[" << timestamp << "]: " << localMsg.constData() << std::endl;
    }
}



/**
 * @brief Main
 * @param argc      Number of arguments.
 * @param argv      Array of arguments.
 * @return          -1 if errors, else 0.
 */
int main(int argc, char *argv[])
{
    int niter=-1, parfile=0, scanfile=0;
    int step=-1, expimg=0, res=SQUARE_WIN_SIZE;

    if (argc>1) {
        if (handleArguments( argc, argv, &niter, &parfile, &scanfile, &step,
                             &expimg, &res )) {
            return 0;
        }
    }

    qInstallMessageHandler(message_output);

    // Command-line interface:
    if (argc>1 && niter>-1 && parfile>0 && scanfile>0) {
        CmdAppCore cmdAppCore(argc, argv);
        if (cmdAppCore.startParameterScan( niter, argv[parfile], argv[scanfile],
                                           step, expimg, res )) {
            return -1;
        }
        return cmdAppCore.exec();
    }

    // Graphical interface:
    QApplication app(argc, argv);
    Hampu hampu;
    if (hampu.init_GUI()) {
        return -1;
    }
    hampu.show();

    return app.exec();
}
