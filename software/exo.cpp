#include "exo.h"
#include <QDebug>

#define RATIO_S        16.98     // gear ratio between shoulder motor and capstan (derived experimentally)
#define RATIO_E        16.59     // gear ratio between elbow motor and capstan (derived experimentally)
#define JNT_OFFSET_U   1.60      // elbow joint offset from upperarm linkage [in]
#define JNT_OFFSET_F   1.53      // elbow joint offset from forearm linkage [in]
#define LINKMAX_S_R    62.343    // maximum angle for right-handed shoulder link, to avoid frame collision (also used for calibration) [deg]
#define LINKMIN_E_R    25.541    // minimum angle for right-handed elbow link, to avoid frame collision (also used for calibration) [deg]
#define LINKMAX_S_L    62.321    // maximum angle for left-handed shoulder link, to avoid frame collision (also used for calibration) [deg]
#define LINKMIN_E_L    26.300    // minimum angle for left-handed elbow link, to avoid frame collision (also used for calibration) [deg]
#define ELB_MAX        145       // approx. maximum angle for elbow joint, to avoid linkage collison [deg]
#define EPS_TH         0.25      // angular epsilon to prevent inappropriate "out of bounds" errors [deg]
#define EPS_DR         0.05      // radial epsilon for calculating nearest point in RR "donut" workspace [m]
#define BUFFER         5         // buffer for wall to prevent links from hitting frame [deg]
#define KpWALL         5         // stiffness of buffer wall
#define T_MAX          12        // maximum torque to command, same as (original) KINARM [N*m]
#define THDOT_MAX      0.4       // maximum angular speed over minimum-jerk trajectory [rad/s]
#define V_MAX          0.2       // maximum linear speed over minimum-jerk trajectory [m/s]
#define A_FILT         0.5       // weight for velocity filtering
#define INT_CLMP       750       // maximum allowed integrated error
#define THRESH_EQ      0.01      // threshold for saying that angles are "equal" [rad]
#define THRESH_JNT     1.5       // threshold for resetting joint angle [deg]
#define THRESH_HND     1.0       // threshold for resetting hand position [cm]
#define CNT_TO_STOP    250       // number of zero-velocity checks to guarantee exo is stationary
#define JNTSPACE       0         // joint-space control
#define TASKSPACE      1         // task-space control
#define TOO_CLOSE      2         // position is inside RR "donut" workspace
#define TOO_FAR        3         // position is outside RR "donut" workspace
#define SUBJ_UNREACH   4         // configuration is unreachable given subject joint limits
#define ROBT_UNREACH   5         // configuration is unreachable given robot joint limits
#define CM_TO_METERS   0.01      // conversion factor between centimeters and meters
#define INCH_TO_METERS 0.0254    // conversion factor between inches and meters
#define PI             3.141592
#define DEBUG          0

using namespace std;
using namespace chai3d;

