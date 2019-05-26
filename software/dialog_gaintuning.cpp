#include "dialog_gaintuning.h"
#include "ui_dialog_gaintuning.h"

#define Kp_SCALAR 100
#define Kd_SCALAR 500
#define Ki_SCALAR 10000
#define A_SCALAR  100

Dialog_GainTuning::Dialog_GainTuning(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog_GainTuning)
{
    // does not yet know about main window
    m_parent = NULL;

    ui->setupUi(this);
}

Dialog_GainTuning::~Dialog_GainTuning()
{
    delete ui;
}

void Dialog_GainTuning::pairWithMainWindow(MainWindow *a_parent)
{
    m_parent = a_parent;
}

void Dialog_GainTuning::syncGains()
{
    ui->KpS_dial->setValue(floor(m_parent->m_exo->m_KpJnt(0)*Kp_SCALAR));
    ui->KdS_dial->setValue(floor(m_parent->m_exo->m_KdJnt(0)*Kd_SCALAR));
    ui->KiS_dial->setValue(floor(m_parent->m_exo->m_KiJnt(0)*Ki_SCALAR));
    ui->KpE_dial->setValue(floor(m_parent->m_exo->m_KpJnt(1)*Kp_SCALAR));
    ui->KdE_dial->setValue(floor(m_parent->m_exo->m_KdJnt(1)*Kd_SCALAR));
    ui->KiE_dial->setValue(floor(m_parent->m_exo->m_KiJnt(1)*Ki_SCALAR));
    ui->KpX_dial->setValue(floor(m_parent->m_exo->m_KpTsk(0)*Kp_SCALAR));
    ui->KdX_dial->setValue(floor(m_parent->m_exo->m_KdTsk(0)*Kd_SCALAR));
    ui->KiX_dial->setValue(floor(m_parent->m_exo->m_KiTsk(0)*Ki_SCALAR));
    ui->KpY_dial->setValue(floor(m_parent->m_exo->m_KpTsk(1)*Kp_SCALAR));
    ui->KdY_dial->setValue(floor(m_parent->m_exo->m_KdTsk(1)*Kd_SCALAR));
    ui->KiY_dial->setValue(floor(m_parent->m_exo->m_KiTsk(1)*Ki_SCALAR));
    ui->KdNegS_dial->setValue(floor(m_parent->m_exo->m_KdNeg(0)*Kd_SCALAR));
    ui->KdNegE_dial->setValue(floor(m_parent->m_exo->m_KdNeg(1)*Kd_SCALAR));
    ui->ampS_slider->setValue(floor(m_parent->m_exo->m_Adith(0)*A_SCALAR));
    ui->ampE_slider->setValue(floor(m_parent->m_exo->m_Adith(1)*A_SCALAR));
    ui->freqS_slider->setValue(m_parent->m_exo->m_fdith(0));
    ui->freqE_slider->setValue(m_parent->m_exo->m_fdith(1));

    ui->KpS_lcd->display(m_parent->m_exo->m_KpJnt(0));
    ui->KdS_lcd->display(m_parent->m_exo->m_KdJnt(0));
    ui->KiS_lcd->display(m_parent->m_exo->m_KiJnt(0));
    ui->KpE_lcd->display(m_parent->m_exo->m_KpJnt(1));
    ui->KdE_lcd->display(m_parent->m_exo->m_KdJnt(1));
    ui->KiE_lcd->display(m_parent->m_exo->m_KiJnt(1));
    ui->KpX_lcd->display(m_parent->m_exo->m_KpTsk(0));
    ui->KdX_lcd->display(m_parent->m_exo->m_KdTsk(0));
    ui->KiX_lcd->display(m_parent->m_exo->m_KiTsk(0));
    ui->KpY_lcd->display(m_parent->m_exo->m_KpTsk(1));
    ui->KdY_lcd->display(m_parent->m_exo->m_KdTsk(1));
    ui->KiY_lcd->display(m_parent->m_exo->m_KiTsk(1));
    ui->KdNegS_lcd->display(m_parent->m_exo->m_KdNeg(0));
    ui->KdNegE_lcd->display(m_parent->m_exo->m_KdNeg(1));
    ui->ampS_lcd->display(m_parent->m_exo->m_Adith(0));
    ui->ampE_lcd->display(m_parent->m_exo->m_Adith(1));
    ui->freqS_lcd->display(m_parent->m_exo->m_fdith(0));
    ui->freqE_lcd->display(m_parent->m_exo->m_fdith(1));
}

