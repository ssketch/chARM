#include "dialog_setup.h"
#include "ui_dialog_setup.h"

#define T_1D_2AFC   5          // time allotted for each 2AFC judgement
#define T_1D_ACTV   10         // time allotted for each 1-D active matching trial
#define T_1D_PASS   15         // time allotted for each 1-D passive matched trial
#define T_2D_ACTV   10         // time allotted for each 2-D active matching trial
#define T_2D_PASS   25         // time allotted for each 2-D passive matching trial
#define INCH_TO_METERS 0.0254  // conversion factor between inches and meters

Dialog_Setup::Dialog_Setup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_Setup)
{
    // does not yet know about main window
    m_parent = NULL;

    ui->setupUi(this);

    // set inputs for default experiment
    ui->staircase_box->setChecked(true);
    ui->match1D_box->setChecked(true);
    ui->match2D_box->setChecked(true);

    ui->match1Dtest1_combo->setEnabled(true);   ui->match1Dtest1_combo->setCurrentText("passive");
    ui->match1Dtest2_combo->setEnabled(true);   ui->match1Dtest2_combo->setCurrentText("active");
    ui->match1Dtest3_combo->setEnabled(true);   ui->match1Dtest3_combo->setCurrentText("active");
    ui->match2Dtest1_combo->setEnabled(true);   ui->match2Dtest1_combo->setCurrentText("passive");
    ui->match2Dtest2_combo->setEnabled(true);   ui->match2Dtest2_combo->setCurrentText("active");
    ui->match2Dtest3_combo->setEnabled(true);   ui->match2Dtest3_combo->setCurrentText("active");

    ui->vision_box_1->setEnabled(false);    ui->vision_box_1->setChecked(false);
    ui->vision_box_2->setEnabled(true);     ui->vision_box_2->setChecked(false);
    ui->vision_box_3->setEnabled(true);     ui->vision_box_3->setChecked(true);
    ui->vision_box_4->setEnabled(false);    ui->vision_box_4->setChecked(false);
    ui->vision_box_5->setEnabled(true);     ui->vision_box_5->setChecked(false);
    ui->vision_box_6->setEnabled(true);     ui->vision_box_6->setChecked(true);

    ui->shoulder_box_1->setEnabled(true);   ui->shoulder_box_1->setChecked(true);
    ui->shoulder_box_2->setEnabled(true);   ui->shoulder_box_2->setChecked(true);
    ui->shoulder_box_3->setEnabled(true);   ui->shoulder_box_3->setChecked(true);
    ui->shoulder_box_4->setEnabled(true);   ui->shoulder_box_4->setChecked(true);

    ui->elbow_box_1->setEnabled(true);  ui->elbow_box_1->setChecked(true);
    ui->elbow_box_2->setEnabled(true);  ui->elbow_box_2->setChecked(true);
    ui->elbow_box_3->setEnabled(true);  ui->elbow_box_3->setChecked(true);
    ui->elbow_box_4->setEnabled(true);  ui->elbow_box_4->setChecked(true);
}

Dialog_Setup::~Dialog_Setup()
{
    delete ui;
}

void Dialog_Setup::pairWithMainWindow(MainWindow *a_parent)
{
    m_parent = a_parent;
}

void Dialog_Setup::on_staircase_box_stateChanged(int newState)
{
    if (newState == Qt::Checked) {  // both shoulder & elbow by default
        ui->shoulder_box_1->setEnabled(true);  ui->elbow_box_1->setEnabled(true);
        ui->shoulder_box_1->setChecked(true);  ui->elbow_box_1->setChecked(true);
    } else {
        ui->shoulder_box_1->setEnabled(false);  ui->elbow_box_1->setEnabled(false);
        ui->shoulder_box_1->setChecked(false);  ui->elbow_box_1->setChecked(false);
    }
}

