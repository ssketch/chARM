#include "charmwidget.h"
#include <QDebug>

#define T_GRAPHICS 50        // update every 50 ms (20 Hz)
#define R_JOINT    0.04      // radius of spheres representing joints [m]
#define R_SEGMENT  0.02      // radius of cylinders representing upper/forearm [m]
#define H_SEGMENT  0.5       // default height of cylinders representing upper/forearm [m]
#define ALPH_GHOST 0.2       // alpha value for "ghost" exoskeleton components
#define FRAMES     0         // 0 = hide joint frames, 1 = show joint frames (for ghost)
#define JNTSPACE   0         // joint-space control
#define TASKSPACE  1         // task-space control
#define PI         3.141592
#define DEBUG      0

using namespace std;
using namespace chai3d;

void _hapticThread(void *arg)
{
    ((chARMWidget*)arg)->hapticThread();
}

chARMWidget::chARMWidget(QWidget *parent) :
    QGLWidget(parent)
{
    // initialize variables for graphic/haptic rendering
    m_timer = new QBasicTimer;
    m_running = false;

    // reset frequency counters
    m_graphicRate.reset();
    m_hapticRate.reset();

    // create new CHAI world
    m_world = new cWorld();
    m_world->m_backgroundColor.setBlack();

    // create camera to view world
    // NOTE: In orthographic (non-perspective) mode, 'camera->set()' functions slightly differently.
    // ----  The scale of objects in the scene is governed by the width of the orthographic viewport,
    //       which is set via 'camera->setOrthographicView(__m)', instead of the distance of the camera
    //       from the scene.
    m_camera = new cCamera(m_world);
    m_camera->set(cVector3d(0.0,0.3,1.0),   // camera position
                  cVector3d(0.0,0.3,0.0),   // look-at vector = straight down
                  cVector3d(0.0,1.0,0.0));  // vector pointing to top of FOV
    m_camera->setClippingPlanes(0.01,20.0);
    m_camera->setOrthographicView(1.0);
    m_world->addChild(m_camera);

    // attach light source to camera for illumination
    m_light = new cDirectionalLight(m_world);
    m_light->setEnabled(true);
    m_camera->addChild(m_light);

    // add "ghost" exoskeletons (joints = spheres, segments = cylinders)
    double scalar = 0.8;
    m_shoulderT = new cShapeSphere(R_JOINT*scalar);
    m_elbowT = new cShapeSphere(R_JOINT*scalar);
    m_handT = new cShapeSphere(R_JOINT*scalar);
    m_shoulderT->m_material->setWhite();
    m_elbowT->m_material->setWhite();
    m_handT->m_material->setWhite();
    m_world->addChild(m_shoulderT);
    m_world->addChild(m_elbowT);
    m_world->addChild(m_handT);
    m_upperarmT = new cShapeCylinder(R_SEGMENT*scalar, R_SEGMENT*scalar, H_SEGMENT);
    m_forearmT = new cShapeCylinder(R_SEGMENT*scalar, R_SEGMENT*scalar, H_SEGMENT);
    m_upperarmT->m_material->setWhite();
    m_forearmT->m_material->setWhite();
    m_upperarmT->setTransparencyLevel((float)ALPH_GHOST);
    m_forearmT->setTransparencyLevel((float)ALPH_GHOST);
    m_world->addChild(m_upperarmT);
    m_world->addChild(m_forearmT);

    m_shoulderG = new cShapeSphere(R_JOINT);
    m_elbowG = new cShapeSphere(R_JOINT);
    m_handG = new cShapeSphere(R_JOINT);
    m_shoulderG->m_material->setBlueDodger();
    m_elbowG->m_material->setGreenLime();
    m_handG->m_material->setRed();
    if (FRAMES) {
        m_shoulderG->setShowFrame(true);
        m_elbowG->setShowFrame(true);
        m_handG->setShowFrame(true);
    }
    m_world->addChild(m_shoulderG);
    m_world->addChild(m_elbowG);
    m_world->addChild(m_handG);
    m_upperarmG = new cShapeCylinder(R_SEGMENT, R_SEGMENT, H_SEGMENT);
    m_forearmG = new cShapeCylinder(R_SEGMENT, R_SEGMENT, H_SEGMENT);
    m_upperarmG->m_material->setWhite();
    m_forearmG->m_material->setWhite();
    m_upperarmG->setTransparencyLevel((float)ALPH_GHOST);
    m_forearmG->setTransparencyLevel((float)ALPH_GHOST);
    m_world->addChild(m_upperarmG);
    m_world->addChild(m_forearmG);

    // add exoskeleton
    m_shoulder = new cShapeSphere(R_JOINT);
    m_elbow = new cShapeSphere(R_JOINT);
    m_hand = new cShapeSphere(R_JOINT);
    m_shoulder->m_material->setBlueDodger();
    m_elbow->m_material->setGreenLime();
    m_hand->m_material->setRed();
    m_world->addChild(m_shoulder);
    m_world->addChild(m_elbow);
    m_world->addChild(m_hand);
    m_upperarm = new cShapeCylinder(R_SEGMENT, R_SEGMENT, H_SEGMENT);
    m_forearm = new cShapeCylinder(R_SEGMENT, R_SEGMENT, H_SEGMENT);
    m_upperarm->m_material->setWhite();
    m_forearm->m_material->setWhite();
    m_world->addChild(m_upperarm);
    m_world->addChild(m_forearm);

    // add labels to background
    cFont* font = NEW_CFONTCALIBRI40();
    m_OOB = new cLabel(font);
    m_OOB->m_fontColor.setRed();
    m_camera->m_backLayer->addChild(m_OOB);
    m_OOB->setText("OUT OF BOUNDS");
}

