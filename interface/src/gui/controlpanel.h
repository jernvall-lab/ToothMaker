#pragma once

#include <QtGlobal>
#if QT_VERSION >= 0x050000
#include <QtWidgets>
#endif
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QLabel>
#include <QCheckBox>
#include <QSlider>
#include <QSpacerItem>
#include <QSpinBox>
#include <QFileDialog>
#include <QDir>
#include <QPainter>
#include <sstream>
#include "morphomaker.h"
#include "parameterwindow.h"
#include "model.h"

// The minimum dimensions of control panel widget:
#define CONTROLPANEL_WIDTH  1000
#define CONTROLPANEL_HEIGHT 110

#define COMBO_WIDTH 185             // Width of the combo boxes.
#define ORIENT_WIDTH 110            // Orientation width.
#define FOLLOW_DEFAULT true         // Boolean for follow development default.
#define DEV_SLIDER_WIDTH 392        // Development slider width.

class ControlPanel : public QWidget
{
    Q_OBJECT

    public:
        ControlPanel( QWidget *parent=0, std::vector<Model*> *m=NULL ) ;
        void updateRunStatus(QString);
        void setSliderMinMax(int, int);
        void setSliderValue(int);
        int getSliderValue();
        int addHistory(int);
        void endHistory(int);
        void removeHistory(int);
        void resetOrientation(int);
        void setModelIndex(int);

        void setViewMode(int);
        void setViewThreshold(double);
        void setnIter(int);

        void setOrientations( const std::vector<model::orientation>& );
        void showCellConnections(bool, bool);
        void enableRunButton(int);
        void enableModelList(int);
        void enableHistory(int);
        void setViewModeBox(const std::vector<model::view_mode>& , int);
        int getnIter();

    private slots:
        void cellConnections(int);
        void changeHistory(int);

        void changeViewMode(int);
        void viewThreshold(const QString &);
        void changeIterations(int);

        // Development slider:
        void follow_development(int);
        void slider_step_view();
        void slider_active();
        void slider_inactive();

        void handleRunButton();
        void readLineValue(int);
        void changeOrientation(int);
        void modelIndex(int);

        // Parameter import/export:
        void readParameters();
        void saveParameters();

    signals:
        void showMesh(int);
        void historyIndex(int);

        void viewmode(int);
        void thresholdChange(double);
        void setIterations(int);

        void followDevel(int);
        void changeStepView(int);
        void viewOrientation(int);
        void changeModel(int);
        // Parameter import/export:
        void msgStatusBar(std::string);
        void importFile(std::string);
        void exportFile(std::string);
        // Running:
        void startModel(int);
        void killModel();

    private:
        void paintEvent(QPaintEvent*);
        bool eventFilter(QObject *, QEvent *);

        QPushButton *createButton(int, int, int, const QString &, const char *);
        QComboBox *modelBox(std::vector<Model*>*, int, int);
        void viewModeBox(int, int);
        QComboBox *orientationBox(int, int);
        QSpinBox *createSpinBox(int, int, const char *);
        QCheckBox *createCheckBox(int, int, const QString &, const char *);

        // Control menus etc.
        QComboBox *viewMode;
        QLineEdit *threshold;
        QComboBox *model;
        QComboBox *orientations;
        QCheckBox *showGrid;
        QSlider *develSlider;
        QPushButton *runButton;
        QComboBox *history;
        QSpinBox *iterations;

        QString runStatus;          // Run button text & status indicator.
        int currentRunIndex;        // History entry number.
        int nIter;                  // Number of iterations.
        QTimer* sliderTimer;        // Timer for checking development slider.
        bool sliderUpdate;         // True if user inacting with devel. slider.
};