void Dialog_Setup::on_match1D_box_stateChanged(int newState)
{
    if (newState == Qt::Checked) {

        // enable comboboxes
        ui->match1Dtest1_combo->setEnabled(true);
        ui->match1Dtest2_combo->setEnabled(true);
        ui->match1Dtest3_combo->setEnabled(true);

        // if combobox not "empty", enable associated checkboxes
        if (strcmpi(ui->match1Dtest1_combo->currentText().toStdString().c_str(),"") != 0) {
            ui->vision_box_1->setEnabled(true);
            ui->shoulder_box_2->setEnabled(true);
            ui->elbow_box_2->setEnabled(true);
        } else {
            ui->vision_box_1->setEnabled(false);
            ui->shoulder_box_2->setEnabled(false);
            ui->elbow_box_2->setEnabled(false);
        }
        if (strcmpi(ui->match1Dtest2_combo->currentText().toStdString().c_str(),"") != 0) {
            ui->vision_box_2->setEnabled(true);
            ui->shoulder_box_3->setEnabled(true);
            ui->elbow_box_3->setEnabled(true);
        } else {
            ui->vision_box_2->setEnabled(false);
            ui->shoulder_box_3->setEnabled(false);
            ui->elbow_box_3->setEnabled(false);
        }
        if (strcmpi(ui->match1Dtest3_combo->currentText().toStdString().c_str(),"") != 0) {
            ui->vision_box_3->setEnabled(true);
            ui->shoulder_box_4->setEnabled(true);
            ui->elbow_box_4->setEnabled(true);
        } else {
            ui->vision_box_3->setEnabled(false);
            ui->shoulder_box_4->setEnabled(false);
            ui->elbow_box_4->setEnabled(false);
        }
    } else {

        // disable all combo/checkboxes
        ui->match1Dtest1_combo->setEnabled(false);
        ui->match1Dtest2_combo->setEnabled(false);
        ui->match1Dtest3_combo->setEnabled(false);
        ui->vision_box_1->setEnabled(false);
        ui->vision_box_2->setEnabled(false);
        ui->vision_box_3->setEnabled(false);
        ui->shoulder_box_2->setEnabled(false);  ui->elbow_box_2->setEnabled(false);
        ui->shoulder_box_3->setEnabled(false);  ui->elbow_box_3->setEnabled(false);
        ui->shoulder_box_4->setEnabled(false);  ui->elbow_box_4->setEnabled(false);
    }
}

void Dialog_Setup::on_match2D_box_stateChanged(int newState)
{
    if (newState == Qt::Checked) {

        // enable comboboxes
        ui->match2Dtest1_combo->setEnabled(true);
        ui->match2Dtest2_combo->setEnabled(true);
        ui->match2Dtest3_combo->setEnabled(true);

        // if combobox not "empty", enable associated checkboxes
        if (strcmpi(ui->match2Dtest1_combo->currentText().toStdString().c_str(),"") != 0)
                ui->vision_box_4->setEnabled(true);
        else    ui->vision_box_4->setEnabled(false);
        if (strcmpi(ui->match2Dtest2_combo->currentText().toStdString().c_str(),"") != 0)
                ui->vision_box_5->setEnabled(true);
        else    ui->vision_box_5->setEnabled(false);
        if (strcmpi(ui->match2Dtest3_combo->currentText().toStdString().c_str(),"") != 0)
                ui->vision_box_6->setEnabled(true);
        else    ui->vision_box_6->setEnabled(false);
    } else {

        // disable all combo/checkboxes
        ui->match2Dtest1_combo->setEnabled(false);
        ui->match2Dtest2_combo->setEnabled(false);
        ui->match2Dtest3_combo->setEnabled(false);
        ui->vision_box_4->setEnabled(false);
        ui->vision_box_5->setEnabled(false);
        ui->vision_box_6->setEnabled(false);
    }
}

void Dialog_Setup::on_match1Dtest1_combo_currentIndexChanged(const QString &selection)
{
    // active default: both shoulder & elbow, without vision
    if (strcmpi(selection.toStdString().c_str(),"active") == 0) {
        ui->vision_box_1->setEnabled(true);     ui->vision_box_1->setChecked(false);
        ui->shoulder_box_2->setEnabled(true);   ui->shoulder_box_2->setChecked(true);
        ui->elbow_box_2->setEnabled(true);      ui->elbow_box_2->setChecked(true);
    }

    // passive default: both shoulder & elbow, no choice about vision
    else if (strcmpi(selection.toStdString().c_str(),"passive") == 0) {
        ui->vision_box_1->setEnabled(false);    ui->vision_box_1->setChecked(false);
        ui->shoulder_box_2->setEnabled(true);   ui->shoulder_box_2->setChecked(true);
        ui->elbow_box_2->setEnabled(true);      ui->elbow_box_2->setChecked(true);
    }

    // disable and uncheck all options
    else {
        ui->vision_box_1->setEnabled(false);    ui->vision_box_1->setChecked(false);
        ui->shoulder_box_2->setEnabled(false);  ui->shoulder_box_2->setChecked(false);
        ui->elbow_box_2->setEnabled(false);     ui->elbow_box_2->setChecked(false);
    }
}

