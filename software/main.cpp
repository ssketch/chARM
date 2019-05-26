#include "subject.h"
#include "exo.h"
#include "mainwindow.h"
#include "expwindow.h"
#include "dialog_setup.h"
#include "dialog_gaintuning.h"
#include <QApplication>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // create default subject and associated exoskeleton
    subject* subj = new subject();
    exo* chARM = new exo(subj);

    // create & initialize main console window
    MainWindow console;
    console.pairWithExo(chARM);
    console.show();

    // create dialog window for subject parameters
    Dialog_Setup setup;
    setup.pairWithMainWindow(&console);
    setup.show();

    // create dialog window for adding to experiment
    Dialog_Exp addExp;
    addExp.pairWithMainWindow(&console);
    console.pairWithExpDialog(&addExp);
    addExp.show();

    // create dialog window for gain tuning
    Dialog_GainTuning tuner;
    tuner.pairWithMainWindow(&console);
    console.pairWithGainTuner(&tuner);
    tuner.show();

    // create experiment window & pair (2-way) with console
    ExpWindow experiment;
    experiment.pairWithMainWindow(&console);
    console.pairWithExp(&experiment);
    experiment.show();

    // hide all windows except entry for subject parameters
    console.hide();
    addExp.hide();
    tuner.hide();
    experiment.hide();

    return app.exec();
}
