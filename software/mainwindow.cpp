#include "mainwindow.h"
#include "ui_mainwindow.h"

#define TIMEOUT        20        // timeout period for GUI updates [msec]
#define INCH_TO_METERS 0.0254    // conversion factor between inches and meters
#define PI             3.141592
#define DEBUG          1

using namespace std;

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // does not yet know about exoskeleton, gain tuner, or experiment
    m_exo = NULL;
    m_tuner = NULL;
    m_exp = NULL;
    m_outputFile = NULL;

    // set up GUI and embedded CHAI widget
    ui->setupUi(this);
    if (!ui->visualizer) {
        QMessageBox::information(this, "Application", "Cannot start application.", QMessageBox::Ok);
        close();
    } else {
        ui->visualizer->m_parent = this;
    }

    // configure timer for GUI updates
    updateTimer = new QTimer(this);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateGUI()));
    updateTimer->start(TIMEOUT);

    // connect widgets to eachother
    connect(ui->shouldMin_dial, SIGNAL(valueChanged(int)), ui->shouldMin_lcd, SLOT(display(int)));
    connect(ui->shouldMax_dial, SIGNAL(valueChanged(int)), ui->shouldMax_lcd, SLOT(display(int)));
    connect(ui->elbowMin_dial, SIGNAL(valueChanged(int)), ui->elbowMin_lcd, SLOT(display(int)));
    connect(ui->elbowMax_dial, SIGNAL(valueChanged(int)), ui->elbowMax_lcd, SLOT(display(int)));
    connect(ui->shouldAng_dial, SIGNAL(valueChanged(int)), ui->shouldAng_lcd, SLOT(display(int)));
    connect(ui->elbowAng_dial, SIGNAL(valueChanged(int)), ui->elbowAng_lcd, SLOT(display(int)));

    // neither joint or task space control, in demo mode
    ui->joint_box->setChecked(false);   ui->jointCtrl->hide();
    ui->task_box->setChecked(false);    ui->taskCtrl->hide();
    ui->dither_box->setChecked(false);
    ui->bumpers_box->setChecked(false);
    ui->exp_box->setChecked(false);     m_demo = true;

    // can only tune gains when control is active
    ui->tune_push->setEnabled(false);

    // initialize status bar
    graphicRate.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    ui->statusBar->addPermanentWidget(&graphicRate);
    hapticRate.setFrameStyle(QFrame::Panel | QFrame::Sunken);
    ui->statusBar->addPermanentWidget(&hapticRate);
    graphicRate.setText(QString("GRAPHIC: ---- Hz"));
    hapticRate.setText(QString("HAPTIC: ---- Hz"));

    // define keyboard shortcuts
    QKey = new QShortcut(Qt::Key_Q, this, SLOT(close()));
}

MainWindow::~MainWindow()
{
    ui->visualizer->stop();
    delete ui;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_exp->isVisible()) m_exp->close();  // close experiment window if it is open
    event->accept();
}

void MainWindow::pairWithExo(exo *a_exo)
{
    m_exo = a_exo;
}

void MainWindow::pairWithGainTuner(Dialog_GainTuning *a_tuner)
{
    m_tuner = a_tuner;
}

void MainWindow::pairWithExpDialog(Dialog_Exp *a_addExp)
{
    m_addExp = a_addExp;
}

void MainWindow::pairWithExp(ExpWindow *a_exp)
{
    m_exp = a_exp;
}

void MainWindow::initialize()
{
    // display default subject & control parameters
    syncSubjectDisplay();
    syncControlDisplay();

    // check for queued experiments
    if (m_exp->m_testQueue.empty()) {
        ui->exp_box->setEnabled(false);
    } else {
        ui->exp_box->setEnabled(true);
    }

    // start graphics/haptics for CHAI widget
    if (!ui->visualizer->start()) {
        QMessageBox::warning(this, "CHAI3D", "Cannot start rendering.", QMessageBox::Ok);
        close();
    }
}