void Dialog_GainTuning::on_KpS_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KpJnt(0) = double(value)/Kp_SCALAR;
    ui->KpS_lcd->display(m_parent->m_exo->m_KpJnt(0));
}

void Dialog_GainTuning::on_KdS_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KdJnt(0) = double(value)/Kd_SCALAR;
    ui->KdS_lcd->display(m_parent->m_exo->m_KdJnt(0));
}

void Dialog_GainTuning::on_KiS_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KiJnt(0) = double(value)/Ki_SCALAR;
    ui->KiS_lcd->display(m_parent->m_exo->m_KiJnt(0));
}

void Dialog_GainTuning::on_KpE_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KpJnt(1) = double(value)/Kp_SCALAR;
    ui->KpE_lcd->display(m_parent->m_exo->m_KpJnt(1));
}

void Dialog_GainTuning::on_KdE_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KdJnt(1) = double(value)/Kd_SCALAR;
    ui->KdE_lcd->display(m_parent->m_exo->m_KdJnt(1));
}

void Dialog_GainTuning::on_KiE_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KiJnt(1) = double(value)/Ki_SCALAR;
    ui->KiE_lcd->display(m_parent->m_exo->m_KiJnt(1));
}

void Dialog_GainTuning::on_KpX_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KpTsk(0) = double(value)/Kp_SCALAR;
    ui->KpX_lcd->display(m_parent->m_exo->m_KpTsk(0));
}

void Dialog_GainTuning::on_KdX_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KdTsk(0) = double(value)/Kd_SCALAR;
    ui->KdX_lcd->display(m_parent->m_exo->m_KdTsk(0));
}

void Dialog_GainTuning::on_KiX_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KiTsk(0) = double(value)/Ki_SCALAR;
    ui->KiX_lcd->display(m_parent->m_exo->m_KiTsk(0));
}

void Dialog_GainTuning::on_KpY_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KpTsk(1) = double(value)/Kp_SCALAR;
    ui->KpY_lcd->display(m_parent->m_exo->m_KpTsk(1));
}

void Dialog_GainTuning::on_KdY_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KdTsk(1) = double(value)/Kd_SCALAR;
    ui->KdY_lcd->display(m_parent->m_exo->m_KdTsk(1));
}

void Dialog_GainTuning::on_KiY_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KiTsk(1) = double(value)/Ki_SCALAR;
    ui->KiY_lcd->display(m_parent->m_exo->m_KiTsk(1));
}

void Dialog_GainTuning::on_KdNegS_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KdNeg(0) = double(value)/Kd_SCALAR;
    ui->KdNegS_lcd->display(m_parent->m_exo->m_KdNeg(0));
}

void Dialog_GainTuning::on_KdNegE_dial_valueChanged(int value)
{
    m_parent->m_exo->m_KdNeg(1) = double(value)/Kd_SCALAR;
    ui->KdNegE_lcd->display(m_parent->m_exo->m_KdNeg(1));
}

void Dialog_GainTuning::on_ampS_slider_valueChanged(int value)
{
    m_parent->m_exo->m_Adith(0) = double(value)/A_SCALAR;
    ui->ampS_lcd->display(m_parent->m_exo->m_Adith(0));
}

void Dialog_GainTuning::on_ampE_slider_valueChanged(int value)
{
    m_parent->m_exo->m_Adith(1) = double(value)/A_SCALAR;
    ui->ampE_lcd->display(m_parent->m_exo->m_Adith(1));
}

void Dialog_GainTuning::on_freqS_slider_valueChanged(int value)
{
    m_parent->m_exo->m_fdith(0) = double(value);
    ui->freqS_lcd->display(m_parent->m_exo->m_fdith(0));
}

void Dialog_GainTuning::on_freqE_slider_valueChanged(int value)
{
    m_parent->m_exo->m_fdith(1) = double(value);
    ui->freqE_lcd->display(m_parent->m_exo->m_fdith(1));
}

void Dialog_GainTuning::on_complete_push_clicked()
{
    this->hide();
}
