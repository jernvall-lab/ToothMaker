#ifndef PARAMETERWINDOW_H
#define PARAMETERWINDOW_H

#include <iostream>
#include <QWidget>
#include <QtGui>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>
#include <QFileDialog>
#include <QLabel>
#include <QXmlStreamReader>

#include <morphomaker.h>
#include <model.h>



class ParameterWindow : public QWidget
{
    Q_OBJECT

    public:
        ParameterWindow(QWidget *parent=0);
        ~ParameterWindow();

        void set_model( Model* );
        void set_parameter_buttons( Parameters& );
        void add_orientation( orientation& );
        void add_file_dialog( QString&, int, int );
        void updateButtonValues();

    private:
        void paintEvent(QPaintEvent *);

    private slots:
        void setParValue(int);
        void infoBox(int);
        void importFile();

    protected:
        void addValueField(int, int, bool=true);
        void createButton(int, int, int, int width=67, bool show=true);

        std::vector<QLabel*> fileLabels;        // File dialog button objects
        std::vector<QLineEdit*> valueFields;    // Value field objects

        std::vector<QPushButton*> buttons;      // Parameter button objects
        QStringList buttonNames;                // Parameter button names
        QStringList buttonNotes;                // Parameter button descriptions

        Model *model;                           // Associated model object
        std::vector<std::string> modelFiles;
};

#endif
