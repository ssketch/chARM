#ifndef DIALOG_EXP_H
#define DIALOG_EXP_H

#include "mainwindow.h"
#include <queue>
#include <QDialog>

class MainWindow;

namespace Ui {
class Dialog_Exp;
}

class Dialog_Exp : public QDialog
{
    Q_OBJECT

public:
    MainWindow* m_parent;  // pointer to main window

    explicit Dialog_Exp(QWidget *parent = 0);
    ~Dialog_Exp();

    void pairWithMainWindow(MainWindow *a_parent);
    void reset();

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
    Ui::Dialog_Exp *ui;  // pointer to user interface
};

#endif // DIALOG_EXP_H
