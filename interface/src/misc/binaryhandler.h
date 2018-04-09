#ifndef BINARYHANDLER_H
#define BINARYHANDLER_H

#include <QProcess>
#include <QThread>
#include <QFile>
#include "model.h"

#define DEFAULT_TOOTH_COL 0.5   // Default tooth color. 0.5 means middle gray.

class BinaryHandler : public Model
{
    Q_OBJECT

    public:
        BinaryHandler();
        ~BinaryHandler();
        int init_model(const QString&, const int, ToothLife&, const int, const int,
                       const int);
        int start_model();
        void stop_model();
        Mesh& fill_mesh( Tooth& );

    private:
        std::vector<std::string> _get_data_filenames(int, bool);
        int _add_tooth(const int);
        int _set_temp_env(const QString&);
        int _set_bin_settings(const QString&, const int, const int);
        int _calc_progress( int, std::vector<long>&, int );

        QProcess _process;
        QFile _progress_file;
        QString _binary;
        QString _cmd;
        bool _killed_by_user;

        int _id;                        // Simulation run ID.
        ToothLife* _toothLife;          // Simulation history.

    private slots:
        void _binary_finished();
        void _binary_error(QProcess::ProcessError);

    protected:
        void run();
};

#endif // BINARYHANDLER_H