exo::exo(subject *a_subj)
{
    // exoskeleton available for connection
    m_exoAvailable = true;
    m_exoReady = false;
    m_error = false;
    m_errMessage = "";

    // associate exo with provided subject
    m_subj = a_subj;

    // initialize kinematic variables
    m_t          = 0;
    for (int i = 0; i < NUM_ENC; i++) { m_thZero[i] = 0; }
    m_clk        = new cPrecisionClock();
    m_thLinkLim  = cVector3d(LINKMAX_S_R, LINKMIN_E_R, 0.0)*(PI/180);
    m_thLinkNom  = cVector3d(0.0,0.0,0.0);
    m_thLink     = cVector3d(0.0,0.0,0.0);
    m_th         = cVector3d(0.0,0.0,0.0);
    m_thTarg     = cVector3d(0.0,0.0,0.0);
    m_thDes      = cVector3d(0.0,0.0,0.0);
    m_thErr      = cVector3d(0.0,0.0,0.0);
    m_thdot      = cVector3d(0.0,0.0,0.0);
    m_thdotDes   = cVector3d(0.0,0.0,0.0);
    m_thdotErr   = cVector3d(0.0,0.0,0.0);
    m_thErrInt   = cVector3d(0.0,0.0,0.0);
    m_pos        = cVector3d(0.0,0.0,0.0);
    m_posTarg    = cVector3d(0.0,0.0,0.0);
    m_posDes     = cVector3d(0.0,0.0,0.0);
    m_posErr     = cVector3d(0.0,0.0,0.0);
    m_vel        = cVector3d(0.0,0.0,0.0);
    m_velDes     = cVector3d(0.0,0.0,0.0);
    m_velErr     = cVector3d(0.0,0.0,0.0);
    m_posErrInt  = cVector3d(0.0,0.0,0.0);

    // initialize control variables
    m_mode       = position;
    m_ctrl       = none;
    m_onTraj     = false;
    m_bumpers    = false;
    m_negDamp    = false;
    m_dither     = false;
    m_activeJnts = {false, false};
    m_lockedJnts = {false, false};
    m_KpJnt      = cVector3d(6.0,3.0,0.0);
    m_KdJnt      = cVector3d(2.0,2.0,0.0);
    m_KiJnt      = cVector3d(0.0005,0.0005,0.0);
    m_KpTsk      = cVector3d(6.5,6.5,0.0);
    m_KdTsk      = cVector3d(2.0,2.0,0.0);
    m_KiTsk      = cVector3d(0.0005,0.0005,0.0);
    m_KdNeg      = cVector3d(0.0,0.0,0.0);
    m_Adith      = cVector3d(0.2,0.2,0.0);
    m_fdith      = cVector3d(100,100,0.0);
    m_T          = cVector3d(0.0,0.0,0.0);
    m_F          = cVector3d(0.0,0.0,0.0);
}

exo::~exo()
{
    // disconnect from S826 and free dynamically allocated memory
    if (disconnect()) {
        delete m_clk;
    }
}


bool exo::connect()
{
    // check if exoskeleton is available/has been opened already
    if (!m_exoAvailable) return(C_ERROR);
    if (m_exoReady)      return(C_ERROR);

    // connect to S826
    bool success = connectToS826();
    if (!success) return(C_ERROR);

    // initialize encoders
    for (int i = 0; i < NUM_ENC; i++) {
        success = initEncod((uint)i);
        if (!success) return(C_ERROR);
    }

    // initialize motors
    for (int i = 0; i < NUM_MTR; i++) {
        success = initMotor((uint)i);
        if (!success) return(C_ERROR);
    }

    m_clk->start();
    m_exoReady = true;
    return(C_SUCCESS);
}

bool exo::disconnect()
{
    // check that exoskeleton is open
    if (!m_exoReady) return(C_ERROR);

    // set motor torques to zero and disconnect from S826
    disableCtrl();
    disconnectFromS826();

    m_clk->stop();
    m_exoReady = false;
    return(C_SUCCESS);
}

void exo::calibrate()
{
    // turn off any control before blocking code
    disableCtrl();

    // ready user for calibration
    QMessageBox msgBox;
    msgBox.setText("READY TO CALIBRATE?");
    msgBox.setInformativeText("Move shoulder and elbow links to their limits. To prevent "
                              "drift of the linkage, hold it in place while calibrating.");
    msgBox.exec();

    // save encoder counts at calibration position
    for (int i = 0; i < NUM_ENC; i++) {
        m_thZero[i] = getCounts((uint)i);
    }

    // set linkage limits based on handedness
    if (!m_subj->m_rightHanded)  m_thLinkLim = cVector3d(LINKMAX_S_L, LINKMIN_E_L, 0.0)*(PI/180);
    m_thLinkNom = m_thLinkLim;
}

