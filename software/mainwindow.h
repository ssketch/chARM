#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chai3d.h"
#include "subject.h"
#include "exo.h"
#include "dialog_gaintuning.h"
#include "dialog_exp.h"
#include "expwindow.h"
#include <cstdio>
#include <fstream>
#include <string>
#include <cmath>
#include <vector>
#include <QCoreApplication>
#include <QMainWindow>
#include <QGLWidget>
#include <QTimer>
#include <QMessageBox>
#include <QShortcut>
#include <QLabel>

// let main window know about classes defined later
class Dialog_GainTuning;
class Dialog_Exp;
class ExpWindow;

typedef struct
{
    double d_time;
    chai3d::cVector3d d_th;
    chai3d::cVector3d d_thdot;
    chai3d::cVector3d d_pos;
    chai3d::cVector3d d_vel;

} demo_data;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    exo* m_exo;                  // pointer to exoskeleton associated with widget
    Dialog_GainTuning* m_tuner;  // pointer to dialog window for gain tuning
    Dialog_Exp* m_addExp;        // pointer to dialog window for adding to experiment test queue
    ExpWindow* m_exp;            // pointer to experiment window
    bool m_demo;                 // TRUE = demo mode, FALSE = experiment mode

    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void pairWithExo(exo *a_exo);
    void pairWithGainTuner(Dialog_GainTuning *a_tuner);
    void pairWithExpDialog(Dialog_Exp *a_addExp);
    void pairWithExp(ExpWindow *a_exp);
    void initialize();
    void syncSubjectDisplay();
    void syncControlDisplay();

    // functions allowing for GUI changes from experiment window
    void onExperimentStop();
    void toggleDither();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::MainWindow* ui;                 // pointer to user interface
    QTimer* updateTimer;                // timer for GUI updates
    QLabel graphicRate;                 // graphic update frequency label for status bar
    QLabel hapticRate;                  // haptic update frequency label for status bar
    QShortcut* QKey;                    // shortcut key for quitting application

    bool recording;                     // TRUE = record exo state data
    char filename[100];                 // output filename
    FILE* m_outputFile;                 // data file
    std::vector<demo_data> m_demoData;  // data for one demo

    void saveOneTimeStep();
    void recordForDemo();

private slots:
    void updateGUI();
    void on_save_push_clicked();
    void on_addExp_push_clicked();
    void on_calibrate_push_clicked();
    void on_joint_box_stateChanged(int newState);
    void on_task_box_stateChanged(int newState);
    void on_dither_box_stateChanged(int newState);
    void on_bumpers_box_stateChanged(int newState);
    void on_exp_box_stateChanged(int newState);
    void on_tune_push_clicked();
    void on_shouldAng_dial_sliderMoved(int position);
    void on_elbowAng_dial_sliderMoved(int position);
    void on_go_push_clicked();
    void on_START_STOP_push_clicked();
};

#endif // MAINWINDOW_H
