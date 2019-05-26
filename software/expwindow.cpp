#include "expwindow.h"
#include "ui_expwindow.h"

ExpWindow::ExpWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::ExpWindow)
{
    // does not yet know about main window
    m_parent = NULL;

    // set up GUI and embedded CHAI widget
    ui->setupUi(this);
    if (!ui->display) {
        QMessageBox::information(this, "Application", "Cannot start application.", QMessageBox::Ok);
        close();
    } else {
        ui->display->m_parent = this;
    }

    // define keyboard shortcuts
    QKey = new QShortcut(Qt::Key_Q, this, SLOT(close()));
    FKey = new QShortcut(Qt::Key_F, this, SLOT(showFullScreen()));
}

ExpWindow::~ExpWindow()
{
    ui->display->stop();
    delete ui;
}

void ExpWindow::closeEvent(QCloseEvent *event)
{
    stopExp();
    event->accept();
}

void ExpWindow::pairWithMainWindow(MainWindow *a_parent)
{
    m_parent = a_parent;
}

void ExpWindow::startExp()
{
    ui->display->start();
}

void ExpWindow::stopExp()
{
    ui->display->stop();
    m_parent->onExperimentStop();
}

exp_snapshot ExpWindow::getSnapshot()
{
    return ui->display->m_snap;
}