void MainWindow::syncSubjectDisplay()
{
    ui->name_fill->setText(QString::fromStdString(m_exo->m_subj->m_name));
    ui->ID_fill->setText(QString::fromStdString(m_exo->m_subj->m_ID));
    ui->age_fill->setText(QString::number(m_exo->m_subj->m_age));
    ui->health_slider->setValue(m_exo->m_subj->m_stroke);
    ui->gender_slider->setValue(m_exo->m_subj->m_female);
    ui->hand_slider->setValue(m_exo->m_subj->m_rightHanded);
    ui->upperarmL_fill->setText(QString::number(m_exo->m_subj->m_Lupper*(1/INCH_TO_METERS)));
    ui->LtoEE_fill->setText(QString::number(m_exo->m_subj->m_LtoEE*(1/INCH_TO_METERS)));
    ui->forearmL_fill->setText(QString::number(m_exo->m_subj->m_Llower*(1/INCH_TO_METERS)));
    ui->shouldMin_dial->setValue((int)(m_exo->m_subj->m_lims.shoul_min*(180/PI)));
    ui->shouldMax_dial->setValue((int)(m_exo->m_subj->m_lims.shoul_max*(180/PI)));
    ui->elbowMin_dial->setValue((int)(m_exo->m_subj->m_lims.elbow_min*(180/PI)));
    ui->elbowMax_dial->setValue((int)(m_exo->m_subj->m_lims.elbow_max*(180/PI)));
}

void MainWindow::syncControlDisplay()
{
    ui->shouldAng_dial->setValue(floor(m_exo->m_thTarg(0)*(180/PI)));
    ui->elbowAng_dial->setValue(floor(m_exo->m_thTarg(1)*(180/PI)));
    ui->X_lcd->display(m_exo->m_posTarg(0));
    ui->Y_lcd->display(m_exo->m_posTarg(1));
}

void MainWindow::saveOneTimeStep()
{
    demo_data temp;

    // record individual parameters
    temp.d_time = m_exo->m_t;
    temp.d_th = m_exo->m_th*(180/PI);
    temp.d_thdot = m_exo->m_thdot*(180/PI);
    temp.d_pos = m_exo->m_pos;
    temp.d_vel = m_exo->m_vel;

    // push into vector for current test
    m_demoData.push_back(temp);
}

void MainWindow::recordForDemo()
{
    // iterate over vector, writing one time step at a time
    for (vector<demo_data>::iterator it = m_demoData.begin() ; it != m_demoData.end(); ++it) {
        if (m_outputFile != NULL)
            fprintf(m_outputFile,"%f, %f, %f, %f, %f, %f, %f, %f, %f\n",
                    it->d_time, it->d_th(0), it->d_th(1), it->d_thdot(0), it->d_thdot(1),
                    it->d_pos(0), it->d_pos(1), it->d_vel(0), it->d_vel(1));
    }
    m_demoData.clear();
}

void MainWindow::updateGUI()
{
    // check for errors with exoskeleton
    if (m_exo->m_error) {

        // turn off control
        m_exo->setCtrl(none);

        // get state of experiment at error
        exp_snapshot snap = m_exp->getSnapshot();
        bool expRunning = snap.p_running;
        test_params currTest = snap.p_test;

        // stop experiment and close window if running
        if (expRunning) {
            m_exp->stopExp();
            m_exp->close();
        }

        // create error message
        string errMessage = "chARM exoskeleton errored ";
        if (expRunning) {
            errMessage += "during experiment at\n";
            errMessage += "\n   TEST: ";
            switch (currTest.p_type) {
            case staircase: errMessage += "staircase"; break;
            case match1D:   errMessage += "1-D matching"; break;
            case match2D:   errMessage += "2-D matching"; break;
            default:        errMessage += "---"; break;
            }
            errMessage += "\n   JOINT: ";
            switch (currTest.p_joint) {
            case S:  errMessage += "shoulder"; break;
            case E:  errMessage += "elbow"; break;
            default: errMessage += "---"; break;
            }
            errMessage += "\n   ACTIVE: ";
            if (currTest.p_active)  errMessage += "yes";
            else                    errMessage += "no";
            errMessage += "\n   VISION: ";
            if (currTest.p_vision)  errMessage += "yes";
            else                    errMessage += "no";
            errMessage += "\n\nfor the following reason(s):";
        } else {
            errMessage += "for the following reason(s):";
        }
        errMessage += "\n" + m_exo->m_errMessage;

        // display message in pop-up window
        QMessageBox msgBox;
        msgBox.setText("EXOSKELETON ERROR!");
        msgBox.setInformativeText(QString::fromStdString(errMessage));
        msgBox.exec();

        // quit application
        QCoreApplication::quit();
    }

    // check for queued experiments, since they can be added "on the fly"
    if (m_exp->m_testQueue.empty()) {
        ui->exp_box->setEnabled(false);
    } else {
        ui->exp_box->setEnabled(true);
    }

    // update exoskeleton state
    ui->th1_lcd->display(m_exo->m_th(0)*(180/PI));
    ui->th1dot_lcd->display(m_exo->m_thdot(0)*(180/PI));
    ui->th2_lcd->display(m_exo->m_th(1)*(180/PI));
    ui->th2dot_lcd->display(m_exo->m_thdot(1)*(180/PI));

    // update status bar
    graphicRate.setText(QString("GRAPHIC: %1 Hz").arg((int)(ui->visualizer->m_graphicRate.getFrequency()), 3));
    hapticRate.setText(QString("HAPTIC: %1 Hz").arg((int)(ui->visualizer->m_hapticRate.getFrequency()), 4));

    // update debugging parameters
    if (DEBUG) {
        ui->time_lcd->display(m_exo->m_t);
        ui->x_lcd->display(m_exo->m_pos(0));
        ui->y_lcd->display(m_exo->m_pos(1));
        ui->xdot_lcd->display(m_exo->m_vel(0));
        ui->ydot_lcd->display(m_exo->m_vel(1));
        ui->th1Err_lcd->display(m_exo->m_thErr(0)*(180/PI));
        ui->th2Err_lcd->display(m_exo->m_thErr(1)*(180/PI));
        ui->th1dotErr_lcd->display(m_exo->m_thdotErr(0)*(180/PI));
        ui->th2dotErr_lcd->display(m_exo->m_thdotErr(1)*(180/PI));
        ui->th1intErr_lcd->display(m_exo->m_thErrInt(0)*(180/PI));
        ui->th2intErr_lcd->display(m_exo->m_thErrInt(1)*(180/PI));
        ui->xErr_lcd->display(m_exo->m_posErr(0));
        ui->yErr_lcd->display(m_exo->m_posErr(1));
        ui->xdotErr_lcd->display(m_exo->m_velErr(0));
        ui->ydotErr_lcd->display(m_exo->m_velErr(1));
        ui->xintErr_lcd->display(m_exo->m_posErrInt(0));
        ui->yintErr_lcd->display(m_exo->m_posErrInt(1));
        ui->T1_lcd->display(m_exo->m_T(0));
        ui->T2_lcd->display(m_exo->m_T(1));
        ui->Fx_lcd->display(m_exo->m_F(0));
        ui->Fy_lcd->display(m_exo->m_F(1));
    }

    if (recording) saveOneTimeStep();
}