void exo::getState()
{
    // declare "memory" variables for calculations
    static double tLast = m_t;
    static cVector3d thLast = m_th;
    static cVector3d thdotLast = m_thdot;
    static cVector3d thErrIntLast = m_thErrInt;
    static cVector3d posErrIntLast = m_posErrInt;

    // update from trajectory (or reset control variables)
    if (!isOutOfBounds(m_thTarg, JNTSPACE) && m_onTraj) {
        updateDesiredState(m_t - m_traj.tInit);
    } else {
        syncStates(false);
        thErrIntLast = cVector3d(0.0,0.0,0.0);
        posErrIntLast = cVector3d(0.0,0.0,0.0);
    }

    // get current joint & task-space positions/errors
    m_th = getAngles();
    m_thErr = vecDiff(m_thDes, m_th);
    m_pos = forwardKin(m_th);
    m_posErr = m_posDes - m_pos;

    // calculate velocities/errors
    m_t = m_clk->getCurrentTimeSeconds();
    m_thdot = A_FILT*(m_th - thLast)/(m_t - tLast) + (1-A_FILT)*thdotLast;
    m_thdotErr = m_thdotDes - m_thdot;
    m_vel = Jacobian(m_th)*m_thdot;
    m_velErr = m_velDes - m_vel;

    // integrate position error
    m_thErrInt = thErrIntLast + m_thErr*(m_t - tLast);
    m_posErrInt = posErrIntLast + m_posErr*(m_t - tLast);

    // clamp integrated error
    for (int i = 0; i < NUM_ENC; i++) {
        if (fabs(m_thErrInt(i)) > INT_CLMP) {
            m_thErrInt(i) = (m_thErrInt(i)/fabs(m_thErrInt(i)))*INT_CLMP;
        }
        if (fabs(m_posErrInt(i)) > INT_CLMP) {
            m_posErrInt(i) = (m_posErrInt(i)/fabs(m_posErrInt(i)))*INT_CLMP;
        }
    }

    // save last values
    tLast = m_t;
    thLast = m_th;
    thdotLast = m_thdot;
    thErrIntLast = m_thErrInt;
    posErrIntLast = m_posErrInt;
}

bool exo::sendCommand()
{
    // wait to get control mode & paradigm (and send command based
    // on this mode/paradigm) until all other threads are done with it
    m_ctrlLock.acquire();
    ctrl_modes mode = m_mode;
    ctrl_states ctrl = m_ctrl;
    m_ctrlLock.release();

    // check current control paradigm and command accordingly
    switch (ctrl) {
    case none:
        m_activeJnts = {false, false};
        disableCtrl();
        return(C_SUCCESS);
    case shoulder:
        m_activeJnts = {true, false};
        if (jointSpaceCtrl(mode)) return(C_SUCCESS);  // in workspace
        else                      return(C_ERROR);    // out of bounds
    case elbow:
        m_activeJnts = {false, true};
        if (jointSpaceCtrl(mode)) return(C_SUCCESS);
        else                      return(C_ERROR);
    case joint:
        m_activeJnts = {true, true};
        if (jointSpaceCtrl(mode)) return(C_SUCCESS);
        else                      return(C_ERROR);
    case task:
        m_activeJnts = {true, true};
        if (taskSpaceCtrl(mode)) return(C_SUCCESS);
        else                     return(C_ERROR);
    default:
        m_activeJnts = {false, false};
        disableCtrl();
        return(C_SUCCESS);
    }
}

bool exo::reachedTarg()
{
    // create variable to track velocity
    static int count = 0;

    // if controlling in task space or both joints in joint space
    if (m_ctrl == task || m_ctrl == joint) {

        // get appropriate "error" vectors and position thresholds
        cVector3d posErr, velErr;
        double thresh;
        if (m_ctrl == task) {
            posErr = m_posTarg - m_pos;
            velErr = m_vel;
            thresh = THRESH_HND*CM_TO_METERS;
        } else {
            posErr = vecDiff(m_thTarg, m_th);
            velErr = m_thdot;
            thresh = THRESH_JNT*(PI/180);
        }

        // check that exo has reached desired configuration & is stopped
        if (posErr.length() <= thresh) {      // at target position
            if (velErr.length() <= thresh) {  // stopped currently
                count++;
                if (count > CNT_TO_STOP) {    // been stopped long enough
                    count = 0;
                    return true;
                } else {                      // not yet stopped long enough
                    return false;
                }
            } else {                          // not stopped currently
                count = 0;
                return false;
            }
        } else {                              // not at position
            count = 0;
            return false;
        }
    }

    // if just controlling single joint
    else {

        // get current and desired joint angles
        double currAng, desAng;
        if (m_ctrl == shoulder) {
            currAng = m_th(S);
            desAng = m_thTarg(S);
        } else {
            currAng = m_th(E);
            desAng = m_thTarg(E);
        }

        // check that joint has reached desired angle & is stopped
        if (fabs(currAng - desAng) <= THRESH_JNT*(PI/180)) {
            if (m_thdot.length() <= THRESH_JNT*(PI/180)) {
                count++;
                if (count > CNT_TO_STOP) {
                    count = 0;
                    return true;
                } else {
                    return false;
                }
            } else {
                count = 0;
                return false;
            }
        } else {
            count = 0;
            return false;
        }
    }
}