void Dialog_Setup::on_match1Dtest2_combo_currentIndexChanged(const QString &selection)
{
    // active default: both shoulder & elbow, without vision
    if (strcmpi(selection.toStdString().c_str(),"active") == 0) {
        ui->vision_box_2->setEnabled(true);     ui->vision_box_2->setChecked(false);
        ui->shoulder_box_3->setEnabled(true);   ui->shoulder_box_3->setChecked(true);
        ui->elbow_box_3->setEnabled(true);      ui->elbow_box_3->setChecked(true);
    }

    // passive default: both shoulder & elbow, no choice about vision
    else if (strcmpi(selection.toStdString().c_str(),"passive") == 0) {
        ui->vision_box_2->setEnabled(false);    ui->vision_box_2->setChecked(false);
        ui->shoulder_box_3->setEnabled(true);   ui->shoulder_box_3->setChecked(true);
        ui->elbow_box_3->setEnabled(true);      ui->elbow_box_3->setChecked(true);
    }

    // disable and uncheck all options
    else {
        ui->vision_box_2->setEnabled(false);    ui->vision_box_2->setChecked(false);
        ui->shoulder_box_3->setEnabled(false);  ui->shoulder_box_3->setChecked(false);
        ui->elbow_box_3->setEnabled(false);     ui->elbow_box_3->setChecked(false);
    }
}

void Dialog_Setup::on_match1Dtest3_combo_currentIndexChanged(const QString &selection)
{
    // active default: both shoulder & elbow, without vision
    if (strcmpi(selection.toStdString().c_str(),"active") == 0) {
        ui->vision_box_3->setEnabled(true);     ui->vision_box_3->setChecked(false);
        ui->shoulder_box_4->setEnabled(true);   ui->shoulder_box_4->setChecked(true);
        ui->elbow_box_4->setEnabled(true);      ui->elbow_box_4->setChecked(true);
    }

    // passive default: both shoulder & elbow, no choice about vision
    else if (strcmpi(selection.toStdString().c_str(),"passive") == 0) {
        ui->vision_box_3->setEnabled(false);    ui->vision_box_3->setChecked(false);
        ui->shoulder_box_4->setEnabled(true);   ui->shoulder_box_4->setChecked(true);
        ui->elbow_box_4->setEnabled(true);      ui->elbow_box_4->setChecked(true);
    }

    // disable and uncheck all options
    else {
        ui->vision_box_3->setEnabled(false);    ui->vision_box_3->setChecked(false);
        ui->shoulder_box_4->setEnabled(false);  ui->shoulder_box_4->setChecked(false);
        ui->elbow_box_4->setEnabled(false);     ui->elbow_box_4->setChecked(false);
    }
}

void Dialog_Setup::on_match2Dtest1_combo_currentIndexChanged(const QString &selection)
{
    // active default: without vision
    if (strcmpi(selection.toStdString().c_str(),"active") == 0) {
        ui->vision_box_4->setEnabled(true);
        ui->vision_box_4->setChecked(false);
    }

    // passive default: no choice about vision
    else if (strcmpi(selection.toStdString().c_str(),"passive") == 0) {
        ui->vision_box_4->setEnabled(false);
        ui->vision_box_4->setChecked(false);
    }

    // disable and uncheck all options
    else {
        ui->vision_box_4->setEnabled(false);
        ui->vision_box_4->setChecked(false);
    }
}

void Dialog_Setup::on_match2Dtest2_combo_currentIndexChanged(const QString &selection)
{
    // active default: without vision
    if (strcmpi(selection.toStdString().c_str(),"active") == 0) {
        ui->vision_box_5->setEnabled(true);
        ui->vision_box_5->setChecked(false);
    }

    // passive default: no choice about vision
    else if (strcmpi(selection.toStdString().c_str(),"passive") == 0) {
        ui->vision_box_5->setEnabled(false);
        ui->vision_box_5->setChecked(false);
    }

    // disable and uncheck all options
    else {
        ui->vision_box_5->setEnabled(false);
        ui->vision_box_5->setChecked(false);
    }
}

void Dialog_Setup::on_match2Dtest3_combo_currentIndexChanged(const QString &selection)
{
    // active default: without vision
    if (strcmpi(selection.toStdString().c_str(),"active") == 0) {
        ui->vision_box_6->setEnabled(true);
        ui->vision_box_6->setChecked(false);
    }

    // passive default: no choice about vision
    else if (strcmpi(selection.toStdString().c_str(),"passive") == 0) {
        ui->vision_box_6->setEnabled(false);
        ui->vision_box_6->setChecked(false);
    }

    // disable and uncheck all options
    else {
        ui->vision_box_6->setEnabled(false);
        ui->vision_box_6->setChecked(false);
    }
}

