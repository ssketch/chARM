#ifndef EXO_H
#define EXO_H

#include "chai3d.h"
#include "subject.h"
#include "motorcontrol.h"
#include "Windows.h"
#include <cmath>
#include <array>
#include <QMessageBox>

#define NUM_ENC 2  // number of encoders (0 = shoulder, 1 = elbow)
#define NUM_MTR 2  // number of motors (0 = shoulder, 1 = elbow)
#define PI      3.141592

// enumeration of control modes
typedef enum
{
    position,  // position control: track position/velocity
    force      // force control: apply torques/forces, independent of position
} ctrl_modes;

// enumeration of control paradigms
typedef enum
{
    none,      // shoulder and elbow both free
    shoulder,  // only control over shoulder angle, elbow free
    elbow,     // only control over elbow angle, shoulder free
    joint,     // control over both shoulder & elbow angles
    task       // control over hand position
} ctrl_states;

// parameters defining exo movement trajectory (with defaults)
typedef struct
{
    chai3d::cVector3d posInit
        = chai3d::cVector3d(0.0,0.0,0.0);  // initial position ([thS,thE] in joint space, [x,y] in task space)
    double tInit = 0.0;                    // time target is set [sec]
    double dpos[NUM_ENC] = {0,0};          // total displacement vector ([dthS,dthE] in joint space, [dr,th] in task space)
    double dt[NUM_ENC] = {0,0};            // time allotted for movement, in each DOF [sec]
} moveTraj;

class exo
{
public:
    subject* m_subj;                        // pointer to subject associated with exoskeleton

    bool m_error;                           // TRUE = problem with exo while running
    std::string m_errMessage;               // error message
    double m_t;                             // current time [sec]
    int m_thZero[NUM_ENC];                  // zero angles for motor-angle measurement [counts, in motor space]
    chai3d::cPrecisionClock* m_clk;         // pointer to clock for computing velocity and ramping torques
    chai3d::cVector3d m_thLink;             // current linkage angles [rad]
    chai3d::cVector3d m_th;                 // current joint angles [rad]
    chai3d::cVector3d m_thTarg;             // target joint angles (by end of trajectory) [rad]
    chai3d::cVector3d m_thDes;              // desired joint angles (at current time step) [rad]
    chai3d::cVector3d m_thErr;              // joint-angle error (relative to 'm_thDes') [rad]
    chai3d::cVector3d m_thdot;              // current joint velocities [rad/s]
    chai3d::cVector3d m_thdotDes;           // desired joint velocities [rad/s]
    chai3d::cVector3d m_thdotErr;           // joint-velocity error [rad/s]
    chai3d::cVector3d m_thErrInt;           // integrated joint angle error [rad*s]
    chai3d::cVector3d m_pos;                // current end-effector position [m]
    chai3d::cVector3d m_posTarg;            // target end-effector position (by end of trajectory) [m]
    chai3d::cVector3d m_posDes;             // desired end-effector position (at current time step) [m]
    chai3d::cVector3d m_posErr;             // end-effector position error (relative to 'm_posDes') [m]
    chai3d::cVector3d m_vel;                // current end-effector linear velocity [m/s]
    chai3d::cVector3d m_velDes;             // desired end-effector linear velocity [m/s]
    chai3d::cVector3d m_velErr;             // end-effector velocity error [m/s]
    chai3d::cVector3d m_posErrInt;          // integrated end-effector position error [m*s]

    ctrl_modes m_mode;                      // current control mode
    ctrl_states m_ctrl;                     // current control paradigm
    moveTraj m_traj;                        // current trajectory for control
    bool m_onTraj;                          // TRUE = follow designated trajectory (*important for GUI-based joint-space target setting)
    chai3d::cMutex m_ctrlLock;              // mutex for accessing control variable (changed by multiple threads)
    bool m_bumpers;                         // TRUE = virtual walls prevent collision with acrylic frame
    bool m_negDamp;                         // TRUE = negative damping control to help with friction
    bool m_dither;                          // TRUE = high-frequency actuation to help with (nonlinear) friction
    std::array<bool,NUM_MTR> m_activeJnts;  // list of joints to be actively controlled (not including "bumper", negative damping, or dither torques)
    std::array<bool,NUM_MTR> m_lockedJnts;  // list of joints to be MANUALLY locked
    chai3d::cVector3d m_KpJnt;              // proportional gains for joint-space control
    chai3d::cVector3d m_KdJnt;              // derivative gains for joint-space control
    chai3d::cVector3d m_KiJnt;              // integral gains for joint-space control
    chai3d::cVector3d m_KpTsk;              // proportional gains for task-space control
    chai3d::cVector3d m_KdTsk;              // derivative gains for task-space control
    chai3d::cVector3d m_KiTsk;              // integral gains for task-space control
    chai3d::cVector3d m_KdNeg;              // negative damping values for joint-space friction compensation
    chai3d::cVector3d m_Adith;              // dither amplitude for joint-space (nonlinear) friction compensation [N*m]
    chai3d::cVector3d m_fdith;              // dither frequency for joint-space (nonlinear) friction compensation [Hz]
    chai3d::cVector3d m_T;                  // desired joint torques [N*m]
    chai3d::cVector3d m_F;                  // desired end-effector force [N]

    exo(subject *a_subj);
    ~exo();

    bool connect();
    bool disconnect();
    void calibrate();
    void getState();
    bool sendCommand();
    bool reachedTarg();
    void setMode(ctrl_modes a_mode);
    void setCtrl(ctrl_states a_ctrl);
    void setTarg(chai3d::cVector3d a_targ, ctrl_states a_ctrl);
    void setForce(chai3d::cVector3d a_force, ctrl_states a_ctrl);
    void syncStates(bool resetTarg = true);
    chai3d::cVector3d forwardKin(chai3d::cVector3d a_th);
    chai3d::cVector3d inverseKin(chai3d::cVector3d a_pos);
    chai3d::cVector3d findNearest(chai3d::cVector3d a_pos);

protected:
    bool m_exoAvailable;            // TRUE = exoskeleton instance has been created
    bool m_exoReady;                // TRUE = connection to exoskeleton successful
    chai3d::cVector3d m_thLinkLim;  // max shoulder link and min elbow link angles [rad, depends on handedness]
    chai3d::cVector3d m_thLinkNom;  // nominal offsets from link-angle zeros [rad, in linkage space]

    chai3d::cVector3d getAngles();
    chai3d::cMatrix3d Jacobian(chai3d::cVector3d a_th);
    void updateDesiredState(double a_dt);
    void setJntTorqs(chai3d::cVector3d a_torque);
    void setEndForce(chai3d::cVector3d a_force);
    void disableCtrl() { setJntTorqs(chai3d::cVector3d(0.0,0.0,0.0)); }
    bool jointSpaceCtrl(ctrl_modes a_mode);
    bool taskSpaceCtrl(ctrl_modes a_mode);
    int isOutOfBounds(chai3d::cVector3d a_pos, bool space);
    bool isTooFast(chai3d::cVector3d a_vel, bool space);
    double angleDiff(double a_thA, double a_thB);
    chai3d::cVector3d vecDiff(chai3d::cVector3d a_vecA, chai3d::cVector3d a_vecB);
};

#endif // EXO_H