void exo::setMode(ctrl_modes a_mode)
{
    m_ctrlLock.acquire();
    m_mode = a_mode;
    m_ctrlLock.release();
}

void exo::setCtrl(ctrl_states a_ctrl)
{
    m_ctrlLock.acquire();
    syncStates(a_ctrl != m_ctrl);  // only reset target if switching control paradigm
    m_ctrl = a_ctrl;
    m_ctrlLock.release();
}

void exo::setTarg(chai3d::cVector3d a_targ, ctrl_states a_ctrl)
{
    // update control paradigm for position control
    setMode(position);
    setCtrl(a_ctrl);

    // update control target & trajectory parameters, assuming
    // that trajectory starts from current state
    if (a_ctrl == task) {
        m_posTarg = a_targ;
        m_thTarg = inverseKin(a_targ);

        double dx = a_targ(0) - m_pos(0);
        double dy = a_targ(1) - m_pos(1);
        double dr = sqrt(dx*dx + dy*dy);  // distance to move [m]
        double th = atan2(dy,dx);         // angle at which to move [rad, relative to start]
        double dt = (30*dr)/(16*V_MAX);   // time over which to sweep through path to meet max velocity constraint [sec]

        m_traj.posInit = m_pos;
        m_traj.dpos[0] = dr;    m_traj.dpos[1] = th;
        m_traj.dt[0] = dt;      m_traj.dt[1] = dt;
    } else {
        m_thTarg = a_targ;
        m_posTarg = forwardKin(a_targ);

        m_traj.posInit = m_th;
        cVector3d dth = vecDiff(a_targ, m_th);
        for (int i = 0; i < NUM_ENC; i++) {
            m_traj.dpos[i] = dth(i);
            m_traj.dt[i] = (30*fabs(dth(i)))/(16*THDOT_MAX);
        }
    }

    // set time of target update
    m_traj.tInit = m_t;

    // send onto trajectory
    m_onTraj = true;
}

void exo::setForce(chai3d::cVector3d a_force, ctrl_states a_ctrl)
{
    // update control paradigm for force control
    setMode(force);
    setCtrl(a_ctrl);

    // update desired "force"
    cMatrix3d J = Jacobian(m_th);
    J.trans();
    if (a_ctrl == task) {
        m_F = a_force;
        m_T = J*m_F;
    } else {
        m_T = a_force;
        if (J.invert()) m_F = J*m_T;
    }
}

void exo::syncStates(bool resetTarg)
{
    // sync desired state
    m_thDes = m_th;
    m_posDes = m_pos;
    m_thdotDes = cVector3d(0.0,0.0,0.0);
    m_velDes = cVector3d(0.0,0.0,0.0);

    // reset trajectory parameters
    moveTraj resetTraj;
    if (m_ctrl == task) resetTraj.posInit = m_pos;
    else                resetTraj.posInit = m_th;
    m_traj = resetTraj;

    // by default, reset target parameters
    if (resetTarg) {
        m_thTarg = m_th;
        m_posTarg = m_pos;
    }
}

cVector3d exo::forwardKin(cVector3d a_th)
{
    // extract subject parameters
    double L1 = m_subj->m_Lupper;
    double L2 = m_subj->m_LtoEE;
    double th1 = a_th(0);
    double th2 = a_th(1);

    // compute task-space position via forward kinematics
    // NOTE: this assumes reachability
    double x, y;
    if (m_subj->m_rightHanded) {
        x = L1*cos(th1) + L2*cos(th1+th2);
        y = L1*sin(th1) + L2*sin(th1+th2);
    } else {
        x = L1*cos(PI-th1) + L2*cos(PI-th1-th2);
        y = L1*sin(PI-th1) + L2*sin(PI-th1-th2);
    }
    return cVector3d(x, y, 0.0);
}

