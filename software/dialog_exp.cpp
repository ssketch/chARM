#include "dialog_exp.h"
#include "ui_dialog_exp.h"

#define T_1D_2AFC   5   // time allotted for each 2AFC judgement
#define T_1D_ACTV   10  // time allotted for each 1-D active matching trial
#define T_1D_PASS   15  // time allotted for each 1-D passive matched trial
#define T_2D_ACTV   10  // time allotted for each 2-D active matching trial
#define T_2D_PASS   25  // time allotted for each 2-D passive matching trial

Dialog_Exp::Dialog_Exp(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_Exp)
{
    // does not yet know about main window
    m_parent = NULL;

    ui->setupUi(this);

    // disable select elements
    ui->shoulder_box_1->setEnabled(false);  ui->elbow_box_1->setEnabled(false);
    ui->shoulder_box_2->setEnabled(false);  ui->elbow_box_2->setEnabled(false);
    ui->shoulder_box_3->setEnabled(false);  ui->elbow_box_3->setEnabled(false);
    ui->shoulder_box_4->setEnabled(false);  ui->elbow_box_4->setEnabled(false);

    ui->vision_box_1->setEnabled(false);
    ui->vision_box_2->setEnabled(false);
    ui->vision_box_3->setEnabled(false);
    ui->vision_box_4->setEnabled(false);
    ui->vision_box_5->setEnabled(false);
    ui->vision_box_6->setEnabled(false);

    ui->match1Dtest1_combo->setEnabled(false);
    ui->match1Dtest2_combo->setEnabled(false);
    ui->match1Dtest3_combo->setEnabled(false);
    ui->match2Dtest1_combo->setEnabled(false);
    ui->match2Dtest2_combo->setEnabled(false);
    ui->match2Dtest3_combo->setEnabled(false);
}

Dialog_Exp::~Dialog_Exp()
{
    delete ui;
}

void Dialog_Exp::pairWithMainWindow(MainWindow *a_parent)
{
    m_parent = a_parent;
}

void Dialog_Exp::reset()
{
    // clear selections
    ui->staircase_box->setChecked(false);
    ui->match1D_box->setChecked(false);
    ui->match2D_box->setChecked(false);

    ui->shoulder_box_1->setChecked(false);  ui->elbow_box_1->setChecked(false);
    ui->shoulder_box_2->setChecked(false);  ui->elbow_box_2->setChecked(false);
    ui->shoulder_box_3->setChecked(false);  ui->elbow_box_3->setChecked(false);
    ui->shoulder_box_4->setChecked(false);  ui->elbow_box_4->setChecked(false);

    ui->vision_box_1->setChecked(false);
    ui->vision_box_2->setChecked(false);
    ui->vision_box_3->setChecked(false);
    ui->vision_box_4->setChecked(false);
    ui->vision_box_5->setChecked(false);
    ui->vision_box_6->setChecked(false);

    ui->match1Dtest1_combo->setCurrentIndex(-1);
    ui->match1Dtest2_combo->setCurrentIndex(-1);
    ui->match1Dtest3_combo->setCurrentIndex(-1);
    ui->match2Dtest1_combo->setCurrentIndex(-1);
    ui->match2Dtest2_combo->setCurrentIndex(-1);
    ui->match2Dtest3_combo->setCurrentIndex(-1);

    // disable select elements
    ui->shoulder_box_1->setEnabled(false);  ui->elbow_box_1->setEnabled(false);
    ui->shoulder_box_2->setEnabled(false);  ui->elbow_box_2->setEnabled(false);
    ui->shoulder_box_3->setEnabled(false);  ui->elbow_box_3->setEnabled(false);
    ui->shoulder_box_4->setEnabled(false);  ui->elbow_box_4->setEnabled(false);

    ui->vision_box_1->setEnabled(false);
    ui->vision_box_2->setEnabled(false);
    ui->vision_box_3->setEnabled(false);
    ui->vision_box_4->setEnabled(false);
    ui->vision_box_5->setEnabled(false);
    ui->vision_box_6->setEnabled(false);

    ui->match1Dtest1_combo->setEnabled(false);
    ui->match1Dtest2_combo->setEnabled(false);
    ui->match1Dtest3_combo->setEnabled(false);
    ui->match2Dtest1_combo->setEnabled(false);
    ui->match2Dtest2_combo->setEnabled(false);
    ui->match2Dtest3_combo->setEnabled(false);
}

void Dialog_Exp::on_staircase_box_stateChanged(int newState)
{
    if (newState == Qt::Checked) {  // both shoulder & elbow by default
        ui->shoulder_box_1->setEnabled(true);  ui->elbow_box_1->setEnabled(true);
        ui->shoulder_box_1->setChecked(true);  ui->elbow_box_1->setChecked(true);
    } else {
        ui->shoulder_box_1->setEnabled(false);  ui->elbow_box_1->setEnabled(false);
        ui->shoulder_box_1->setChecked(false);  ui->elbow_box_1->setChecked(false);
    }
}

void Dialog_Exp::on_match1D_box_stateChanged(int newState)
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

void Dialog_Exp::on_match2D_box_stateChanged(int newState)
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

void Dialog_Exp::on_match1Dtest1_combo_currentIndexChanged(const QString &selection)
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

void Dialog_Exp::on_match1Dtest2_combo_currentIndexChanged(const QString &selection)
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

void Dialog_Exp::on_match1Dtest3_combo_currentIndexChanged(const QString &selection)
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

void Dialog_Exp::on_match2Dtest1_combo_currentIndexChanged(const QString &selection)
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

void Dialog_Exp::on_match2Dtest2_combo_currentIndexChanged(const QString &selection)
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

void Dialog_Exp::on_match2Dtest3_combo_currentIndexChanged(const QString &selection)
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

void Dialog_Exp::on_buttonBox_accepted()
{
    // populate queue of test parameters from combo/check-box selections
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

    this->hide();
}

void Dialog_Exp::on_buttonBox_rejected()
{
    this->hide();
}