void MainWindow::on_save_push_clicked()
{
    // get parameters from GUI
    string name = ui->name_fill->text().toStdString();
    string ID = ui->ID_fill->text().toStdString();
    int age = ui->age_fill->text().toInt();
    bool stroke = (bool)ui->health_slider->value();
    bool gender = (bool)ui->gender_slider->value();
    bool rightHanded = (bool)ui->hand_slider->value();
    double Lupper = ui->upperarmL_fill->text().toDouble();
    double LtoEE = ui->LtoEE_fill->text().toDouble();
    double Llower = ui->forearmL_fill->text().toDouble();
    jointLims lims;
    lims.shoul_min = ui->shouldMin_dial->value();
    lims.shoul_max = ui->shouldMax_dial->value();
    lims.elbow_min = ui->elbowMin_dial->value();
    lims.elbow_max = ui->elbowMax_dial->value();

    // update subject
    m_exo->m_subj->update(name, ID, age,
                          stroke, gender, rightHanded,
                          Lupper, LtoEE, Llower, lims);
}

void MainWindow::on_addExp_push_clicked()
{
    m_addExp->reset();
    m_addExp->show();
}

void MainWindow::on_calibrate_push_clicked()
{
    m_exo->calibrate();
}

void MainWindow::on_joint_box_stateChanged(int newState)
{
    if (newState == Qt::Checked) {

        // ensure only joint control is available
        if (ui->task_box->isChecked()) {
            ui->task_box->setChecked(false);
            ui->taskCtrl->hide();
        }

        // enable joint control, with option for gain tuning
        m_exo->setCtrl(joint);
        syncControlDisplay();
        ui->jointCtrl->show();
        ui->tune_push->setEnabled(true);

    } else {
        ui->jointCtrl->hide();
        if (!ui->task_box->isChecked()) {
            m_exo->setCtrl(none);
            ui->tune_push->setEnabled(false);
        }
    }
}

void MainWindow::on_task_box_stateChanged(int newState)
{
    if (newState == Qt::Checked) {

        // ensure only task control is available
        if (ui->joint_box->isChecked()) {
            ui->joint_box->setChecked(false);
            ui->jointCtrl->hide();
        }

        // enable task control, with option for gain tuning
        m_exo->setCtrl(task);
        syncControlDisplay();
        ui->taskCtrl->show();
        ui->tune_push->setEnabled(true);

    } else {
        ui->taskCtrl->hide();
        if (!ui->joint_box->isChecked()) {
            m_exo->setCtrl(none);
            ui->tune_push->setEnabled(false);
        }
    }
}