cVector3d exo::inverseKin(cVector3d a_pos)
{
    // extract subject parameters
    double L1 = m_subj->m_Lupper;
    double L2 = m_subj->m_LtoEE;
    double x = a_pos(0);
    double y = a_pos(1);

    // compute joint-space configuration via inverse kinematics
    // NOTE: this assumes reachability
    double th1, th2;
    double c2 = (x*x + y*y - L1*L1 - L2*L2)/(2.0*L1*L2);
    double s2 = sqrt(1.0 - c2*c2);
    th2 = atan2(s2,c2);
    if (m_subj->m_rightHanded) {
        th1 = atan2(y,x) - atan2(L2*s2, L1+L2*c2);
    } else {
        th1 = PI - (atan2(y,x) + atan2(L2*s2, L1+L2*c2));
    }
    return cVector3d(th1, th2, 0.0);
}

chai3d::cVector3d exo::findNearest(chai3d::cVector3d a_pos)
{
    // compute workspace parameters
    double L1 = m_subj->m_Lupper;
    double L2 = m_subj->m_LtoEE;
    double rMax = L1+L2;
    double rMin = fabs(L1-L2);

    // get details about input position
    int OOBstate = isOutOfBounds(a_pos, TASKSPACE);
    double x = a_pos(0);
    double y = a_pos(1);
    double th = atan2(y,x);

    // if not within "donut" workspace
    if (OOBstate == TOO_CLOSE || OOBstate == TOO_FAR) {
        double rNew, xNew, yNew;
        if (OOBstate == TOO_CLOSE) rNew = rMin + EPS_DR;
        else                       rNew = rMax - EPS_DR;
        xNew = rNew * cos(th);
        yNew = rNew * sin(th);
        return(cVector3d(xNew, yNew, 0.0));
    } else {
        return(a_pos);
    }
}


cVector3d exo::getAngles()
{
    // get angular offsets from motor zeros (checking for encoder failure)
    cVector3d th = cVector3d(0.0,0.0,0.0);
    for (int i = 0; i < NUM_ENC; i++) {
        if (checkEncod((uint)i)) {
            th(i) = getAngle((uint)i, m_thZero[i]);
            if (DEBUG) {
                qDebug() << "Mtr #" << i << " = " << th(i)*(180/PI) << " deg";
                qDebug() << " ";
            }
        } else {
            // enter "failsafe" mode
            m_error = true;
            if (m_errMessage.empty())  m_errMessage += "   + quadrature error on encoder #" + to_string(i);
            else                       m_errMessage += "\n   + quadrature error on encoder #" + to_string(i);
            return m_th;
        }
    }

    // compute linkage angles
    if (!m_subj->m_rightHanded) th = -1.0*th;  // handedness
    th(0) = th(0)/RATIO_S;                     // gearing
    th(1) = th(1)/RATIO_E;
    m_thLink = th + m_thLinkNom;               // offsets

    // print linkage angles for debugging
    if (DEBUG){
        qDebug() << "Shoulder Link = " << m_thLink(0)*(180/PI) << " deg";
        qDebug() << "Elbow Link    = " << m_thLink(1)*(180/PI) << " deg";
        qDebug() << " ";
    }

    // convert from linkage to "nominal" joint space
    cVector3d thJnt = cVector3d(0.0,0.0,0.0);
    thJnt(0) = m_thLink(0);
    thJnt(1) = PI/2.0 + m_thLink(1) - m_thLink(0);

    // correct for joint offsets relative to linkage
    double alpha = asin(JNT_OFFSET_U*INCH_TO_METERS/m_subj->m_Lupper);
    double beta = asin(JNT_OFFSET_F*INCH_TO_METERS/m_subj->m_LtoEE);
    thJnt(0) = thJnt(0) - alpha;
    thJnt(1) = thJnt(1) + alpha - beta;

    // print joint angles for debugging
    if (DEBUG){
        qDebug() << "Shoulder Joint = " << thJnt(0)*(180/PI) << " deg";
        qDebug() << "Elbow Joint    = " << thJnt(1)*(180/PI) << " deg";
        qDebug() << " ";
    }

    // constrain to [0,2*PI)
    for (int i = 0; i < NUM_ENC; i++) {
        thJnt(i) = fmod(thJnt(i),2*PI);
        if (thJnt(i) < 0) thJnt(i) += 2*PI;
    }
    return thJnt;
}

