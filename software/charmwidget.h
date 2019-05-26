#ifndef CHARMWIDGET_H
#define CHARMWIDGET_H

#include "chai3d.h"
#include "mainwindow.h"
#include "expwidget.h"
#include <cmath>
#include <QBasicTimer>
#include <QMouseEvent>

void _hapticThread(void *arg);  // pointer to thread function (not a class member)

class chARMWidget : public QGLWidget
{
public:
    MainWindow* m_parent;                     // pointer to main window

    chai3d::cThread m_thread;                 // haptics thread
    chai3d::cMutex m_worldLock;               // mutex for graphics updates
    chai3d::cMutex m_runLock;                 // mutex for haptics updates
    chai3d::cFrequencyCounter m_graphicRate;  // counter for graphics updates
    chai3d::cFrequencyCounter m_hapticRate;   // counter for haptics updates

    chai3d::cWorld* m_world;                  // CHAI world
    chai3d::cCamera* m_camera;                // camera to render the world
    chai3d::cDirectionalLight* m_light;       // light to illuminate the world
    chai3d::cShapeSphere* m_shoulder;         // sphere representing shoulder joint
    chai3d::cShapeSphere* m_elbow;            // sphere representing elbow joint
    chai3d::cShapeSphere* m_hand;             // sphere representing hand (end-effector)
    chai3d::cShapeSphere* m_shoulderG;        // "ghost" sphere representing desired position of shoulder joint
    chai3d::cShapeSphere* m_elbowG;           // "ghost" sphere representing desired position of elbow joint
    chai3d::cShapeSphere* m_handG;            // "ghost" sphere representing desired position of hand (end-effector)
    chai3d::cShapeSphere* m_shoulderT;        // "ghost" sphere representing target position of shoulder joint
    chai3d::cShapeSphere* m_elbowT;           // "ghost" sphere representing target position of elbow joint
    chai3d::cShapeSphere* m_handT;            // "ghost" sphere representing target position of hand (end-effector)
    chai3d::cShapeCylinder* m_upperarm;       // cylinder representing upperarm
    chai3d::cShapeCylinder* m_forearm;        // cylinder representing forearm
    chai3d::cShapeCylinder* m_upperarmG;      // "ghost" cylinder representing desired configuration of upperarm
    chai3d::cShapeCylinder* m_forearmG;       // "ghost" cylinder representing desired configuration of forearm
    chai3d::cShapeCylinder* m_upperarmT;      // "ghost" cylinder representing target configuration of upperarm
    chai3d::cShapeCylinder* m_forearmT;       // "ghost" cylinder representing target configuration of forearm
    chai3d::cLabel* m_OOB;                    // warning to display when desired position is outside subject's & robot's workspace

    QBasicTimer* m_timer;                     // timer for graphics updates
    bool m_running;                           // TRUE = haptics thread is running
    int m_width;                              // width of view
    int m_height;                             // height of view
    int m_mouseX;                             // cursor X-position
    int m_mouseY;                             // cursor Y-position

    chARMWidget(QWidget *parent);
    virtual ~chARMWidget();

    bool start();
    void stop();
    void* hapticThread();

protected:
    // map from GUI to CHAI coordinates (scaling + shift)
    const chai3d::cMatrix3d m_scale = chai3d::cMatrix3d(0.0014,0.0,0.0,0.0,-0.0014,0.0,0.0,0.0,0.0);
    const chai3d::cVector3d m_shift = chai3d::cVector3d(-0.4927, 0.8486, 0.0);

    void initializeGL();
    void paintGL();
    void drawExo();
    void resizeGL(int a_width, int a_height);
    void mousePressEvent(QMouseEvent *event);
    void timerEvent(QTimerEvent *event) { updateGL(); }
};

#endif // CHARMWIDGET_H