chARMWidget::~chARMWidget()
{
    delete m_timer;
    delete m_world;
}

bool chARMWidget::start()
{
    // start graphic rendering at specified frequency
    m_timer->start(T_GRAPHICS, this);

    // connect to exo & start haptic thread running
    // NOTE: this should be the only haptic thread running
    if (m_parent->m_exo->connect()) {
        m_thread.start(_hapticThread, CTHREAD_PRIORITY_HAPTICS, this);
    } else
        return(C_ERROR);

    return(C_SUCCESS);
}

void chARMWidget::stop()
{
    // stop the haptic thread
    m_running = false;

    // disable exoskeleton control once haptics thread is done with it
    m_runLock.acquire();
    m_parent->m_exo->setCtrl(none);
    m_runLock.release();

    // stop graphic rendering
    m_timer->stop();
}

void* chARMWidget::hapticThread()
{
    // acquire mutex and don't release until thread is stopped (i.e., m_running == FALSE)
    m_runLock.acquire();
    m_running = true;

    while (m_running)
    {
        // update exoskeleton configuration
        m_parent->m_exo->getState();
        drawExo();

        // command exoskeleton via designated control paradigm
        bool inWorkspace = m_parent->m_exo->sendCommand();
        if (!inWorkspace) {
            m_OOB->setLocalPos(10,(int)(m_height-m_OOB->getHeight()),0);
            m_OOB->setShowEnabled(true);
        } else
            m_OOB->setShowEnabled(false);

        // update haptics counter
        m_hapticRate.signal(1);
    }

    m_running = false;
    m_runLock.release();
    return(NULL);
}

void chARMWidget::initializeGL()
{
#ifdef GLEW_VERSION
    glewInit();
#endif

    // enable anti-aliasing
    QGLWidget::setFormat(QGLFormat(QGL::SampleBuffers));
}

void chARMWidget::paintGL()
{
    if (!m_running) return;

    m_worldLock.acquire();

    // render world
    m_camera->renderView(m_width, m_height);
    glFinish();

    // update graphics counter
    m_graphicRate.signal(1);

    m_worldLock.release();
}