cMatrix3d exo::Jacobian(cVector3d a_th)
{
    // extract subject parameters
    double L1 = m_subj->m_Lupper;
    double L2 = m_subj->m_LtoEE;
    double th1 = a_th(0);
    double th2 = a_th(1);

    // compute entries of Jacobian for given joint angles
    // NOTE: this assumes reachability
    double J11, J12, J21, J22;
    if (m_subj->m_rightHanded) {
        J11 = -1.0*(L1*sin(th1) + L2*sin(th1+th2));
        J12 = -1.0*L2*sin(th1+th2);
        J21 = L1*cos(th1) + L2*cos(th1+th2);
        J22 = L2*cos(th1+th2);
    } else {
        J11 = L1*sin(th1) + L2*sin(th1+th2);
        J12 = L2*sin(th1+th2);
        J21 = L1*cos(th1) + L2*cos(th1+th2);
        J22 = L2*cos(th1+th2);
    }
    return cMatrix3d(J11,J12,0.0,J21,J22,0.0,0.0,0.0,1.0);  // pad matrix to make 3x3
}

void exo::updateDesiredState(double a_dt)
{
    // compute differently if trajectory in task or joint space
    if (m_ctrl == task) {
        double drTot = m_traj.dpos[0];
        double dtTot = m_traj.dt[0];  // elements of array are equal

        // check if should have reached target
        if (a_dt >= dtTot) {
            m_thDes = m_thTarg;
            m_posDes = m_posTarg;
            m_thdotDes = chai3d::cVector3d(0.0,0.0,0.0);
            m_velDes = chai3d::cVector3d(0.0,0.0,0.0);
        } else {

            // compute desired displacement & speed in polar coordinates
            double dr = drTot*(10*pow(a_dt/dtTot,3) - 15*pow(a_dt/dtTot,4) + 6*pow(a_dt/dtTot,5));
            double drDot = (drTot/dtTot)*(30*pow(a_dt/dtTot,2) - 60*pow(a_dt/dtTot,3) + 30*pow(a_dt/dtTot,4));

            // convert to Cartesian coordinates
            double th = m_traj.dpos[1];
            cVector3d polrToCart = cVector3d(cos(th),sin(th),0.0);
            m_posDes = m_traj.posInit + dr * polrToCart;
            m_velDes = drDot * polrToCart;

            // convert to joint space, for consistency
            m_thDes = inverseKin(m_posDes);
            cMatrix3d Jinv;
            cMatrix3d J = Jacobian(m_thDes);
            if (J.invertr(Jinv)) m_thdotDes = Jinv*m_velDes;
        }

    } else {

        // compute desired angular displacements & speeds in joint space
        for (int i = 0; i < NUM_ENC; i++) {
            if (a_dt >= m_traj.dt[i]) {
                m_thDes(i) = m_thTarg(i);
                m_thdotDes(i) = 0.0;
            } else {
                m_thDes(i) = m_traj.posInit(i) +
                        m_traj.dpos[i]*(10*pow(a_dt/m_traj.dt[i],3) - 15*pow(a_dt/m_traj.dt[i],4) + 6*pow(a_dt/m_traj.dt[i],5));
                m_thdotDes(i) = (m_traj.dpos[i]/m_traj.dt[i])*
                        (30*pow(a_dt/m_traj.dt[i],2) - 60*pow(a_dt/m_traj.dt[i],3) + 30*pow(a_dt/m_traj.dt[i],4));
            }
        }

        // convert to task space, for consistency
        m_posDes = forwardKin(m_thDes);
        m_velDes = Jacobian(m_thDes)*m_thdotDes;
    }
}

