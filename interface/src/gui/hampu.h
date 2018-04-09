#ifndef HAMPU_H
#define HAMPU_H

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QWidget>
#include <QGroupBox>
#include <QProcess>
#include <iostream>
#include <QGLFormat>
#include <QDir>
#include <QTimer>

#include "gui/glwidget.h"
#include "morphomaker.h"
#include "tooth.h"
#include "gui/controlpanel.h"
#include "gui/parameterwindow.h"
#include "parameters.h"
#include "gui/scanwindow.h"

#define EXPORT_DATA         0x01
#define EXPORT_SCREENSHOTS  0x02
#define STATUSBAR_QUIET     1
#define STATUSBAR_VERBOSE   0


class Hampu : public QMainWindow
{
    Q_OBJECT

    public:
        explicit Hampu(QWidget *parent = 0);
        ~Hampu();
        int init_GUI();

    private slots:        
        // Control panel actions.
        void Panel_ViewMode(int);
        void Panel_ViewThreshold(double);
        void Panel_Orientation(int);
        void Panel_CellConnections(int);
        void Panel_Model(int);
        void Panel_History(int);
        void Panel_Import(std::string);
        void Panel_Export(std::string);
        void Panel_Development(int);
        void Panel_Iterations(int);
        void Panel_Run(int);
        void Panel_Stop();
        void Panel_FollowDevelopment(int);

        // Menu bar: File, Tools, Options.
        void File_Exit();
        void Tools_ExportObjects();
        void Tools_ExportImages();
        void Tools_ScanParameters();
        void Options_PurgeHistory();
        // void Options_Preferences();

        void startParameterScan();
        void stopParameterScan();

        void resetOrientation(int);
        void setModelSettings(int, int);
        int exportModelData(int, int, QString);
        void setVisualData();
        void writeStatusBar(std::string);

        void screenshotWidget();
        void updateProgress();
        void updateModel();
        void viewIntSteps(int);

    private:
        void _set_signals();
        void _set_menu_bar();

        void _update_current_step_view(int);
        void _scan_parameters();
        void _import_example_parameters();
        unsigned int _get_max_history_size();

        void keyPressEvent(QKeyEvent *);
        void dragEnterEvent(QDragEnterEvent *);
        void dropEvent(QDropEvent *);

        QGridLayout *mainLayout;
        ParameterWindow *parwidget;             // Current parameter window
        GLWidget *glwidget;                     // OpenGL view widget
        ControlPanel *controlPanel;             // Control panel widget
        ScanWindow *scanWindow;                 // Parameter scanning window
        QTimer *progressTimer;                  // Visuals update timer

        std::vector<Model*> models;             // Attached model objects
        std::vector<ParameterWindow*> parameterWindows; // Model parameter windows

        ToothLife *toothLifeWork;               // Currently active ToothLife object
        std::vector<ToothLife*> toothHistory;   // Model history
        uint currentHistory;                    // Index of currently viewed history item
        std::string tempPathMorpho;             // System temporary files path
        int currentModel;                       // Index of the currently viewed model

        int followDevelopment;                  // 1 if 'Follow development' checked
        int viewIntStep;                        // Currently viewed step
        int timeStart;                          // Model start time
        int screenshotCounter;                 // Screenshot image ID for file name
        int runCounter;                        // Incremented at model model start.

        ScanList *scanList;                     // List of parameters to scan
        int scanning;                           // Scanning status (1=scanning, 0=not)
        Parameters *baseParameters;             // Initial parameters for scanning
};

#endif
