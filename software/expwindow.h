#ifndef EXPWINDOW_H
#define EXPWINDOW_H

#include "mainwindow.h"
#include <queue>
#include <cmath>
#include <QMainWindow>
#include <QGLWidget>
#include <QMessageBox>
#include <QShortcut>

class MainWindow;

// enumeration of test types
typedef enum
{
    staircase,
    match1D,
    match2D,
    NUM_TESTS
} test_types;

// structure capturing all attributes of a test
typedef struct
{
    test_types p_type;    // type of test
    joints p_joint;       // joint(s) being tested
    bool p_active;        // movement condition (TRUE = active, FALSE = passive)
    bool p_vision;        // vision condition (TRUE = provided, FALSE = blind)
    int p_time;           // time allotted for trials (soft limit for staircase trials) [sec]
    double p_lock = NAN;  // if 1-D test, angle at which untested joint is "locked" (NAN if 2-D test) [deg]
} test_params;

// structure capturing attributes of a trial
typedef struct
{
    double p_ref;               // reference/starting joint angle (if 1-D test) or circular reach angle (if 2-D test) [deg]
    double p_targ;              // comparison/target joint angle (if 1-D test) or circular reach angle (if 2-D test) [deg]
    bool p_isPractice = false;  // default = non-practice
} trial_params;

// structure capturing shapshot of experiment, for debugging
typedef struct {
    bool p_running;
    test_params p_test;
    trial_params p_trial;
} exp_snapshot;

namespace Ui {
class ExpWindow;
}

class ExpWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow* m_parent;                 // pointer to main window

    std::queue<test_params> m_testQueue;  // FIFO list of test parameters

    explicit ExpWindow(QWidget *parent = 0);
    ~ExpWindow();

    void pairWithMainWindow(MainWindow *a_parent);
    void startExp();
    void stopExp();
    exp_snapshot getSnapshot();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    Ui::ExpWindow *ui;  // pointer to user interface
    QShortcut* QKey;    // shortcut key for stopping experiment
    QShortcut* FKey;    // shortcut key for going fullscreen
};

#endif // EXPWINDOW_H