void exo::setJntTorqs(cVector3d a_torque)
{
    cVector3d T = a_torque;

    // "zero" torque commanded to inactive joints
    for (int i = 0; i < NUM_MTR; i++) if (!m_activeJnts[i])  T(i) = 0.0;
    if (DEBUG)  qDebug() << "T = [" << T(0) << " , " << T(1) << "] N";

    // apply "bumper" torques to unlocked joints
    if (m_bumpers) {
        if (m_thLink(0)*(180/PI) > m_thLinkLim(0)*(180/PI)-BUFFER && m_lockedJnts[0]) {
            T(0) += KpWALL*angleDiff((m_thLinkLim(0)*(180/PI)-BUFFER)*(PI/180), m_thLink(0));  // push upperarm away from frame
        }
        if (m_th(1)*(180/PI) > ELB_MAX-BUFFER && m_lockedJnts[1]) {
            T(1) += KpWALL*angleDiff((ELB_MAX-BUFFER)*(PI/180), m_th(1));                      // push forearm away from frame
        }
        if (m_thLink(1)*(180/PI) < m_thLinkLim(1)*(180/PI)+BUFFER && m_lockedJnts[1]) {
            T(1) += KpWALL*angleDiff((m_thLinkLim(1)*(180/PI)+BUFFER)*(PI/180), m_thLink(1));  // keep upperarm & forearm apart
        }
    }

    // add negative damping to inactive/unlocked joints
    if (m_negDamp) {
        if (DEBUG)  qDebug() << "adding negative damping";
        cVector3d T_negDamp = m_thdot;
        T_negDamp.mulElement(m_KdNeg);
        for (int i = 0; i < NUM_MTR; i++) {
            if (!m_activeJnts[i] && !m_lockedJnts[i])  T += T_negDamp;
        }
    }

    // convert torques from robot to motor space
    T(0) = T(0)/RATIO_S;                     // gearing
    T(1) = T(1)/RATIO_E;
    if (!m_subj->m_rightHanded) T = -1.0*T;  // handedness

    // add high-frequency dither to inactive/unlocked joints
    if (m_dither) {
        if (DEBUG)  qDebug() << "adding dither";
        for (int i = 0; i < NUM_MTR; i++) {
            if (!m_activeJnts[i] && !m_lockedJnts[i])
                T(i) += m_Adith(i)*sin(2.0*PI*m_fdith(i)*m_t);
        }
    }

    // command torques to unlocked joints, saturating for safety
    for (int i = 0; i < NUM_MTR; i++) {
        if (fabs(T(i)) > T_MAX)  T(i) = (T(i)/fabs(T(i)))*T_MAX;
        if (!m_lockedJnts[i])  setTorque((uint)i, T(i));
    }
}

void exo::setEndForce(cVector3d a_force)
{
    // compute Jacobian transpose
    cMatrix3d J = Jacobian(m_th);
    J.trans();

    // convert force to torque
    cVector3d Tequiv = J*a_force;
    setJntTorqs(Tequiv);
}

bool exo::jointSpaceCtrl(ctrl_modes a_mode)
{
    switch (a_mode) {

    case position:

        // only exert torques if target configuration is within subject's workspace
        if (!isOutOfBounds(m_thTarg, JNTSPACE)) {
            cVector3d T_pos = m_thErr;         T_pos.mulElement(m_KpJnt);
            cVector3d T_vel = m_thdotErr;      T_vel.mulElement(m_KdJnt);
            cVector3d T_posInt = m_thErrInt;   T_posInt.mulElement(m_KiJnt);
            m_T = T_pos + T_vel + T_posInt;
            setJntTorqs(m_T);
            return(C_SUCCESS);
        } else {
            disableCtrl();
            return(C_ERROR);
        }
        break;

    case force:
        setJntTorqs(m_T);
        return(C_SUCCESS);
        break;

    default:
        disableCtrl();
        return(C_ERROR);
        break;
    }
}