// NOTE: accept default subject parameters with no tests queued if "Cancel" pressed
void Dialog_Setup::on_buttonBox_accepted()
{
    // update subject parameters from sliders
    m_parent->m_exo->m_subj->updateHealth(bool(ui->health_slider->value()));
    m_parent->m_exo->m_subj->updateGender(bool(ui->gender_slider->value()));
    m_parent->m_exo->m_subj->updateHandedness(bool(ui->hand_slider->value()));

    // update subject parameters from fills (only if filled)
    if (!ui->name_fill->text().isEmpty()) {
       std::string newName = ui->name_fill->text().toStdString();
       m_parent->m_exo->m_subj->updateName(newName);
    }
    if (!ui->ID_fill->text().isEmpty()) {
       std::string newID = ui->ID_fill->text().toStdString();
       m_parent->m_exo->m_subj->updateID(newID);
    }
    if (!ui->age_fill->text().isEmpty()) {
       int newAge = ui->age_fill->text().toInt();
       m_parent->m_exo->m_subj->updateAge(newAge);
    }
    if (!ui->upperarmL_fill->text().isEmpty()) {
       double newLupper = ui->upperarmL_fill->text().toDouble();
       m_parent->m_exo->m_subj->updateUpperarm(newLupper);
    }
    if (!ui->forearmL_fill->text().isEmpty()) {
       double newLlower = ui->forearmL_fill->text().toDouble();
       m_parent->m_exo->m_subj->updateForearm(newLlower);
    }
    if (!ui->LtoEE_fill->text().isEmpty()) {
       double newLtoEE = ui->LtoEE_fill->text().toDouble();
       m_parent->m_exo->m_subj->updateLtoEE(newLtoEE);
    }

    // populate queue of test parameters from combo/check-box selections
    // NOTE: this accounts for the subject's unique joint test order
    test_params testCurr;
    // staircases?
    if (ui->staircase_box->isChecked()) {
        testCurr.p_type = staircase;
        testCurr.p_active = false;
        testCurr.p_vision = false;
        testCurr.p_time = T_1D_2AFC;
        if (m_parent->m_exo->m_subj->m_params.p_firstJnt == S) {
            if (ui->shoulder_box_1->isChecked()) {
                testCurr.p_joint = S;
                testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                m_parent->m_exp->m_testQueue.push(testCurr);
            }
            if (ui->elbow_box_1->isChecked()) {
                testCurr.p_joint = E;
                testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                m_parent->m_exp->m_testQueue.push(testCurr);
            }
        } else {
            if (ui->elbow_box_1->isChecked()) {
                testCurr.p_joint = E;
                testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                m_parent->m_exp->m_testQueue.push(testCurr);
            }
            if (ui->shoulder_box_1->isChecked()) {
                testCurr.p_joint = S;
                testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                m_parent->m_exp->m_testQueue.push(testCurr);
            }
        }
    }
    // 1-D matching?
    if (ui->match1D_box->isChecked()) {
        testCurr.p_type = match1D;
        // combobox #1
        if (strcmpi(ui->match1Dtest1_combo->currentText().toStdString().c_str(),"active") == 0) {
            testCurr.p_active = true;
            testCurr.p_vision = ui->vision_box_1->isChecked();
            testCurr.p_time = T_1D_ACTV;
            if (m_parent->m_exo->m_subj->m_params.p_firstJnt == S) {
                if (ui->shoulder_box_2->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->elbow_box_2->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            } else {
                if (ui->elbow_box_2->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->shoulder_box_2->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            }
        } else if (strcmpi(ui->match1Dtest1_combo->currentText().toStdString().c_str(),"passive") == 0) {
            testCurr.p_active = false;
            testCurr.p_vision = false;
            testCurr.p_time = T_1D_PASS;
            if (m_parent->m_exo->m_subj->m_params.p_firstJnt == S) {
                if (ui->shoulder_box_2->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->elbow_box_2->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            } else {
                if (ui->elbow_box_2->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->shoulder_box_2->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            }
        }
        // combobox #2
        if (strcmpi(ui->match1Dtest2_combo->currentText().toStdString().c_str(),"active") == 0) {
            testCurr.p_active = true;
            testCurr.p_vision = ui->vision_box_2->isChecked();
            testCurr.p_time = T_1D_ACTV;
            if (m_parent->m_exo->m_subj->m_params.p_firstJnt == S) {
                if (ui->shoulder_box_3->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->elbow_box_3->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            } else {
                if (ui->elbow_box_3->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->shoulder_box_3->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            }
        } else if (strcmpi(ui->match1Dtest2_combo->currentText().toStdString().c_str(),"passive") == 0) {
            testCurr.p_active = false;
            testCurr.p_vision = false;
            testCurr.p_time = T_1D_PASS;
            if (m_parent->m_exo->m_subj->m_params.p_firstJnt == S) {
                if (ui->shoulder_box_3->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->elbow_box_3->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            } else {
                if (ui->elbow_box_3->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->shoulder_box_3->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            }
        }
        // combobox #3
        if (strcmpi(ui->match1Dtest3_combo->currentText().toStdString().c_str(),"active") == 0) {
            testCurr.p_active = true;
            testCurr.p_vision = ui->vision_box_3->isChecked();
            testCurr.p_time = T_1D_ACTV;
            if (m_parent->m_exo->m_subj->m_params.p_firstJnt == S) {
                if (ui->shoulder_box_4->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->elbow_box_4->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            } else {
                if (ui->elbow_box_4->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->shoulder_box_4->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            }
        } else if (strcmpi(ui->match1Dtest3_combo->currentText().toStdString().c_str(),"passive") == 0) {
            testCurr.p_active = false;
            testCurr.p_vision = false;
            testCurr.p_time = T_1D_PASS;
            if (m_parent->m_exo->m_subj->m_params.p_firstJnt == S) {
                if (ui->shoulder_box_4->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->elbow_box_4->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            } else {
                if (ui->elbow_box_4->isChecked()) {
                    testCurr.p_joint = E;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[S];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
                if (ui->shoulder_box_4->isChecked()) {
                    testCurr.p_joint = S;
                    testCurr.p_lock = m_parent->m_exo->m_subj->m_params.p_lockAngs[E];
                    m_parent->m_exp->m_testQueue.push(testCurr);
                }
            }
        }
    }
    // 2-D matching?
    if (ui->match2D_box->isChecked()) {
        testCurr.p_type = match2D;
        testCurr.p_joint = both;
        // combobox #1
        if (strcmpi(ui->match2Dtest1_combo->currentText().toStdString().c_str(),"active") == 0) {
            testCurr.p_active = true;
            testCurr.p_vision = ui->vision_box_4->isChecked();
            testCurr.p_time = T_2D_ACTV;
            m_parent->m_exp->m_testQueue.push(testCurr);
        } else if (strcmpi(ui->match2Dtest1_combo->currentText().toStdString().c_str(),"passive") == 0) {
            testCurr.p_active = false;
            testCurr.p_vision = false;
            testCurr.p_time = T_2D_PASS;
            m_parent->m_exp->m_testQueue.push(testCurr);
        }
        // combobox #2
        if (strcmpi(ui->match2Dtest2_combo->currentText().toStdString().c_str(),"active") == 0) {
            testCurr.p_active = true;
            testCurr.p_vision = ui->vision_box_5->isChecked();
            testCurr.p_time = T_2D_ACTV;
            m_parent->m_exp->m_testQueue.push(testCurr);
        } else if (strcmpi(ui->match2Dtest2_combo->currentText().toStdString().c_str(),"passive") == 0) {
            testCurr.p_active = false;
            testCurr.p_vision = false;
            testCurr.p_time = T_2D_PASS;
            m_parent->m_exp->m_testQueue.push(testCurr);
        }
        // combobox #3
        if (strcmpi(ui->match2Dtest3_combo->currentText().toStdString().c_str(),"active") == 0) {
            testCurr.p_active = true;
            testCurr.p_vision = ui->vision_box_6->isChecked();
            testCurr.p_time = T_2D_ACTV;
            m_parent->m_exp->m_testQueue.push(testCurr);
        } else if (strcmpi(ui->match2Dtest3_combo->currentText().toStdString().c_str(),"passive") == 0) {
            testCurr.p_active = false;
            testCurr.p_vision = false;
            testCurr.p_time = T_2D_PASS;
            m_parent->m_exp->m_testQueue.push(testCurr);
        }
    }

    // sync changes with and show main window GUI
    m_parent->syncSubjectDisplay();
    m_parent->initialize();
    m_parent->show();

    // perform first calibration
    m_parent->m_exo->calibrate();

}

void Dialog_Setup::on_buttonBox_rejected()
{
    m_parent->syncSubjectDisplay();
    m_parent->initialize();
    m_parent->show();
    m_parent->m_exo->calibrate();
}