void MainWindow::on_dither_box_stateChanged(int newState)
{
    if (newState == Qt::Checked) {
        m_exo->m_dither = true;
        ui->tune_push->setEnabled(true);
    } else {
        m_exo->m_dither = false;
        if (!ui->joint_box->isChecked() && !ui->task_box->isChecked())
            ui->tune_push->setEnabled(false);
    }
}

void MainWindow::on_bumpers_box_stateChanged(int newState)
{
    if (newState == Qt::Checked) m_exo->m_bumpers = true;
    else                         m_exo->m_bumpers = false;
}

void MainWindow::on_exp_box_stateChanged(int newState)
{
    // disable GUI-level control in experiment mode
    if (newState == Qt::Checked) {
        m_demo = false;
        if (ui->joint_box->isChecked()) {
            ui->joint_box->setChecked(false);
            ui->jointCtrl->hide();
        } else if (ui->task_box->isChecked()) {
            ui->task_box->setChecked(false);
            ui->taskCtrl->hide();
        }
        ui->joint_box->setEnabled(false);
        ui->task_box->setEnabled(false);
        ui->tune_push->setEnabled(true);
        ui->START_STOP_push->setText("START EXPERIMENT");
    } else {
        m_demo = true;
        ui->joint_box->setEnabled(true);
        ui->task_box->setEnabled(true);
        ui->tune_push->setEnabled(false);
        ui->START_STOP_push->setText("START DATA RECORDING");
    }
}

void MainWindow::on_tune_push_clicked()
{
    m_tuner->syncGains();
    m_tuner->show();
}

void MainWindow::on_shouldAng_dial_sliderMoved(int position)
{
    m_exo->m_onTraj = false;
    m_exo->m_thTarg(0) = position*(PI/180);
}

void MainWindow::on_elbowAng_dial_sliderMoved(int position)
{
    m_exo->m_onTraj = false;
    m_exo->m_thTarg(1) = position*(PI/180);
}

void MainWindow::on_go_push_clicked()
{
    // send exo on trajectory to target already set by dials
    m_exo->setTarg(m_exo->m_thTarg, joint);
}

void MainWindow::on_START_STOP_push_clicked()
{
    // check state of button
    string buttonText = ui->START_STOP_push->text().toStdString();
    bool toStart = (strcmpi(buttonText.c_str(), "START DATA RECORDING") == 0) ||
                   (strcmpi(buttonText.c_str(), "START EXPERIMENT") == 0);

    // ready to start demo or experiment
    if (toStart) {

        // if demo mode, start saving exo state data
        if (m_demo) {
            sprintf(filename, "test.csv");
            m_outputFile = fopen(filename, "w");
            fprintf(m_outputFile, "Time [sec], Shoulder Pos [deg], Elbow Pos [deg], Shoulder Vel [deg/s], Elbow Vel [deg/s], "
                                  "Hand PosX [m], Hand PosY [m], Hand VelX [m/s], Hand VelY [m/s]\n");
            m_demoData.clear();
            recording = true;
            ui->START_STOP_push->setText("STOP DATA RECORDING");
        }

        // if experiment mode, start experiment
        else {
            m_exp->show();
            m_exp->startExp();
            ui->START_STOP_push->setText("STOP EXPERIMENT");
        }

        // disable experiment mode checkbox
        ui->exp_box->setEnabled(false);
    }

    // ready to stop demo or experiment
    else {

        // if demo mode, record data
        if (m_demo) {
            recordForDemo();
            if (m_outputFile != NULL)  fclose(m_outputFile);
            recording = false;
            if (!m_exp->m_testQueue.empty()) ui->exp_box->setEnabled(true);
            ui->START_STOP_push->setText("START DATA RECORDING");
        }

        // if experiment mode, stop experiment
        else {
            m_exp->stopExp();
            m_exp->hide();
            if (m_exp->m_testQueue.empty()) {
                ui->exp_box->setChecked(false);
                ui->exp_box->setEnabled(false);
                ui->START_STOP_push->setText("START DATA RECORDING");
            } else {
                ui->exp_box->setEnabled(true);
                ui->START_STOP_push->setText("START EXPERIMENT");
            }
        }
    }
}

void MainWindow::onExperimentStop()
{
    if (m_exp->m_testQueue.empty()) {
        ui->exp_box->setChecked(false);
        ui->exp_box->setEnabled(false);
        ui->START_STOP_push->setText("START DATA RECORDING");
    } else {
        ui->exp_box->setEnabled(true);
        ui->START_STOP_push->setText("START EXPERIMENT");
    }
}

void MainWindow::toggleDither()
{
    m_exo->m_dither = !(m_exo->m_dither);
    ui->dither_box->setChecked(m_exo->m_dither);
}