bool exo::taskSpaceCtrl(ctrl_modes a_mode)
{
    switch (a_mode) {

    case position:

        // only exert force if target position is within subject's workspace
        // NOTE: because the exo's workspace is non-convex, it is possible that a
        // ----  desired position en route to the target is unreachable; this
        //       should be handled by the natural deviation from straight-line
        //       trajectory that is caused by the exo's uncompensated dynamics
        if (!isOutOfBounds(m_posTarg, TASKSPACE)) {
            cVector3d F_pos = m_posErr;         F_pos.mulElement(m_KpTsk);
            cVector3d F_vel = m_velErr;         F_vel.mulElement(m_KdTsk);
            cVector3d F_posInt = m_posErrInt;   F_posInt.mulElement(m_KiTsk);
            m_F = F_pos + F_vel + F_posInt;
            setEndForce(m_F);
            return(C_SUCCESS);
        } else {
            disableCtrl();
            return(C_ERROR);
        }
        break;

    case force:
        setEndForce(m_F);
        return(C_SUCCESS);
        break;

    default:
        disableCtrl();
        return(C_ERROR);
        break;
    }
}

int exo::isOutOfBounds(cVector3d a_pos, bool space)
{
    // extract subject parameters
    double L1 = m_subj->m_Lupper;
    double L2 = m_subj->m_LtoEE;
    double th1_min = m_subj->m_lims.shoul_min;
    double th1_max = m_subj->m_lims.shoul_max;
    double th2_min = m_subj->m_lims.elbow_min;
    double th2_max = m_subj->m_lims.elbow_max;

    // in task space (space = 1)
    cVector3d th;
    if (space == TASKSPACE) {
        // check "donut" workspace, ignoring joint limits
        double x = a_pos(0);
        double y = a_pos(1);
        bool tooClose = (fabs(L1-L2) >= sqrt(x*x + y*y));
        bool tooFar   = (fabs(L1+L2) <= sqrt(x*x + y*y));
        if    (tooClose) return(TOO_CLOSE);
        else if (tooFar) return(TOO_FAR);
        else             th = inverseKin(a_pos);
    }
    // in joint space (space = 0)
    else {
        th = a_pos;
    }

    // check subject joint limits
    double th1 = th(0);
    double th2 = th(1);
    if (th1 > th1_max || th1 < th1_min) return(SUBJ_UNREACH);
    if (th2 > th2_max || th2 < th2_min) return(SUBJ_UNREACH);

    // check robot joint limits
    double alpha = asin(JNT_OFFSET_U*INCH_TO_METERS/m_subj->m_Lupper);
    double beta = asin(JNT_OFFSET_F*INCH_TO_METERS/m_subj->m_LtoEE);
    double thLink0 = th1 + alpha;
    double thLink1 = th2 + th1 - PI/2 + beta;
    if (th2*(180/PI) > ELB_MAX)                                 return(ROBT_UNREACH);
    if (thLink0*(180/PI) > (m_thLinkLim(0)*(180/PI) + EPS_TH))  return(ROBT_UNREACH);
    if (thLink1*(180/PI) < (m_thLinkLim(1)*(180/PI) - EPS_TH))  return(ROBT_UNREACH);

    // in bounds
    return(0);
}

bool exo::isTooFast(chai3d::cVector3d a_vel, bool space)
{
    // in task space (space = 1)
    if (space == TASKSPACE) {
        if (a_vel.length() > V_MAX) return(true);
        else                        return(false);
    }
    // in joint space (space = 0)
    else {
        for (int i = 0; i < NUM_ENC; i++) {
            if (a_vel(i) > THDOT_MAX*180/PI) return(true);
        }
        return(false);
    }
}

double exo::angleDiff(double a_thA, double a_thB)
{
    // determine shortest distance between A and B
    double diff1 = fabs(a_thA - a_thB);
    double diff2 = 2*PI - diff1;
    double diff = fmin(diff1, diff2);

    // determine direction for moving from B to A along shortest path
    if (abs(fmod(a_thB + diff, 2*PI) - a_thA) < THRESH_EQ)  return  1.0*diff;
    else                                                    return -1.0*diff;
}

cVector3d exo::vecDiff(cVector3d a_vecA, cVector3d a_vecB)
{
    cVector3d diff;
    for (int i = 0; i < NUM_ENC; i++) {
        diff(i) = angleDiff(a_vecA(i), a_vecB(i));
    }
    return diff;
}
