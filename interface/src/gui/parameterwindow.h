#pragma once

#include <QWidget>
#include <QtGui>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QLabel>
#include <QXmlStreamReader>
#include <QCheckBox>

#include "morphomaker.h"
#include "model.h"


// Need to add some platform-specific paddigns to make button and field align.
#if defined(__linux__)
#define BUTTON_V_PADDING 0
#define FIELD_V_PADDING 0
#define FIELD_H_PADDING 7
#else
#define BUTTON_V_PADDING -1
#define FIELD_V_PADDING 4
#define FIELD_H_PADDING 0
#endif



class ParameterWindow : public QWidget
{
    Q_OBJECT

    public:
        ParameterWindow(QWidget *parent=0);
        ~ParameterWindow();

        void setModel( Model* );
        void setParameters( Parameters& );

        void addFileDialog( QString&, int, int );
        void updateButtonValues();

    private slots:
        void setParValue(int);
        void checkbox_state(int);
        void infoBox(int);
        void importFile();

    signals:
        void parameter_changed();

    private:
        void paintEvent(QPaintEvent *);

        void addValueField_(int, int, int, bool=true);
        void createButton_(int, int, int, int width=60, bool show=true);
        void addCheckbox_( QString text, int x, int y, int i );

        std::vector<QLabel*>        fileLabels_;     // File dialog button objects
        std::vector<QLineEdit*>     valueFields_;    // Value field objects
        std::vector<QCheckBox*>     checkboxes_;
        QStringList                 names_;
        QStringList                 notes_;
        std::vector<QPushButton*>   buttons_;        // Parameter button objects

        QStringList                 buttonNames;    // Parameter button names
        QStringList                 buttonNotes;    // Parameter button descriptions

        Model*                      model;          // Associated model object
        std::vector<std::string>    modelFiles;
};