void chARMWidget::drawExo()
{
    // extract subject parameters
    cVector3d origin = cVector3d(0.0,0.0,0.0);
    double L1 = m_parent->m_exo->m_subj->m_Lupper;
    double L2 = m_parent->m_exo->m_subj->m_LtoEE;
    double th1 = m_parent->m_exo->m_th(0);
    double th2 = th1 + m_parent->m_exo->m_th(1);
    double th1Des = m_parent->m_exo->m_thDes(0);
    double th2Des = th1Des + m_parent->m_exo->m_thDes(1);
    double th1Trg = m_parent->m_exo->m_thTarg(0);
    double th2Trg = th1Trg + m_parent->m_exo->m_thTarg(1);

    // adjust for handedness
    if (!m_parent->m_exo->m_subj->m_rightHanded) {
        th1 = PI - th1;     th1Des = PI - th1Des;   th1Trg = PI - th1Trg;
        th2 = PI - th2;     th2Des = PI - th2Des;   th2Trg = PI - th2Trg;
    }

    // set lengths for exo segments
    m_upperarm->setHeight(L1);
    m_forearm->setHeight(L2);
    m_upperarmG->setHeight(L1);
    m_forearmG->setHeight(L2);
    m_upperarmT->setHeight(L1);
    m_forearmT->setHeight(L2);

    // actual exoskeleton
    m_shoulder->setLocalPos(origin);  // origin
    m_elbow->setLocalPos(m_shoulder->getLocalPos() + cVector3d(L1*cos(th1), L1*sin(th1), 0.0));
    m_hand->setLocalPos(m_elbow->getLocalPos() + cVector3d(L2*cos(th2), L2*sin(th2), 0.0));
    m_upperarm->setLocalPos(m_shoulder->getLocalPos());
    m_forearm->setLocalPos(m_elbow->getLocalPos());
    m_upperarm->setLocalRot(cMatrix3d(th1, PI/2, 0.0, C_EULER_ORDER_ZYX));
    m_forearm->setLocalRot(cMatrix3d(th2, PI/2, 0.0, C_EULER_ORDER_ZYX));

    // "ghost" desired-state exoskeleton
    m_shoulderG->setLocalPos(origin);
    m_elbowG->setLocalPos(m_shoulderG->getLocalPos() + cVector3d(L1*cos(th1Des), L1*sin(th1Des), 0.0));
    m_handG->setLocalPos(m_elbowG->getLocalPos() + cVector3d(L2*cos(th2Des), L2*sin(th2Des), 0.0));
    m_upperarmG->setLocalPos(m_shoulderG->getLocalPos());
    m_forearmG->setLocalPos(m_elbowG->getLocalPos());
    m_upperarmG->setLocalRot(cMatrix3d(th1Des, PI/2, 0.0, C_EULER_ORDER_ZYX));
    m_forearmG->setLocalRot(cMatrix3d(th2Des, PI/2, 0.0, C_EULER_ORDER_ZYX));

    // "ghost" target-state exoskeleton
    m_shoulderT->setLocalPos(origin);
    m_elbowT->setLocalPos(m_shoulderT->getLocalPos() + cVector3d(L1*cos(th1Trg), L1*sin(th1Trg), 0.0));
    m_handT->setLocalPos(m_elbowT->getLocalPos() + cVector3d(L2*cos(th2Trg), L2*sin(th2Trg), 0.0));
    m_upperarmT->setLocalPos(m_shoulderT->getLocalPos());
    m_forearmT->setLocalPos(m_elbowT->getLocalPos());
    m_upperarmT->setLocalRot(cMatrix3d(th1Trg, PI/2, 0.0, C_EULER_ORDER_ZYX));
    m_forearmT->setLocalRot(cMatrix3d(th2Trg, PI/2, 0.0, C_EULER_ORDER_ZYX));

    // only show ghost if control enabled
    m_parent->m_exo->m_ctrlLock.acquire();
    ctrl_states ctrl = m_parent->m_exo->m_ctrl;
    m_parent->m_exo->m_ctrlLock.release();
    if (ctrl == none) {
        m_shoulderG->setShowEnabled(false);
        m_elbowG->setShowEnabled(false);
        m_handG->setShowEnabled(false);
        m_upperarmG->setShowEnabled(false);
        m_forearmG->setShowEnabled(false);
        m_shoulderT->setShowEnabled(false);
        m_elbowT->setShowEnabled(false);
        m_handT->setShowEnabled(false);
        m_upperarmT->setShowEnabled(false);
        m_forearmT->setShowEnabled(false);
    } else {
        m_shoulderG->setShowEnabled(true);
        m_elbowG->setShowEnabled(true);
        m_handG->setShowEnabled(true);
        m_upperarmG->setShowEnabled(true);
        m_forearmG->setShowEnabled(true);
        m_shoulderT->setShowEnabled(true);
        m_elbowT->setShowEnabled(true);
        m_handT->setShowEnabled(true);
        m_upperarmT->setShowEnabled(true);
        m_forearmT->setShowEnabled(true);
    }
}

void chARMWidget::resizeGL(int a_width, int a_height)
{
    m_worldLock.acquire ();

    m_width = a_width;
    m_height = a_height;

    m_worldLock.release ();
}

void chARMWidget::mousePressEvent(QMouseEvent *event)
{
    if (m_parent->m_demo) {

        m_parent->m_exo->m_ctrlLock.acquire();
        ctrl_states ctrl = m_parent->m_exo->m_ctrl;
        m_parent->m_exo->m_ctrlLock.release();

        if (ctrl == task) {

            // acquire position of click relative to widget
            m_mouseX = event->pos().x();
            m_mouseY = event->pos().y();

            // print raw mouse positions for debugging
            if (DEBUG)  qDebug() << "(x,y) = (" << m_mouseX << "," << m_mouseY << ")";

            // convert position to CHAI coordinates
            cVector3d GUIcoord = cVector3d(m_mouseX, m_mouseY, 0.0);
            cVector3d CHAIcoord = m_scale*GUIcoord + m_shift;

            // if position is outside of RR "donut" workspace, move within
            cVector3d targ = m_parent->m_exo->findNearest(CHAIcoord);

            // update main window GUI
            m_parent->m_exo->setTarg(targ, task);
            m_parent->syncControlDisplay();
        }
    }
}
