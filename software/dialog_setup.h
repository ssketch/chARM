#ifndef DIALOG_SETUP_H
#define DIALOG_SETUP_H

#include "mainwindow.h"
#include <queue>
#include <QDialog>

namespace Ui {
class Dialog_Setup;
}

class Dialog_Setup : public QDialog
{
    Q_OBJECT

public:
    MainWindow* m_parent;  // pointer to main window

    explicit Dialog_Setup(QWidget *parent = 0);
    ~Dialog_Setup();

    void pairWithMainWindow(MainWindow *a_parent);

private slots:
    void on_staircase_box_stateChanged(int newState);
    void on_match1D_box_stateChanged(int newState);
    void on_match2D_box_stateChanged(int newState);
    void on_match1Dtest1_combo_currentIndexChanged(const QString &selection);
    void on_match1Dtest2_combo_currentIndexChanged(const QString &selection);
    void on_match1Dtest3_combo_currentIndexChanged(const QString &selection);
    void on_match2Dtest1_combo_currentIndexChanged(const QString &selection);
    void on_match2Dtest2_combo_currentIndexChanged(const QString &selection);
    void on_match2Dtest3_combo_currentIndexChanged(const QString &selection);
    void on_buttonBox_accepted();  // "OK"
    void on_buttonBox_rejected();  // "Cancel"

private:
    Ui::Dialog_Setup* ui;  // pointer to user interface
};

#endif // DIALOG_SETUP_H
