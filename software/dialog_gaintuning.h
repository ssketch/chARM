#ifndef DIALOG_GAINTUNING_H
#define DIALOG_GAINTUNING_H

#include "mainwindow.h"
#include <QDialog>

class MainWindow;

namespace Ui {
class Dialog_GainTuning;
}

class Dialog_GainTuning : public QDialog
{
    Q_OBJECT

public:
    MainWindow* m_parent;  // pointer to main window

    explicit Dialog_GainTuning(QWidget *parent = 0);
    ~Dialog_GainTuning();

    void pairWithMainWindow(MainWindow *a_parent);
    void syncGains();

private slots:
    void on_KpS_dial_valueChanged(int value);
    void on_KdS_dial_valueChanged(int value);
    void on_KiS_dial_valueChanged(int value);
    void on_KpE_dial_valueChanged(int value);
    void on_KdE_dial_valueChanged(int value);
    void on_KiE_dial_valueChanged(int value);
    void on_KpX_dial_valueChanged(int value);
    void on_KdX_dial_valueChanged(int value);
    void on_KiX_dial_valueChanged(int value);
    void on_KpY_dial_valueChanged(int value);
    void on_KdY_dial_valueChanged(int value);
    void on_KiY_dial_valueChanged(int value);
    void on_KdNegS_dial_valueChanged(int value);
    void on_KdNegE_dial_valueChanged(int value);
    void on_ampS_slider_valueChanged(int value);
    void on_ampE_slider_valueChanged(int value);
    void on_freqS_slider_valueChanged(int value);
    void on_freqE_slider_valueChanged(int value);
    void on_complete_push_clicked();

private:
    Ui::Dialog_GainTuning *ui;
};

#endif // DIALOG_GAINTUNING_H
