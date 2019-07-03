#include "expwidget.h"
#include <QDebug>

#define T_GRAPHICS   50        // update graphics every 50 ms (20 Hz)
#define T_RECORD     50        // record movement data every 50 ms (20 Hz), at most
#define CAM_X_RIGHT  0.04      // x-coordinate of camera for all tests with right hand (to be shifted in x for left-handed tests, for RIGHT = 0.0590 / for LEFT = 0.0372) [m]
#define CAM_Y        0.365     // y-coordinate of camera [m]
#define CAM_Z        1.0       // z-coordinate of camera (aka. height above scene) [m]
#define MIRROR       1         // 1 = graphics are mirrored in display
#define CORR_PARALL  0         // 1 = correct for parallax, 0 = leave graphics directly above actual arm
#define D_PARALLAX   0.09      // distance of arm below graphics plane [m]
#define D_TO_EYES    0.07      // distance of eyes in front of shoulder [m]
#define D_TV_EXO_R   0.129     // x-distance from right edge of TV display to corner of exo opposite main axis, when exo configured for right-hand testing [m]
#define D_TV_EXO_L   0.111     // x-distance from left edge of TV display to corner of exo opposite main axis, when exo configured for left-hand testing [m]
#define W_SCREEN     1.022     // screen width (used for setting orthographic view)
#define W_TV         1.049     // width of TV display [m]
#define W_EXO        0.441     // distance between exo's main axis and corner of exo opposite main axis [m]
#define R_JOINT      0.04      // radius of spheres representing joints [m]
#define R_SEGMENT    0.02      // radius of cylinders representing upper/forearm [m]
#define H_SEGMENT    0.5       // default height of cylinders representing upper/forearm [m]
#define H_INF        10        // "infinity" (cylinder height) for 1-D tests where end of segment should not be visible [m]
#define L_SCALE      0.9606    // scaling factor for graphics, to enable matching between real & virtual worlds (prior to parallax correction)
#define L_RELRAD     0.9       // scaling factor between exoskeleton joint/segment & test target radii
#define L_RELLEN     1.25      // scaling factor between exoskeleton segment and test target lengths
#define ALPH_GHOST   0.5       // alpha value for exoskeleton components
#define MOUSE_STEP   15        // angle equivalent of 1 "click" of mouse scroll wheel [deg]
#define JNTSPACE     0         // joint-space control
#define TASKSPACE    1         // task-space control
#define CNTDWN       0         // 1 = trial time limited by countdown timer
#define ANG_STEP     0.5       // step size when moving angle cursor in 1-D passive test [deg]
#define POS_STEP     0.5       // step size when moving position cursor in 2-D passive test [cm]
#define DOUB_STAIRS  1         // 1 = two staircases at a time (randomized), 0 = one staircase at a time
#define ADAPTIVE     0         // 1 = adaptive test, 0 = standard test
#define CLOSER       0         // reference is rotated closer to body than actual arm (test angle)
#define FURTHER      1         // reference is rotated further from body than actual arm (test angle)
#define FROM_CLOSER  0         // in double-staircase paradigm, staircase for which reference angle starts closer to body
#define FROM_FURTHER 1         // in double-staircase paradigm, staircase for which reference angle starts further from body
#define START_DEV    30        // starting deviation from test angle for staircases 1 & 2 [deg]
#define MAX_REVERSAL 4         // number of reversals for each staircase
#define NUM_FOR_EST  4         // in adaptive paradigm, number of reference angles (from each staircase) to use for estimation of perceptual boundary/sensitivity
#define CENTER_X     3.5       // x-coordinate of reach center for right hand (negative for left hand) [cm]
#define CENTER_Y     34.5      // y-coordinate of reach center [cm]
#define CENTEROUT    1         // 1 = 2-D reaches begin from center of target circle, 0 = 2-D reaches begin from other (non-adjacent) target on circle
#define REACH_DIST   10        // reach distance [cm]
#define NUM_VISION   2         // number of trials per matching target with visual feedback
#define MANUAL_LOCK  0         // 1 = manually (physically) lock unused joint for 1-D tests, 0 = "locking" done automatically via exo control
#define WAIT_TIME    2         // time to wait for exoskeleton to reset for grounding [sec]
#define GROUND_TIME  3         // time to display arm location for visual grounding (preventing proprioceptive drift) [sec]
#define SHORT_BREAK  1         // duration of short break [min]
#define LONG_BREAK   2         // duration of long break [min]
#define CM_TO_METERS 0.01      // conversion factor between centimeters and meters
#define MSEC_TO_SEC  0.001     // conversion factor between milliseconds and seconds
#define MIN_TO_SEC   60        // conversion factor between minutes and seconds
#define CIRCLE_DEG   360       // degrees in a circle [deg]
#define PI           3.141592
#define DEBUG        0

using namespace std;
using namespace chai3d;

void _expThread(void *arg)
{
    ((expWidget*)arg)->expThread();
}

expWidget::expWidget(QWidget *parent) :
    QGLWidget(parent)
{
    m_running = false;
    m_snap.p_running = m_running;  // in case of failure prior to experiment beginning

    // create timers/clocks prior to starting experiment
    m_timer = new QBasicTimer;
    m_clk = new cPrecisionClock;
    m_cntdwn = new cPrecisionClock;
    m_dataClk = new cPrecisionClock;

    // initialize input/output parameters
    m_keyPressed = false;
    m_mousePressed = false;
    m_isUpDown = true;
    m_stepsScrolled = 0;
    m_outputFile = NULL;
    setFocusPolicy(Qt::StrongFocus);  // keyboard/mouse input processed by THIS widget

    // create new CHAI world
    m_world = new cWorld();
    m_world->m_backgroundColor.setWhiteSmoke();

    // create camera to view world
    m_camera = new cCamera(m_world);
    m_camera->setClippingPlanes(0.01,20.0);
    m_camera->setOrthographicView(W_SCREEN);
    if (MIRROR) m_camera->setMirrorHorizontal(true);
    m_world->addChild(m_camera);

    // attach light source to camera for illumination
    m_light = new cDirectionalLight(m_world);
    m_light->setEnabled(true);
    m_camera->addChild(m_light);

    // add graphic elements for experiment, scaled relative to exoskeleton joints/segments
    m_angle = new cShapeCylinder(L_RELRAD*R_SEGMENT, L_RELRAD*R_SEGMENT, H_INF);
    m_target = new cShapeSphere(L_RELRAD*R_JOINT);
    m_angle->m_material->setGreenLime();
    m_target->m_material->setGreenLime();
    m_world->addChild(m_angle);
    m_world->addChild(m_target);

    // add exoskeleton segments as (slightly) transparent cylinders
    m_upperarm = new cShapeCylinder(R_SEGMENT, R_SEGMENT, H_SEGMENT);
    m_forearm = new cShapeCylinder(R_SEGMENT, R_SEGMENT, H_SEGMENT);
    m_upperarmL = new cShapeCylinder(R_SEGMENT, R_SEGMENT, H_INF);
    m_forearmL = new cShapeCylinder(R_SEGMENT, R_SEGMENT, H_INF);
    m_upperarm->m_material->setGrayLight();
    m_forearm->m_material->setGrayLight();
    m_upperarmL->m_material->setGrayLight();
    m_forearmL->m_material->setGrayLight();
    m_upperarm->setTransparencyLevel((float)ALPH_GHOST);
    m_forearm->setTransparencyLevel((float)ALPH_GHOST);
    m_upperarmL->setTransparencyLevel((float)ALPH_GHOST);
    m_forearmL->setTransparencyLevel((float)ALPH_GHOST);
    m_world->addChild(m_upperarm);
    m_world->addChild(m_forearm);
    m_world->addChild(m_upperarmL);
    m_world->addChild(m_forearmL);

    // add exoskeleton joints as (slightly) transparent spheres
    m_shoulder = new cShapeSphere(R_JOINT);
    m_elbow = new cShapeSphere(R_JOINT);
    m_hand = new cShapeSphere(R_JOINT);
    m_shoulder->m_material->setBlack();
    m_elbow->m_material->setBlack();
    m_hand->m_material->setBlack();
    m_shoulder->setTransparencyLevel((float)ALPH_GHOST);
    m_elbow->setTransparencyLevel((float)ALPH_GHOST);
    m_hand->setTransparencyLevel((float)ALPH_GHOST);
    m_world->addChild(m_shoulder);
    m_world->addChild(m_elbow);
    m_world->addChild(m_hand);

    // add labels to background
    cFont* fontSm = NEW_CFONTCALIBRI20();
    cFont* fontMd = NEW_CFONTCALIBRI40();
    cFont* fontLg = NEW_CFONTCALIBRI72();
    cFont* fontHg = NEW_CFONTCALIBRI144();
    m_instruct = new cLabel(fontLg);
    m_note = new cLabel(fontSm);
    m_remind = new cLabel(fontLg);
    m_labelPractice = new cLabel(fontHg);
    m_labelCntdwn = new cLabel(fontHg);
    m_labelTestType = new cLabel(fontSm);
    m_labelJoint = new cLabel(fontSm);
    m_labelActive = new cLabel(fontSm);
    m_labelVision = new cLabel(fontSm);
    m_labelNumJudge = new cLabel(fontSm);
    m_labelNumRev = new cLabel(fontSm);
    m_labelNumStair = new cLabel(fontSm);
    m_labelNumTrial = new cLabel(fontSm);
    m_labelCntdwn->setFontScale(2.0);  // 2x bigger
    m_instruct->m_fontColor.setBlack();
    m_note->m_fontColor.setGray();
    m_remind->m_fontColor.setRed();
    m_labelPractice->m_fontColor.setRed();
    m_labelCntdwn->m_fontColor.setRed();
    m_labelTestType->m_fontColor.setBlack();
    m_labelJoint->m_fontColor.setBlack();
    m_labelActive->m_fontColor.setBlack();
    m_labelVision->m_fontColor.setBlack();
    m_labelNumJudge->m_fontColor.setBlack();
    m_labelNumRev->m_fontColor.setBlack();
    m_labelNumStair->m_fontColor.setBlack();
    m_labelNumTrial->m_fontColor.setBlack();
    m_camera->m_backLayer->addChild(m_instruct);
    m_camera->m_backLayer->addChild(m_note);
    m_camera->m_backLayer->addChild(m_remind);
    m_camera->m_backLayer->addChild(m_labelPractice);
    m_camera->m_backLayer->addChild(m_labelCntdwn);
    m_camera->m_backLayer->addChild(m_labelTestType);
    m_camera->m_backLayer->addChild(m_labelJoint);
    m_camera->m_backLayer->addChild(m_labelActive);
    m_camera->m_backLayer->addChild(m_labelVision);
    m_camera->m_backLayer->addChild(m_labelNumJudge);
    m_camera->m_backLayer->addChild(m_labelNumRev);
    m_camera->m_backLayer->addChild(m_labelNumStair);
    m_camera->m_backLayer->addChild(m_labelNumTrial);
}

expWidget::~expWidget()
{
    // delete timers
    delete m_timer;
    delete m_clk;
    delete m_cntdwn;
    delete m_dataClk;

    // delete CHAI world
    delete m_world;
}

void expWidget::start()
{
    // turn off exoskeleton control for safety
    m_parent->m_parent->m_exo->setCtrl(none);

    // initialize experiment parameters
    initExperiment();

    // start graphics and experiment threads
    m_timer->start(T_GRAPHICS, this);
    m_thread.start(_expThread, CTHREAD_PRIORITY_GRAPHICS, this);
}

void expWidget::stop()
{
    // stop the experiment thread
    m_running = false;

    // disable exoskeleton control once experiment thread is done with it
    // NOTE: haptics thread is still running so no need to send a command to the exo
    m_runLock.acquire();
    m_parent->m_parent->m_exo->setCtrl(none);
    m_runLock.release();

    // stop graphic rendering
    m_timer->stop();

    // record last bit of data & close file
    recordData();
    if (m_outputFile != NULL)  fclose(m_outputFile);
}

void* expWidget::expThread()
{
    m_runLock.acquire();
    m_running = true;

    while (m_running) updateExperiment();

    m_running = false;
    m_runLock.release();
    return(NULL);
}


void expWidget::initExperiment()
{
    // initialize experiment state variables
    m_state = welcome;
    m_trialComplete = 0;
    m_timeOut = false;
    m_testComplete = false;
    m_expComplete = false;
    m_lockSignal = false;

    // personalize experiment for subject
    prepForSubj();
    createTests();

    // (attempt to) open subject's data file for writing
    string ID = m_parent->m_parent->m_exo->m_subj->m_ID;
    sprintf(filename, "subj_%s.csv", ID.c_str());
    m_outputFile = fopen(filename, "a");  // append
    if (m_outputFile != NULL) {

        // record subject and experiment parameters
        recordSubjParams();
        recordExpParams();

        // set & start time for data recording
        m_dataClk->setTimeoutPeriodSeconds(T_RECORD*MSEC_TO_SEC);
        m_dataClk->start(true);

    } else {

        // alert user to file-opening failure
        QMessageBox msgBox;
        msgBox.setText("The file failed to open for writing.");
        msgBox.setInformativeText("Do you want to continue?");
        msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
        msgBox.setDefaultButton(QMessageBox::No);
        int ret = msgBox.exec();
        bool abortExp;
        switch (ret) {
        case QMessageBox::Yes:
            abortExp = false;
            break;
        case QMessageBox::No:
            abortExp = true;
            break;
        default:
            abortExp = true;
            break;
        }
        if (abortExp) {
            m_parent->m_parent->m_exp->close();
            return;
        }
    }

    // prepare for first test and first trial
    updateTrialParams();
}

void expWidget::prepForSubj()
{
    // set graphical scaling parameters (correcting for parallax, if necessary)
    if (CORR_PARALL) {
        m_scalePoint = cVector3d(0.508*m_parent->m_parent->m_exo->m_subj->m_Llower,D_TO_EYES,0.0)*L_SCALE;
        m_scaleFactor = (1.0 - D_PARALLAX/(0.717*m_parent->m_parent->m_exo->m_subj->m_Llower))*L_SCALE;
    } else {
        m_scalePoint = cVector3d(0.0,0.0,0.0);
        m_scaleFactor = L_SCALE;
    }

    // scale graphics to match subject's arm
    m_shldOff = cVector3d(0.0,0.0,0.0);
    m_upperarm->setHeight(m_parent->m_parent->m_exo->m_subj->m_Lupper*m_scaleFactor);
    m_forearm->setHeight(m_parent->m_parent->m_exo->m_subj->m_LtoEE*m_scaleFactor);

    // adjust for subject handedness
    if (m_parent->m_parent->m_exo->m_subj->m_rightHanded) {
        m_center = cVector3d(CENTER_X,CENTER_Y,0.0);
        m_camX = CAM_X_RIGHT;
    } else {
        m_center = cVector3d(-1*CENTER_X,CENTER_Y,0.0);
        double d_btwn = W_TV - 2*W_EXO - D_TV_EXO_R - D_TV_EXO_L;
        m_camX = CAM_X_RIGHT + d_btwn;  // shift by distance between main axis in right- and left-handed configurations
    }
    m_groundPos = m_center;
    m_groundAng = m_parent->m_parent->m_exo->inverseKin(m_groundPos*CM_TO_METERS)*(180/PI);

    // set up camera for graphics viewing
    m_camPos = cVector3d(m_camX,CAM_Y,CAM_Z);
    m_camLookAt = m_camPos - cVector3d(0.0,0.0,CAM_Z);  // directly below camera
    m_camUp = cVector3d(0.0,1.0,0.0);
    m_camera->set(m_camPos, m_camLookAt, m_camUp);
}

void expWidget::createTests()
{
    random_device rnd;

    // finish defining (non-scalar) parameters necessary for creating tests
    // NOTE: scalar parameters defined via '#define' above
    double angStep = (double)CIRCLE_DEG/NUM_REACH;
    for (int i = 0; i < NUM_REACH; i++) m_targPos[i] = angStep*i;  // need all targets defined
    for (int i = 0; i < NUM_REACH; i++) {
        vector<double> v = getStartTargs(m_targPos[i]);
        copy(v.begin(), v.end(), begin(m_startPos[i]));
    }

    // create vectors for shuffling order of test parameters
    int nAngs[NUM_ANG];   for (int i = 0; i < NUM_ANG; i++)   { nAngs[i] = i; }
    int nPos[NUM_REACH];  for (int i = 0; i < NUM_REACH; i++) { nPos[i] = i; }

    // staircase tests
    // NOTE: can only plan first trial for each staircase since parameters
    // ----  are both random and a function of subject responses
    for (int i = 0; i < NUM_JNT; i++) {
        shuffle(begin(nAngs), end(nAngs), rnd);

        // create & push practice staircase (1 for each joint)
        trial_params practice;
        int r = nAngs[randInRange(0,NUM_ANG-1)];
        practice.p_targ = m_targAngs[i][r];
        practice.p_ref = practice.p_targ + randSign()*START_DEV;
        practice.p_isPractice = true;
        m_firstStairTrls[i].push(practice);

        // push non-practice staircases
        for (int j = 0; j < NUM_ANG; j++) {
            trial_params trial;
            trial.p_targ = m_targAngs[i][nAngs[j]];
            trial.p_ref = trial.p_targ + randSign()*START_DEV;
            m_firstStairTrls[i].push(trial);
        }
    }

    // 1-D matching tests
    for (int i = 0; i < NUM_JNT; i++) {
        shuffle(begin(nAngs), end(nAngs), rnd);

        // shuffle order of starting offsets & add practice trials (1 for each target angle)
        for (int j = 0; j < NUM_ANG; j++) {
            shuffle(begin(m_startOff[i][j]), end(m_startOff[i][j]), rnd);

            trial_params practice;
            practice.p_targ = m_targAngs[i][nAngs[j]];
            int r = randInRange(0,NUM_MATCH-1);
            practice.p_ref = practice.p_targ + m_startOff[i][nAngs[j]][r];
            practice.p_isPractice = true;
            m_1DMatchTrls_NV[i].push(practice);
            m_1DMatchTrls_V[i].push(practice);
        }

        // push non-practice NO VISION trials
        for (int k = 0; k < NUM_MATCH; k++) {
            shuffle(begin(nAngs), end(nAngs), rnd);
            for (int j = 0; j < NUM_ANG; j++) {
                trial_params trial;
                trial.p_targ = m_targAngs[i][nAngs[j]];
                trial.p_ref = trial.p_targ + m_startOff[i][nAngs[j]][k];
                m_1DMatchTrls_NV[i].push(trial);
            }
        }

        // push non-practice VISION trials
        for (int k = 0; k < NUM_VISION; k++) {
            shuffle(begin(nAngs), end(nAngs), rnd);
            for (int j = 0; j < NUM_ANG; j++) {
                trial_params trial;
                trial.p_targ = m_targAngs[i][nAngs[j]];
                int r = randInRange(0,NUM_MATCH-1);
                trial.p_ref = trial.p_targ + m_startOff[i][nAngs[j]][r];
                m_1DMatchTrls_V[i].push(trial);
            }
        }
    }

    // 2-D matching tests
    shuffle(begin(nPos), end(nPos), rnd);

    // shuffle order of starting positions & add practice trials (1 for each target position)
    for (int i = 0; i < NUM_REACH; i++) {
        shuffle(begin(m_startPos[i]), end(m_startPos[i]), rnd);

        trial_params practice;
        practice.p_targ = m_targPos[nPos[i]];
        if (CENTEROUT) {
            practice.p_ref = -1;  // negative indicates start from center
        } else {
            int r = randInRange(0,(NUM_REACH-3)-1);
            practice.p_ref = m_startPos[nPos[i]][r];
        }
        practice.p_isPractice = true;
        m_2DMatchTrls_NV.push(practice);
        m_2DMatchTrls_V.push(practice);
    }

    // push non-practice NO VISION trials
    if (CENTEROUT)  m_numMatch = NUM_MATCH;
    else            m_numMatch = NUM_REACH-3;  // one for each non-adjacent target
    for (int j = 0; j < m_numMatch; j++) {
        shuffle(begin(nPos), end(nPos), rnd);
        for (int i = 0; i < NUM_REACH; i++) {
            trial_params trial;
            trial.p_targ = m_targPos[nPos[i]];
            if (CENTEROUT)  trial.p_ref = -1;
            else            trial.p_ref = m_startPos[nPos[i]][j];
            m_2DMatchTrls_NV.push(trial);
        }
    }

    // push non-practice VISION trials
    for (int j = 0; j < NUM_VISION; j++) {
        shuffle(begin(nPos), end(nPos), rnd);
        for (int i = 0; i < NUM_REACH; i++) {
            trial_params trial;
            trial.p_targ = m_targPos[nPos[i]];
            if (CENTEROUT)  trial.p_ref = -1;
            else            trial.p_ref = m_startPos[nPos[i]][j];
            m_2DMatchTrls_V.push(trial);
        }
    }
}


void expWidget::updateExperiment()
{
    // update graphics
    updateGraphics();
    updateLabels();

    // update experiment snapshot (for debugging)
    m_snap.p_running = m_running;
    m_snap.p_test = m_test;
    m_snap.p_trial = m_trial;

    // enter finite state machine
    switch (m_state) {

    case welcome:

        // wait for SPACE keypress
        if (m_keyPressed && m_key == Qt::Key_Space) {
            m_keyPressed = false;
            m_state = locking;
        }
        break;

    case locking:

        // only pause here if necessary to (manually) lock or unlock joint(s)
        if (m_lockSignal) {
            if (m_keyPressed && m_key == Qt::Key_Space) {
                m_keyPressed = false;
                m_lockSignal = false;
            }
        } else {
            sendToGround();
            m_clk->setTimeoutPeriodSeconds(WAIT_TIME);
            m_clk->start(true);
            m_state = resetting;
        }
        break;

    case resetting:

        // wait until exo reaches position for visual grounding
        if (m_clk->timeoutOccurred() && m_parent->m_parent->m_exo->reachedTarg()) {
            m_clk->setTimeoutPeriodSeconds(GROUND_TIME);
            m_clk->start(true);
            m_state = grounding;
        }

        // skip upcoming trial
        else if (m_keyPressed && m_key == Qt::Key_S) {
            m_keyPressed = false;
            m_state = prepForNextTrial();
        }
        break;

    case grounding:

        // wait for timer to expire until sending to start for next trial
        if (m_clk->timeoutOccurred()) {
            sendToStart();
            m_state = setting;
        }

        // skip upcoming trial
        else if (m_keyPressed && m_key == Qt::Key_S) {
            m_keyPressed = false;
            m_state = prepForNextTrial();
        }
        break;

    case setting:

        // wait until exo reaches start position
        if (m_parent->m_parent->m_exo->reachedTarg()) {
            startTrialControl();
            m_cntdwn->setTimeoutPeriodSeconds(m_test.p_time);
            m_cntdwn->start(true);  // reset from 0.0 sec
            m_state = waitingForResponse;
        }

        // skip upcoming trial
        else if (m_keyPressed && m_key == Qt::Key_S) {
            m_keyPressed = false;
            m_state = prepForNextTrial();
        }
        break;

    case waitingForResponse:

        // check for (and record) response and prep for next trial
        checkForResp();
        if (m_trialComplete || m_timeOut) {
            recordData();
            m_trialComplete = 0;
            m_timeOut = false;
            m_state = prepForNextTrial();
        }
        break;

    case breaking:

        // wait until break is over (skip with 'S')
        if (m_clk->timeoutOccurred() || (m_keyPressed && m_key == Qt::Key_S)) {
            m_keyPressed = false;
            m_state = locking;
        }
        break;

    case thanks:
        break;

    default:
        break;
    }
}

void expWidget::updateGraphics()
{
    // get subject kinematics
    double L1 = m_parent->m_parent->m_exo->m_subj->m_Lupper*m_scaleFactor;
    double L2 = m_parent->m_parent->m_exo->m_subj->m_LtoEE*m_scaleFactor;
    double th1 = m_parent->m_parent->m_exo->m_th(0);
    double th2 = th1 + m_parent->m_parent->m_exo->m_th(1);
    if (!m_parent->m_parent->m_exo->m_subj->m_rightHanded) {
        th1 = PI - th1;
        th2 = PI - th2;
    }

    // update exoskeleton to match subject
    m_shoulder->setLocalPos(m_scaleFactor*(m_shldOff - m_scalePoint/L_SCALE) + m_scalePoint);
    m_elbow->setLocalPos(m_shoulder->getLocalPos() + cVector3d(L1*cos(th1), L1*sin(th1), 0.0));
    m_hand->setLocalPos(m_elbow->getLocalPos() + cVector3d(L2*cos(th2), L2*sin(th2), 0.0));
    m_upperarm->setLocalPos(m_shoulder->getLocalPos());
    m_forearm->setLocalPos(m_elbow->getLocalPos());
    m_upperarmL->setLocalPos(m_shoulder->getLocalPos());
    m_forearmL->setLocalPos(m_elbow->getLocalPos());
    m_upperarm->setLocalRot(cMatrix3d(th1, PI/2, 0.0, C_EULER_ORDER_ZYX));
    m_forearm->setLocalRot(cMatrix3d(th2, PI/2, 0.0, C_EULER_ORDER_ZYX));
    m_upperarmL->setLocalRot(cMatrix3d(th1, PI/2, 0.0, C_EULER_ORDER_ZYX));
    m_forearmL->setLocalRot(cMatrix3d(th2, PI/2, 0.0, C_EULER_ORDER_ZYX));

    switch (m_state) {

    case welcome:
    case locking:
    case resetting:
    case setting:
    case breaking:
    case thanks:

        // only show exo for debugging purposes
        if (DEBUG) {
            m_shoulder->setShowEnabled(true);
            m_elbow->setShowEnabled(true);
            m_hand->setShowEnabled(true);
            m_upperarm->setShowEnabled(true);
            m_forearm->setShowEnabled(true);
        } else {
            m_shoulder->setShowEnabled(false);
            m_elbow->setShowEnabled(false);
            m_hand->setShowEnabled(false);
            m_upperarm->setShowEnabled(false);
            m_forearm->setShowEnabled(false);
        }
        m_upperarmL->setShowEnabled(false);
        m_forearmL->setShowEnabled(false);
        m_angle->setShowEnabled(false);
        m_target->setShowEnabled(false);
        break;

    case grounding:

        // show current exo configuration to ground proprioception with vision
        switch (m_test.p_type) {
        case staircase:
        case match1D:
            if (m_test.p_joint == S) {
                m_shoulder->setShowEnabled(true);
                m_upperarmL->setShowEnabled(true);
                m_elbow->setShowEnabled(false);
                m_forearmL->setShowEnabled(false);
            } else {
                m_elbow->setShowEnabled(true);
                m_forearmL->setShowEnabled(true);
                m_shoulder->setShowEnabled(false);
                m_upperarmL->setShowEnabled(false);
            }
            m_upperarm->setShowEnabled(false);
            m_forearm->setShowEnabled(false);
            m_hand->setShowEnabled(false);
            break;
        case match2D:
            m_hand->setShowEnabled(true);
            m_shoulder->setShowEnabled(false);
            m_elbow->setShowEnabled(false);
            m_upperarm->setShowEnabled(false);
            m_forearm->setShowEnabled(false);
            m_upperarmL->setShowEnabled(false);
            m_forearmL->setShowEnabled(false);
            break;
        default:
            m_shoulder->setShowEnabled(true);
            m_elbow->setShowEnabled(true);
            m_hand->setShowEnabled(true);
            m_upperarm->setShowEnabled(true);
            m_forearm->setShowEnabled(true);
            m_upperarmL->setShowEnabled(false);
            m_forearmL->setShowEnabled(false);
            break;
        }
        m_angle->setShowEnabled(false);
        m_target->setShowEnabled(false);
        break;

    case waitingForResponse:

        switch (m_test.p_type) {

        // show reference angle
        case staircase:
            double refAng;
            if (m_test.p_joint == S) {
                refAng = m_trial.p_ref*(PI/180);
                m_angle->setLocalPos(m_shoulder->getLocalPos());
                m_shoulder->setShowEnabled(true);
                m_elbow->setShowEnabled(false);
            } else {
                if (m_parent->m_parent->m_exo->m_subj->m_rightHanded) refAng = th1 + m_trial.p_ref*(PI/180);
                else                                                  refAng = (PI-th1) + m_trial.p_ref*(PI/180);
                m_angle->setLocalPos(m_elbow->getLocalPos());
                m_elbow->setShowEnabled(true);
                m_shoulder->setShowEnabled(false);
            }
            if (!m_parent->m_parent->m_exo->m_subj->m_rightHanded) refAng = PI - refAng;
            m_angle->setLocalRot(cMatrix3d(refAng, PI/2, 0.0, C_EULER_ORDER_ZYX));
            m_angle->setShowEnabled(true);
            m_upperarm->setShowEnabled(false);
            m_forearm->setShowEnabled(false);
            m_upperarmL->setShowEnabled(false);
            m_forearmL->setShowEnabled(false);
            m_hand->setShowEnabled(false);
            m_target->setShowEnabled(false);
            break;

        case match1D:

            // show goal joint/segment position
            if (m_test.p_active) {
                double targAng;
                if (m_test.p_joint == S) {
                    targAng = m_trial.p_targ*(PI/180);
                    m_angle->setLocalPos(m_shoulder->getLocalPos());
                    m_shoulder->setShowEnabled(true);
                    m_elbow->setShowEnabled(false);
                    m_forearmL->setShowEnabled(false);
                    if (m_test.p_vision) m_upperarmL->setShowEnabled(true);
                    else                 m_upperarmL->setShowEnabled(false);
                } else {
                    if (m_parent->m_parent->m_exo->m_subj->m_rightHanded) targAng = th1 + m_trial.p_targ*(PI/180);
                    else                                                  targAng = (PI-th1) + m_trial.p_targ*(PI/180);
                    m_angle->setLocalPos(m_elbow->getLocalPos());
                    m_elbow->setShowEnabled(true);
                    m_shoulder->setShowEnabled(false);
                    m_upperarmL->setShowEnabled(false);
                    if (m_test.p_vision) m_forearmL->setShowEnabled(true);
                    else                 m_forearmL->setShowEnabled(false);
                }
                if (!m_parent->m_parent->m_exo->m_subj->m_rightHanded) targAng = PI - targAng;
                m_angle->setLocalRot(cMatrix3d(targAng, PI/2, 0.0, C_EULER_ORDER_ZYX));
            }

            // show angle cursor
            else {
                double subjAng;
                if (m_test.p_joint == S) {
                    subjAng = m_subjAng*(PI/180);
                    m_angle->setLocalPos(m_shoulder->getLocalPos());
                    m_shoulder->setShowEnabled(true);
                    m_elbow->setShowEnabled(false);
                } else {
                    if (m_parent->m_parent->m_exo->m_subj->m_rightHanded) subjAng = th1 + m_subjAng*(PI/180);
                    else                                                  subjAng = (PI-th1) + m_subjAng*(PI/180);
                    m_angle->setLocalPos(m_elbow->getLocalPos());
                    m_elbow->setShowEnabled(true);
                    m_shoulder->setShowEnabled(false);
                }
                if (!m_parent->m_parent->m_exo->m_subj->m_rightHanded) subjAng = PI - subjAng;
                m_angle->setLocalRot(cMatrix3d(subjAng, PI/2, 0.0, C_EULER_ORDER_ZYX));
                m_upperarmL->setShowEnabled(false);
                m_forearmL->setShowEnabled(false);
            }
            m_angle->setShowEnabled(true);
            m_upperarm->setShowEnabled(false);
            m_forearm->setShowEnabled(false);
            m_hand->setShowEnabled(false);
            m_target->setShowEnabled(false);
            break;

        case match2D:

            // show goal hand position
            if (m_test.p_active) {
                m_target->setLocalPos(m_scaleFactor*(angleToTarg(m_trial.p_targ)*CM_TO_METERS - m_scalePoint/L_SCALE) + m_scalePoint);
                if (m_test.p_vision) m_hand->setShowEnabled(true);
                else                 m_hand->setShowEnabled(false);
            }

            // show position cursor
            else {
                m_target->setLocalPos(m_scaleFactor*(m_subjPos*CM_TO_METERS - m_scalePoint/L_SCALE) + m_scalePoint);
                m_hand->setShowEnabled(false);
            }
            m_target->setShowEnabled(true);
            m_shoulder->setShowEnabled(false);
            m_elbow->setShowEnabled(false);
            m_upperarm->setShowEnabled(false);
            m_forearm->setShowEnabled(false);
            m_upperarmL->setShowEnabled(false);
            m_forearmL->setShowEnabled(false);
            m_angle->setShowEnabled(false);
            break;

        default:
            break;
        }
        break;

    default:
        m_shoulder->setShowEnabled(false);
        m_elbow->setShowEnabled(false);
        m_hand->setShowEnabled(false);
        m_upperarm->setShowEnabled(false);
        m_forearm->setShowEnabled(false);
        m_upperarmL->setShowEnabled(false);
        m_forearmL->setShowEnabled(false);
        m_angle->setShowEnabled(false);
        m_target->setShowEnabled(false);
        break;
    }
}

void expWidget::updateLabels()
{
    // declare variables for label positioning
    double longestWidth;
    double stndrdHeight;

    switch (m_state) {

    case welcome:

        m_instruct->setText("Welcome");
        m_instruct->setLocalPos((int)((m_width - m_instruct->getWidth())/2),
                                (int)((m_height - m_instruct->getHeight())/2),0);  // center
        m_instruct->setShowEnabled(true);
        m_note->setShowEnabled(false);
        m_remind->setShowEnabled(false);
        hideTestStateLabels();
        break;

    case locking:

        // NOTE: assume that one joint will always be unlocked
        if (m_lockSignal) {
            m_instruct->setText("Please wait for experimenter.");
            m_instruct->setLocalPos((int)((m_width - m_instruct->getWidth())/2),
                                    (int)((m_height - m_instruct->getHeight())/2),0);  // center
            m_instruct->setShowEnabled(true);

            // add note for experimenter below subject instructions
            if (m_parent->m_parent->m_exo->m_lockedJnts[S]) {
                m_note->setText("EXPERIMENTER: lock shoulder at " + to_string(m_test.p_lock) + " deg");
            } else if (m_parent->m_parent->m_exo->m_lockedJnts[E]) {
                m_note->setText("EXPERIMENTER: lock elbow at " + to_string(m_test.p_lock) + " deg");
            } else {
                m_note->setText("EXPERIMENTER: unlock both joints");
            }
            m_note->setLocalPos((int)((m_width - m_note->getWidth())/2),
                                (int)(m_height/2 - m_instruct->getHeight()),0);
            m_note->setShowEnabled(true);
        }
        m_remind->setShowEnabled(false);
        hideTestStateLabels();
        break;

    case resetting:

        m_instruct->setText("Moving arm to home position.");
        m_instruct->setLocalPos(75,(int)((m_height - m_instruct->getHeight())/2),0);
        m_instruct->setShowEnabled(true);
        m_note->setShowEnabled(false);
        m_remind->setShowEnabled(false);
        hideTestStateLabels();
        break;

    case grounding:

        if (m_test.p_type == staircase || m_test.p_type == match1D)
            if (m_test.p_joint == S) m_remind->setText("Your upperarm is here.");
            else                     m_remind->setText("Your forearm is here.");
        else                         m_remind->setText("Your hand is here.");
        m_remind->setLocalPos((int)((m_width - m_instruct->getWidth())/2),(int)(m_height-150),0);
        m_remind->setShowEnabled(true);
        m_instruct->setShowEnabled(false);
        m_note->setShowEnabled(false);
        hideTestStateLabels();
        break;

    case setting:

        m_instruct->setText("Moving arm to starting position.");
        m_instruct->setLocalPos(75,(int)(m_height/2),0);
        m_instruct->setShowEnabled(true);
        m_note->setShowEnabled(false);
        m_remind->setShowEnabled(false);
        hideTestStateLabels();
        break;

    case waitingForResponse:

        // label practice trials
        if (m_trial.p_isPractice) {
            m_labelPractice->setText("PRACTICE TRIAL");
            m_labelPractice->setLocalPos((int)((m_width - m_labelPractice->getWidth())/2),
                                         (int)((m_height - m_labelPractice->getHeight())/2),0);  // center
            m_labelPractice->setShowEnabled(true);
        } else {
            m_labelPractice->setShowEnabled(false);
        }

        // compute & (if necessary) display time remaining in trial
        m_timeRemaining = round(m_test.p_time - m_cntdwn->getCurrentTimeSeconds());
        if (m_timeRemaining < 0)  m_timeRemaining = 0;
        m_labelCntdwn->setText(to_string(m_timeRemaining));
        if (m_parent->m_parent->m_exo->m_subj->m_rightHanded)
              m_labelCntdwn->setLocalPos(150,(int)((m_height - m_labelCntdwn->getHeight())/2),0);  // on LEFT (opposite tested arm)
        else  m_labelCntdwn->setLocalPos((int)(m_width - m_labelCntdwn->getWidth() - 150),
                                         (int)((m_height - m_labelCntdwn->getHeight())/2),0);      // on RIGHT (opposite tested arm)
        m_labelCntdwn->setShowEnabled(bool(CNTDWN));

        // set test details
        switch (m_test.p_type) {
        case staircase:
            m_labelTestType->setText("TEST: 1-D forced choice");
            if (m_test.p_joint == S) m_labelJoint->setText("JOINT: shoulder");
            else                     m_labelJoint->setText("JOINT: elbow");
            m_labelActive->setText("MOVEMENT: passive");
            m_labelVision->setText("VISION: ---");
            m_labelNumStair->setText("STAIRCASE #" + to_string(m_currStaircase + 1));
            m_labelNumJudge->setText("JUDGEMENT #" + to_string(m_numJudgements[getStairIndex(m_currStaircase)] + 1));
            m_labelNumRev->setText("REVERSALS: " + to_string(m_numReversals[getStairIndex(m_currStaircase)]));
            break;
        case match1D:
            m_labelTestType->setText("TEST: 1-D free choice");
            if (m_test.p_joint == S) m_labelJoint->setText("JOINT: shoulder");
            else                     m_labelJoint->setText("JOINT: elbow");
            if (m_test.p_active) m_labelActive->setText("MOVEMENT: active");
            else                 m_labelActive->setText("MOVEMENT: passive");
            if (m_test.p_vision) m_labelVision->setText("VISION: yes");
            else                 m_labelVision->setText("VISION: ---");
            m_labelNumTrial->setText("TRIAL #" + to_string(m_numTrials + 1));
            break;
        case match2D:
            m_labelTestType->setText("TEST: 2-D matching");
            m_labelJoint->setText("JOINT: both");
            if (m_test.p_active) m_labelActive->setText("MOVEMENT: active");
            else                 m_labelActive->setText("MOVEMENT: passive");
            if (m_test.p_vision) m_labelVision->setText("VISION: yes");
            else                 m_labelVision->setText("VISION: ---");
            m_labelNumTrial->setText("TRIAL #" + to_string(m_numTrials + 1));
            break;
        default:
            break;
        }

        // position/display test details
        longestWidth = m_labelTestType->getWidth() + 8;
        stndrdHeight = m_labelTestType->getHeight() + 8;
        m_labelTestType->setLocalPos((int)(m_width-longestWidth-10),(int)(m_height-stndrdHeight),0);
        m_labelJoint->setLocalPos((int)(m_width-longestWidth-10),(int)(m_height-2*stndrdHeight),0);
        m_labelActive->setLocalPos((int)(m_width-longestWidth-10),(int)(m_height-3*stndrdHeight),0);
        m_labelVision->setLocalPos((int)(m_width-longestWidth-10),(int)(m_height-4*stndrdHeight),0);
        m_labelNumStair->setLocalPos(10,(int)(m_height-stndrdHeight),0);
        m_labelNumJudge->setLocalPos(10,(int)(m_height-2*stndrdHeight),0);
        m_labelNumRev->setLocalPos(10,(int)(m_height-3*stndrdHeight),0);
        m_labelNumTrial->setLocalPos(10,(int)(m_height-stndrdHeight),0);

        m_labelTestType->setShowEnabled(true);
        m_labelJoint->setShowEnabled(true);
        m_labelActive->setShowEnabled(true);
        m_labelVision->setShowEnabled(true);
        if (m_test.p_type == staircase) {
            m_labelNumStair->setShowEnabled(true);
            m_labelNumJudge->setShowEnabled(true);
            m_labelNumRev->setShowEnabled(true);
            m_labelNumTrial->setShowEnabled(false);
        } else {
            m_labelNumStair->setShowEnabled(false);
            m_labelNumJudge->setShowEnabled(false);
            m_labelNumRev->setShowEnabled(false);
            m_labelNumTrial->setShowEnabled(true);
        }

        // set reminder of test instructions
        switch (m_test.p_type) {
        case staircase:
            if (m_test.p_joint == S) m_remind->setText("Where is the green cylinder relative to your upperarm?");
            else                     m_remind->setText("Where is the green cylinder relative to your forearm?");
            break;
        case match1D:
            if (m_test.p_active) {
                if (m_test.p_joint == S) m_remind->setText("Move your upperarm to match the green cylinder.");
                else                     m_remind->setText("Move your forearm to match the green cylinder.");
            } else {
                if (m_test.p_joint == S) m_remind->setText("Move the green cylinder to match your upperarm.");
                else                     m_remind->setText("Move the green cylinder to match your forearm.");
            }
            break;
        case match2D:
            if (m_test.p_active) m_remind->setText("Move your hand to the green sphere.");
            else                 m_remind->setText("Move the green sphere to match your hand position.");
            break;
        default:
            break;
        }
        m_remind->setLocalPos((int)((m_width - m_remind->getWidth())/2),(int)(m_height-150),0);
        m_remind->setShowEnabled(true);

        m_instruct->setShowEnabled(false);
        m_note->setShowEnabled(false);
        break;

    case breaking:

        m_instruct->setText("Take a " + to_string(m_breakTime) + "-minute break.");
        m_instruct->setLocalPos((int)((m_width - m_instruct->getWidth())/2),
                                (int)((m_height - m_instruct->getHeight())/2),0);
        m_instruct->setShowEnabled(true);
        m_note->setShowEnabled(false);
        m_remind->setShowEnabled(false);
        hideTestStateLabels();
        break;

    case thanks:

        m_instruct->setText("Experiment complete.");
        m_instruct->setLocalPos((int)((m_width - m_instruct->getWidth())/2),
                                (int)((m_height - m_instruct->getHeight())/2),0);
        m_instruct->setShowEnabled(true);
        m_note->setShowEnabled(false);
        m_remind->setShowEnabled(false);
        hideTestStateLabels();
        break;

    default:
        break;
    }
}

void expWidget::updateTestParams()
{
    // check for end of experiment
    if (m_parent->m_testQueue.empty()) {
        m_expComplete = true;
        return;
    }

    // pull next set of test parameters
    m_testPrev = m_test;
    m_test = m_parent->m_testQueue.front();
    m_parent->m_testQueue.pop();

    // update queue of trials
    switch (m_test.p_type) {
    case staircase:
        m_trialsCurr = m_firstStairTrls[m_test.p_joint];
        break;
    case match1D:
        if (m_test.p_vision) m_trialsCurr = m_1DMatchTrls_V[m_test.p_joint];
        else                 m_trialsCurr = m_1DMatchTrls_NV[m_test.p_joint];
        break;
    case match2D:
        if (m_test.p_vision) m_trialsCurr = m_2DMatchTrls_V;
        else                 m_trialsCurr = m_2DMatchTrls_NV;
        break;
    default:
        break;
    }
}

void expWidget::updateTrialParams()
{
    // check for end of test (or beginning of experiment)
    if (m_trialsCurr.empty()) {
        m_testComplete = true;
        updateTestParams();
    }

    // unless end of experiment, pull next set of trial parameters
    if (!m_expComplete) {
        m_trialPrev = m_trial;
        m_trial = m_trialsCurr.front();
        m_trialsCurr.pop();
        if (m_testComplete) prepForTest();
    }

    // no need for break at beginning of experiment
    if (m_state == welcome) m_testComplete = false;
}

void expWidget::prepForTest()
{
    // initialize parameters for new test
    switch (m_test.p_type) {
    case staircase:
        m_stairStatus = 2;  // testing new joint
        resetStaircase();
        break;
    case match1D:
        m_subjAng = m_trial.p_ref;  // starting angle
        m_numTrials = 0;
    case match2D:
        if (m_trial.p_ref < 0)  m_subjPos = m_center;                    // starting from center
        else                    m_subjPos = angleToTarg(m_trial.p_ref);  // starting from circle
        m_numTrials = 0;
        break;
    default:
        break;
    }

    // check if need to manually lock unused joint
    if (MANUAL_LOCK) {
        m_lockSignal = true;
        switch (m_test.p_joint) {
        case S:
            m_parent->m_parent->m_exo->m_lockedJnts[S] = false;
            m_parent->m_parent->m_exo->m_lockedJnts[E] = true;
            break;
        case E:
            m_parent->m_parent->m_exo->m_lockedJnts[S] = true;
            m_parent->m_parent->m_exo->m_lockedJnts[E] = false;
            break;
        case both:
            m_parent->m_parent->m_exo->m_lockedJnts[S] = false;
            m_parent->m_parent->m_exo->m_lockedJnts[E] = false;
            break;
        default:
            m_parent->m_parent->m_exo->m_lockedJnts[S] = false;
            m_parent->m_parent->m_exo->m_lockedJnts[E] = false;
            break;
        }
    }

    // ready file for recording test data
    if (!m_headerWritten[m_test.p_type]) {
        writeHeaderForData();
        m_headerWritten[m_test.p_type] = true;
    }
}

exp_states expWidget::prepForNextTrial()
{
    // update test variables & trial parameters
    switch (m_test.p_type) {
    case staircase:
        updateStaircase();
        switch (m_stairStatus) {
        case 1:
            resetStaircase();
            break;
        case 2:
            updateTrialParams();   // get parameters for first trial of coming test
            if (!m_testComplete)   // moving to a new angle for the current joint
                resetStaircase();  // reset is done automatically when switching to new joint
            break;
        default:
            break;
        }
        break;
    case match1D:
        updateTrialParams();
        m_numTrials++;
        if (!m_testComplete)
            m_subjAng = m_trial.p_ref;
        break;
    case match2D:
        updateTrialParams();
        m_numTrials++;
        if (!m_testComplete)
            if (m_trial.p_ref < 0)  m_subjPos = m_center;                    // starting from center
            else                    m_subjPos = angleToTarg(m_trial.p_ref);  // starting from circle
        break;
    default:
        break;
    }

    // assign next experiment state (default = 'breaking')
    if (m_expComplete) return thanks;

    switch (m_test.p_type) {
    case staircase:

        // continuing with current (double) staircase
        if (!m_stairStatus) {
            m_cntdwn->setTimeoutPeriodSeconds(m_test.p_time);
            m_cntdwn->start(true);
            return waitingForResponse;
        }

        // visually ground (but no break) between staircases and joint angles
        else if (m_stairStatus == 1 || (m_stairStatus == 2 && !m_testComplete))
            return locking;

        // break between joints
        else {
            m_breakTime = SHORT_BREAK;
            m_testComplete = false;
        }
        break;

    case match1D:

        if (m_testComplete) {
            if (m_test.p_type != m_testPrev.p_type ||
                m_test.p_active != m_testPrev.p_active ||
                m_test.p_vision != m_testPrev.p_vision)
                    m_breakTime = LONG_BREAK;
            else    m_breakTime = SHORT_BREAK;  // between joints
            m_testComplete = false;
        } else  return locking;
        break;

    case match2D:

        if (m_testComplete) {
            m_breakTime = LONG_BREAK;
            m_testComplete = false;
        } else if (!m_test.p_vision &&                                   // testing without visual feedback
                   m_numTrials == NUM_REACH + m_numMatch*NUM_REACH/2) {  // & halfway done (after practice)
            m_breakTime = SHORT_BREAK;
        } else  return locking;
        break;

    default:
        break;
    }

    // by default, send to break with time set above
    m_clk->setTimeoutPeriodSeconds(m_breakTime*MIN_TO_SEC);
    m_clk->start(true);
    return breaking;
}

void expWidget::resetStaircase()
{
    // reset parameters independent of staircase status
    for (int i = 0; i < 2; i++) {
        m_numJudgements[i] = 0;
        m_oneCorrect[i] = true;
    }

    // reset parameters for new SET of staircases (i.e., beginning to test new angle/joint)
    if (m_stairStatus == 2) {
        m_numStaircases = 0;
        if (ADAPTIVE) m_maxStaircases = 6;
        else          m_maxStaircases = 4;
        for (int i = 0; i < 2; i++) m_refStep[i] = 10;
        if (DOUB_STAIRS) {
            if (m_trial.p_isPractice) m_maxStaircases = 2;             // 2 = 1 double-staircase
            m_currStaircase = (m_trial.p_ref < m_trial.p_targ);        // either 0 (starting CLOSER to body) or 1
            m_refStartSide[CLOSER] = 1; m_refStartSide[FURTHER] = -1;  // always {starts CLOSER, starts FURTHER}
            m_refAng[m_currStaircase] = m_trial.p_ref;
            m_refAng[!m_currStaircase] = m_trial.p_targ + m_refStartSide[!m_currStaircase]*START_DEV;
            for (int i = 0; i < 2; i++) {
                m_numReversals[i] = 0;
                m_refStepDir[i] = -1*m_refStartSide[i];
                m_corrResp[i] = (m_trial.p_targ >= m_refAng[i]);
            }
        } else {
            if (m_trial.p_isPractice) m_maxStaircases = 1;
            m_currStaircase = 0;
            m_numReversals[0] = 0; m_numReversals[1] = MAX_REVERSAL;  // only want to work along first dimension of array
            m_refStartSide[0] = (int)(m_trial.p_ref > m_trial.p_targ) - (int)(m_trial.p_ref < m_trial.p_targ);
            m_refStepDir[0] = -1*m_refStartSide[0];
            m_refAng[0] = m_trial.p_ref;
            m_corrResp[0] = (m_trial.p_targ >= m_refAng[0]);
        }
    }

    // reset parameters for next single or double staircase (within set for same joint angle)
    else {
        if (ADAPTIVE && m_numStaircases == 2)  estimatePerceptParams();
        if (DOUB_STAIRS) {
            m_currStaircase = m_numStaircases + (int)randBool();
            m_refStartSide[CLOSER] = 1; m_refStartSide[FURTHER] = -1;
            for (int i = 0; i < 2; i++) {
                m_numReversals[i] = 0;
                m_refStepDir[i] = -1*m_refStartSide[i];
            }
            if (ADAPTIVE && m_numStaircases >= 2) {
                for (int i = 0; i < 2; i++) {
                    m_refStep[i] = 5;
                    m_refAng[i] = m_boundary + m_refStartSide[i]*m_sensitiv;
                    m_corrResp[i] = (m_trial.p_targ >= m_refAng[i]);  // arm position stays same

                    // possibly correct start side & step direction because where reference falls
                    // relative to target is dependent on estimated perceptual parameters
                    if (m_corrResp[i] == CLOSER) {
                        m_refStartSide[i] = 1;
                        m_refStepDir[i] = -1;
                    } else {
                        m_refStartSide[i] = -1;
                        m_refStepDir[i] = 1;
                    }
                }
            } else {
                for (int i = 0; i < 2; i++) {
                    m_refStep[i] = 10;
                    m_refAng[i] = m_trial.p_targ + m_refStartSide[i]*START_DEV;
                    m_corrResp[i] = (m_trial.p_targ >= m_refAng[i]);
                }
            }
        } else {
            m_currStaircase = m_numStaircases;
            m_numReversals[0] = 0; m_numReversals[1] = MAX_REVERSAL;
            m_refStartSide[0] = -1*m_refStartSide[0];  // flip relative to previous staircase
            m_refStepDir[0] = -1*m_refStartSide[0];
            if (ADAPTIVE && m_numStaircases >= 2) {
                m_refStep[0] = 5;
                m_refAng[0] = m_boundary + m_refStartSide[0]*m_sensitiv;
                m_corrResp[0] = (m_trial.p_targ >= m_refAng[0]);
                if (m_corrResp[0] == CLOSER) {
                    m_refStartSide[0] = 1;
                    m_refStepDir[0] = -1;
                } else {
                    m_refStartSide[0] = -1;
                    m_refStepDir[0] = 1;
                }
            } else {
                m_refStep[0] = 10;
                m_refAng[0] = m_trial.p_targ + m_refStartSide[0]*START_DEV;
                m_corrResp[0] = (m_trial.p_targ >= m_refAng[0]);
            }
        }

        // set up for first trial of next single/double staircase
        m_trial.p_ref = m_refAng[getStairIndex(m_currStaircase)];
    }
}

void expWidget::updateStaircase()
{
    int i = getStairIndex(m_currStaircase);
    m_numJudgements[i]++;
    if (DEBUG)  qDebug() << "judgement #" << m_numJudgements[i];

    // save tested reference angle if staircase is "adaptive"
    if (ADAPTIVE && !m_trial.p_isPractice && m_numStaircases < 2)
        m_refAngsForEstim[m_currStaircase].push_back(m_trial.p_ref);

    // if judgement was first of current staircase (until one correct)
    if (m_numJudgements[i] == 1 || !m_oneCorrect[i]) {
        if (m_subjResp[i] != m_corrResp[i]) {
            if (DEBUG)  qDebug() << "  WRONG, moving away from arm";
            m_oneCorrect[i] = false;
            m_refStepDir[i] = m_refStartSide[i];
        } else {
            if (DEBUG)  qDebug() << "  CORRECT, moving towards arm";
            m_oneCorrect[i] = true;
            m_refStepDir[i] = -1*m_refStartSide[i];
        }
        m_stairStatus = 0;
    }

    // once a correct judgement has been made
    else {

        // if reversed judgement
        if (m_subjResp[i] != m_subjRespPrev[i]) {
            m_numReversals[i]++;
            if (DEBUG)  qDebug() << "  switched decision, reversing direction time #" << m_numReversals[i];
            m_refStep[i] = 0.5*m_refStep[i];
            m_refStepDir[i] = -1*m_refStepDir[i];
            m_stairStatus = 0;

            // if done with staircase
            if (m_numReversals[i] == MAX_REVERSAL) {
                m_numStaircases++;
                if (DEBUG)  qDebug() << "  finished staircase #" << m_numStaircases;

                // if done with BOTH staircases in double-staircase paradigm
                // NOTE: because of how test variables are initialized, this
                // ----  check also works for single-staircase paradigm
                if (m_numReversals[!i] == MAX_REVERSAL) {
                    m_stairStatus = 1;

                    // if done with set of staircases
                    if (m_numStaircases == m_maxStaircases) {
                        m_stairStatus = 2;
                    }
                }
            }
        }
    }

    // if mid-staircase, update reference angle & trial params
    if (!m_stairStatus) {
        m_refAng[i] = m_refAng[i] + m_refStepDir[i]*m_refStep[i];
        int i_next = randBool();
        if (m_numReversals[i_next] == MAX_REVERSAL) i_next = (int)(!i_next);
        if (DOUB_STAIRS) m_currStaircase = (int)floor(m_numStaircases/2)*2 + i_next;
        else             m_currStaircase = m_numStaircases + i_next;
        m_trial.p_ref = m_refAng[i_next];
    }
}


void expWidget::sendToGround()
{
    m_parent->m_parent->m_exo->setTarg(m_groundPos*CM_TO_METERS, task);
}

void expWidget::sendToStart()
{
    // set start position for upcoming test
    cVector3d start;
    switch (m_test.p_type) {
    case staircase:
        if (m_test.p_joint == S)    { start(S) = m_trial.p_targ; start(E) = m_test.p_lock; }
        else                        { start(S) = m_test.p_lock;  start(E) = m_trial.p_targ; }
        break;
    case match1D:
        if (m_test.p_active) {
            if (m_test.p_joint == S)    { start(S) = m_trial.p_ref; start(E) = m_test.p_lock; }
            else                        { start(S) = m_test.p_lock; start(E) = m_trial.p_ref; }
        } else {
            if (m_test.p_joint == S)    { start(S) = m_trial.p_targ; start(E) = m_test.p_lock; }
            else                        { start(S) = m_test.p_lock; start(E) = m_trial.p_targ; }
        }
        break;
    case match2D:
        if (m_test.p_active) {
            if (m_trial.p_ref < 0) start = m_center;
            else                   start = angleToTarg(m_trial.p_ref); }
        else                     { start = angleToTarg(m_trial.p_targ); }
        break;
    default:
        break;
    }

    // send exo to start position
    if (m_test.p_type == staircase || m_test.p_type == match1D) {
              m_parent->m_parent->m_exo->setTarg(start*(PI/180), joint);
    } else    m_parent->m_parent->m_exo->setTarg(start*CM_TO_METERS, task);
}

void expWidget::startTrialControl()
{
    // assign control method given test parameters
    ctrl_states trialCtrl;
    switch (m_test.p_joint) {
    case S:
        if (m_test.p_active)  trialCtrl = elbow;
        else                  trialCtrl = joint;
        break;
    case E:
        if (m_test.p_active)  trialCtrl = shoulder;
        else                  trialCtrl = joint;
        break;
    case both:
        if (m_test.p_active)  trialCtrl = none;
        else                  trialCtrl = task;
        break;
    default:
        break;
    }
    m_parent->m_parent->m_exo->setCtrl(trialCtrl);
}


void expWidget::recordSubjParams()
{
    // write subject parameters if file exists
    if (m_outputFile != NULL) {
        fprintf(m_outputFile, "Name: %s\n", m_parent->m_parent->m_exo->m_subj->m_name.c_str());
        fprintf(m_outputFile, "ID: %s\n",   m_parent->m_parent->m_exo->m_subj->m_ID.c_str());
        fprintf(m_outputFile, "Age: %i\n",  m_parent->m_parent->m_exo->m_subj->m_age);
        if (m_parent->m_parent->m_exo->m_subj->m_stroke) fprintf(m_outputFile, "Health: patient\n");
        else                                             fprintf(m_outputFile, "Health: control\n");
        if (m_parent->m_parent->m_exo->m_subj->m_female) fprintf(m_outputFile, "Gender: F\n");
        else                                             fprintf(m_outputFile, "Gender: M\n");
        if (m_parent->m_parent->m_exo->m_subj->m_rightHanded) fprintf(m_outputFile, "Hand: R\n");
        else                                                  fprintf(m_outputFile, "Hand: L\n");
        fprintf(m_outputFile, "Upperarm Length: %f m\n",  m_parent->m_parent->m_exo->m_subj->m_Lupper);
        fprintf(m_outputFile, "Forearm Length: %f m\n", m_parent->m_parent->m_exo->m_subj->m_LtoEE);
        jointLims lims  = m_parent->m_parent->m_exo->m_subj->m_lims;
        fprintf(m_outputFile, "Shoulder Limits: [%f, %f] deg\n", lims.shoul_min*(180/PI), lims.shoul_max*(180/PI));
        fprintf(m_outputFile, "Elbow Limits: [%f, %f] deg\n\n", lims.elbow_min*(180/PI), lims.elbow_max*(180/PI));

        // add subject-specific experiment parameters
        exp_params p = m_parent->m_parent->m_exo->m_subj->m_params;
        fprintf(m_outputFile, "First Joint Tested: %i\n", (int)p.p_firstJnt);
        fprintf(m_outputFile, "Joint Lock Angles: [%f, %f] deg\n", p.p_lockAngs[0], p.p_lockAngs[1]);
    }
}

void expWidget::recordExpParams()
{
    // write parameters for (all possible) tests if file exists
    if (m_outputFile != NULL) {
        fprintf(m_outputFile, "Parallax Correction: %i\n", (int)CORR_PARALL);
        fprintf(m_outputFile, "Double-Randomized Staircase: %i\n", (int)DOUB_STAIRS);
        fprintf(m_outputFile, "Adaptive Staircase: %i\n", (int)ADAPTIVE);
        fprintf(m_outputFile, "Grounding Position: [%f, %f] m\n", m_groundPos(0)*CM_TO_METERS, m_groundPos(1)*CM_TO_METERS);
        fprintf(m_outputFile, "Grounding Angles: [%f, %f] deg\n", m_groundAng(0), m_groundAng(1));
        fprintf(m_outputFile, "Shoulder Test Angles: [%f, %f, %f] deg\n", m_targAngs[0][0], m_targAngs[0][1], m_targAngs[0][2]);
        fprintf(m_outputFile, "Elbow Test Angles: [%f, %f, %f] deg\n", m_targAngs[1][0], m_targAngs[1][1], m_targAngs[1][2]);
        fprintf(m_outputFile, "Number of Reaches (in circle): %i\n", (int)NUM_REACH);
        fprintf(m_outputFile, "Reach Center: [%f, %f] m\n", m_center(0)*CM_TO_METERS, m_center(1)*CM_TO_METERS);
        fprintf(m_outputFile, "Reach Distance: %f m\n\n", (double)REACH_DIST*CM_TO_METERS);
    }
}

void expWidget::writeHeaderForData()
{
    if (m_outputFile != NULL)
        switch (m_test.p_type) {
        case staircase:
            fprintf(m_outputFile, "\n**************\nStaircase Data\n**************\n\n");
            fprintf(m_outputFile, "Trial Complete?, Timeout?, "
                                  "Time [sec], Shoulder Pos [deg], Elbow Pos [deg], Shoulder Vel [deg/s], Elbow Vel [deg/s], "
                                  "Hand PosX [m], Hand PosY [m], Hand VelX [m/s], Hand VelY [m/s], "
                                  "Test Type, Joint, Staircase #, Judgement #, "
                                  "Reference Angle [deg], Comparison Angle [deg], Subject Response, Correct Response\n");
            break;
        case match1D:
            fprintf(m_outputFile, "\n*****************\n1-D Matching Data\n*****************\n\n");
            fprintf(m_outputFile, "Trial Complete?, Timeout?, "
                                  "Time [sec], Shoulder Pos [deg], Elbow Pos [deg], Shoulder Vel [deg/s], Elbow Vel [deg/s], "
                                  "Hand PosX [m], Hand PosY [m], Hand VelX [m/s], Hand VelY [m/s], "
                                  "Test Type, Joint, Active, Vision, Trial #, Target Angle [deg], Start Angle [deg], Response Angle [deg]\n");
            break;
        case match2D:
            fprintf(m_outputFile, "\n*****************\n2-D Matching Data\n*****************\n\n");
            fprintf(m_outputFile, "Trial Complete?, Timeout?, "
                                  "Time [sec], Shoulder Pos [deg], Elbow Pos [deg], Shoulder Vel [deg/s], Elbow Vel [deg/s], "
                                  "Hand PosX [m], Hand PosY [m], Hand VelX [m/s], Hand VelY [m/s], "
                                  "Test Type, Active, Vision, Trial #, Target PosX [m], Target PosY [m], Start PosX [m], Start PosY [m], "
                                  "Response PosX [m], Response PosY [m]\n");
            break;
        default:
            break;
        }
}

void expWidget::saveData()
{
    if (!m_trial.p_isPractice) {  // only save for non-practice trials
        exp_data temp;

        // record kinematic data
        temp.d_time = m_parent->m_parent->m_exo->m_t;
        temp.d_th = m_parent->m_parent->m_exo->m_th*(180/PI);
        temp.d_thdot = m_parent->m_parent->m_exo->m_thdot*(180/PI);
        temp.d_pos = m_parent->m_parent->m_exo->m_pos;
        temp.d_vel = m_parent->m_parent->m_exo->m_vel;

        // record experiment state
        temp.d_test = m_test;
        temp.d_trial = m_trial;
        temp.d_trialComplete = m_trialComplete;
        temp.d_timeOut = m_timeOut;

        // record staircase data
        int i = getStairIndex(m_currStaircase);
        temp.d_currStaircase = m_currStaircase;
        temp.d_numJudgements = m_numJudgements[i];
        temp.d_subjResp = m_subjResp[i];
        temp.d_corrResp = m_corrResp[i];

        // record matching data
        temp.d_numTrials = m_numTrials;
        temp.d_subjAng = m_subjAng;
        temp.d_subjPos = m_subjPos*CM_TO_METERS;

        // push into vector for current test
        m_testData.push_back(temp);
    }
}

void expWidget::recordData()
{
    // iterate over vector of saved data
    for (vector<exp_data>::iterator it = m_testData.begin() ; it != m_testData.end(); ++it) {

        if (m_outputFile != NULL) {

            // data to write for every test type
            fprintf(m_outputFile,"%d, %d, %f, %f, %f, %f, %f, %f, %f, %f, %f, %d, ",
                    it->d_trialComplete, it->d_timeOut,
                    it->d_time, it->d_th(0), it->d_th(1), it->d_thdot(0), it->d_thdot(1),
                    it->d_pos(0), it->d_pos(1), it->d_vel(0), it->d_vel(1),
                    it->d_test.p_type);

            // write different data depending on test type
            cVector3d d_targPos = angleToTarg(it->d_trial.p_targ)*CM_TO_METERS;
            cVector3d d_refPos;
            if (m_trial.p_ref < 0)  d_refPos = m_center*CM_TO_METERS;
            else                    d_refPos = angleToTarg(it->d_trial.p_ref)*CM_TO_METERS;
            switch (it->d_test.p_type) {
            case staircase:
                fprintf(m_outputFile,"%d, %d, %d, %f, %f, %d, %d\n",
                        it->d_test.p_joint, it->d_currStaircase, it->d_numJudgements,
                        it->d_trial.p_targ, it->d_trial.p_ref, it->d_subjResp, it->d_corrResp);
                break;
            case match1D:
                fprintf(m_outputFile,"%d, %d, %d, %d, %f, %f, %f\n",
                        it->d_test.p_joint, it->d_test.p_active, it->d_test.p_vision, it->d_numTrials,
                        it->d_trial.p_targ, it->d_trial.p_ref, it->d_subjAng);
                break;
            case match2D:
                fprintf(m_outputFile,"%d, %d, %d, %f, %f, %f, %f, %f, %f\n",
                        it->d_test.p_active, it->d_test.p_vision, it->d_numTrials,
                        d_targPos(0), d_targPos(1), d_refPos(0), d_refPos(1),
                        it->d_subjPos(0), it->d_subjPos(1));
                break;
            default:
                break;
            }
        }
    }

    // clear vector of saved data
    m_testData.clear();
}


void expWidget::checkForResp()
{
    // check for mouse event before keyboard
    processScroll();
    if      (m_mousePressed)  processButton();
    else if (m_keyPressed)    processKey();
    else {
        m_trialComplete = 0;
        if (CNTDWN) {
            if (m_test.p_type != staircase) m_timeOut = m_cntdwn->timeoutOccurred();
            else                            m_timeOut = false;  // no "time outs" during staircase tests
        } else {
            m_timeOut = false;
        }
    }

    // no need to save data if it can't be written to file
    if (m_outputFile != NULL) {
        if (m_trialComplete || m_timeOut) {
            saveData();
        }

        // also save data for active tests when data-recording timer has expired
        // NOTE: this timer should only be running when the data file is open for writing
        else if (m_test.p_active) {
            if (m_dataClk->timeoutOccurred()) {
                saveData();
                m_dataClk->setTimeoutPeriodSeconds(T_RECORD*MSEC_TO_SEC);
                m_dataClk->start(true);
            }
        }
    }
}

void expWidget::processKey()
{
    int i = getStairIndex(m_currStaircase);
    switch (m_test.p_type) {

    // for staircase, R/L = (forced) choice
    case staircase:

        switch (m_key) {
        case Qt::Key_Right:
            m_subjRespPrev[i] = m_subjResp[i];
            m_subjResp[i] = FURTHER;
            m_trialComplete = 1;
            break;
        case Qt::Key_Left:
            m_subjRespPrev[i] = m_subjResp[i];
            m_subjResp[i] = CLOSER;
            m_trialComplete = 1;
            break;
        default:
            m_trialComplete = 0;
            break;
        }
        if (DEBUG) {
            qDebug() << "";
            if (m_corrResp[i] == FURTHER)  qDebug() << "  correct response = FURTHER";
            else                           qDebug() << "  correct response = CLOSER";
            if (m_subjResp[i] == FURTHER)  qDebug() << "  subject response = FURTHER";
            else                           qDebug() << "  subject response = CLOSER";
        }
        break;


    case match1D:
        // for 1-D active matching, SPACE = done moving arm segment (to match), S = skip trial
        if (m_test.p_active) {
            switch (m_key) {
            case Qt::Key_Space:
                m_subjAng = m_parent->m_parent->m_exo->m_th(m_test.p_joint)*(180/PI);
                m_trialComplete = 1;
                break;
            case Qt::Key_S:
                m_subjAng = m_parent->m_parent->m_exo->m_th(m_test.p_joint)*(180/PI);
                m_trialComplete = 2;
                break;
            default:
                m_trialComplete = 0;
                break;
            }
        }
        // for 1-D passive matching, SPACE = done moving cursor (to match), S = skip trial, R/L = move cursor
        else {
            switch (m_key) {
            case Qt::Key_Space:
                m_trialComplete = 1;
                break;
            case Qt::Key_S:
                m_trialComplete = 2;
                break;
            case Qt::Key_Right:
                m_subjAng -= ANG_STEP;
                break;
            case Qt::Key_Left:
                m_subjAng += ANG_STEP;
                break;
            default:
                m_trialComplete = 0;
                break;
            }
        }
        break;

    case match2D:
        // for 2-D active matching, SPACE = done moving arm (to match), S = skip trial
        if (m_test.p_active) {
            switch (m_key) {
            case Qt::Key_Space:
                m_subjPos = m_parent->m_parent->m_exo->m_pos*(1/CM_TO_METERS);
                m_trialComplete = 1;
                break;
            case Qt::Key_S:
                m_subjPos = m_parent->m_parent->m_exo->m_pos*(1/CM_TO_METERS);
                m_trialComplete = 2;
                break;
            default:
                m_trialComplete = 0;
                break;
            }
        }
        // for 2-D passive matching, SPACE = done moving cursor (to match), S = skip trial, R/L/U/D = move cursor
        else {
            switch (m_key) {
            case Qt::Key_Space:
                m_trialComplete = 1;
                break;
            case Qt::Key_S:
                m_trialComplete = 2;
                break;
            case Qt::Key_Right:
                m_subjPos += cVector3d(POS_STEP,0.0,0.0);
                break;
            case Qt::Key_Left:
                m_subjPos -= cVector3d(POS_STEP,0.0,0.0);
                break;
            case Qt::Key_Up:
                m_subjPos += cVector3d(0.0,POS_STEP,0.0);
                break;
            case Qt::Key_Down:
                m_subjPos -= cVector3d(0.0,POS_STEP,0.0);
                break;
            default:
                m_trialComplete = 0;
                break;
            }
        }
        break;

    default:
        break;
    }

    m_keyPressed = false;
}

void expWidget::processButton()
{
    int i = getStairIndex(m_currStaircase);
    switch (m_test.p_type) {

    // for staircase, R/L = (forced) choice
    case staircase:
        switch (m_button) {
        case Qt::RightButton:
            m_subjRespPrev[i] = m_subjResp[i];
            if (m_parent->m_parent->m_exo->m_subj->m_rightHanded) m_subjResp[i] = FURTHER;
            else                                                  m_subjResp[i] = CLOSER;
            m_trialComplete = 1;
            break;
        case Qt::LeftButton:
            m_subjRespPrev[i] = m_subjResp[i];
            if (m_parent->m_parent->m_exo->m_subj->m_rightHanded) m_subjResp[i] = CLOSER;
            else                                                  m_subjResp[i] = FURTHER;
            m_trialComplete = 1;
            break;
        default:
            m_trialComplete = 0;
            break;
        }
        break;

    case match1D:
        // for 1-D active matching, L = done moving arm segment (to match)
        if (m_test.p_active) {
            switch (m_button) {
            case Qt::LeftButton:
                m_subjAng = m_parent->m_parent->m_exo->m_th(m_test.p_joint)*(180/PI);
                m_trialComplete = 1;
                break;
            default:
                m_trialComplete = 0;
                break;
            }
        }
        // for 1-D passive matching, L = done moving cursor (to match)
        else {
            switch (m_button) {
            case Qt::LeftButton:
                m_trialComplete = 1;
                break;
            default:
                m_trialComplete = 0;
                break;
            }
        }
        break;

    case match2D:
        // for 2-D active matching, L = done moving arm (to match)
        if (m_test.p_active) {
            switch (m_button) {
            case Qt::LeftButton:
                m_subjPos = m_parent->m_parent->m_exo->m_pos*(1/CM_TO_METERS);
                m_trialComplete = 1;
                break;
            default:
                m_trialComplete = 0;
                break;
            }
        }
        // for 2-D passive matching, L = done moving cursor (to match), M(iddle) = switch
        // scroll wheel control mode (between U/D & L/R)
        else {
            switch (m_button) {
            case Qt::LeftButton:
                m_trialComplete = 1;
                break;
            case Qt::MiddleButton:
                m_isUpDown = !m_isUpDown;
                break;
            default:
                m_trialComplete = 0;
                break;
            }
        }
        break;

    default:
        break;
    }

    m_mousePressed = false;
}

void expWidget::processScroll()
{
    // scroll wheel only used for passive tests
    if (!m_test.p_active) {

        switch (m_test.p_type) {

        // in 1-D, FORWARD/BACKWARD scroll = move cursor closer to/further from body
        case match1D:
            m_subjAng += ANG_STEP * m_stepsScrolled;
            break;

        // in 2-D, FORWARD/BACKWARD scroll = move cursor in +/-y or +/-x, depending on mode
        case match2D:
            if (m_isUpDown) {
                m_subjPos += cVector3d(0.0,POS_STEP * m_stepsScrolled,0.0);
            } else {
                m_subjPos += cVector3d(POS_STEP * m_stepsScrolled,0.0,0.0);
            }
            break;

        default:
            break;
        }
    }
    m_stepsScrolled = 0;
}


int expWidget::getStairIndex(int currStaircase)
{
    if (DOUB_STAIRS) {
        return fmod(currStaircase,2);
    } else {
        return 0;  // don't really need array if only one staircase at a time
    }
}

void expWidget::estimatePerceptParams()
{
    // extract last 4 angles tested in staircases 1 & 2
    vector<double> data;
    for (int sc = 0; sc < 2; sc++) {
        int numJudgements = m_refAngsForEstim[sc].size();
        for (int i = 0; i < NUM_FOR_EST; i++) {
            data.push_back(m_refAngsForEstim[sc][numJudgements-(i+1)]);
        }
        m_refAngsForEstim[sc].clear();
    }

    // estimate perceptual boundary & sensitivity (mean/range of last 4 angles tested in staircases 1 & 2)
    double max = -180;
    double min = 180;
    double sum = 0;
    for(vector<double>::iterator it = data.begin(); it != data.end(); ++it) {
        if (*it > max) max = *it;
        if (*it < min) min = *it;
        sum = sum + *it;
    }
    m_boundary = sum/data.size();
    m_sensitiv = 0.75*(max-min);
    if (DEBUG)  qDebug() << "** boundary = " << m_boundary << " / sensitivity = " << m_sensitiv << " **";
}

chai3d::cVector3d expWidget::angleToTarg(double ang)
{
    double xTarg = REACH_DIST*cos(ang*PI/180) + m_center(0);
    double yTarg = REACH_DIST*sin(ang*PI/180) + m_center(1);
    return cVector3d(xTarg, yTarg, 0.0);
}

std::vector<double> expWidget::getStartTargs(double ang)
{
    // compute angles adjacent to target
    double angStep = (double)CIRCLE_DEG/NUM_REACH;
    double adjAng1 = fmod(ang + angStep, CIRCLE_DEG);
    double adjAng2 = fmod(ang - angStep, CIRCLE_DEG);
    if (adjAng2 < 0) adjAng2 += CIRCLE_DEG;  // only 'adjAng2' can be negative

    // loop over ALL angles, saving those not equal or adjacent to target
    std::vector<double> startPos;
    for (int i = 0; i < NUM_REACH; i++) {
        double currAng = m_targPos[i];
        if (currAng != ang && currAng != adjAng1 && currAng != adjAng2)
            startPos.push_back(currAng);
    }
    return startPos;
}

void expWidget::hideTestStateLabels()
{
    m_labelPractice->setShowEnabled(false);
    m_labelCntdwn->setShowEnabled(false);
    m_labelTestType->setShowEnabled(false);
    m_labelJoint->setShowEnabled(false);
    m_labelActive->setShowEnabled(false);
    m_labelVision->setShowEnabled(false);
    m_labelNumStair->setShowEnabled(false);
    m_labelNumJudge->setShowEnabled(false);
    m_labelNumRev->setShowEnabled(false);
    m_labelNumTrial->setShowEnabled(false);
}

int expWidget::randInRange(int low, int high)
{
    return (int) (low + rand() % (high-low+1));
}

bool expWidget::randBool()
{
    return rand() % 2 == 1;
}

int expWidget::randSign()
{
    if (randBool()) return 1;
    else            return -1;
}


void expWidget::initializeGL()
{
#ifdef GLEW_VERSION
    glewInit();
#endif

    // enable anti-aliasing
    QGLWidget::setFormat(QGLFormat(QGL::SampleBuffers));
}

void expWidget::paintGL()
{
    if (!m_running) return;

    m_worldLock.acquire();

    // render world
    m_camera->renderView(m_width, m_height);
    glFinish();

    m_worldLock.release();
}

void expWidget::resizeGL(int a_width, int a_height)
{
    m_worldLock.acquire();

    m_width = a_width;
    m_height = a_height;

    m_worldLock.release();
}

void expWidget::keyPressEvent(QKeyEvent *event)
{
    // pass keyboard handling to base class if experiment not running
    if (!m_running) {
        QGLWidget::keyPressEvent(event);
        return;
    }

    // get pressed key (only register if previous press has been processed
    // and current key is SPACE, RIGHT, LEFT, UP, DOWN, or 'S')
    switch (event->key()) {

    case Qt::Key_Space:
    case Qt::Key_Right:
    case Qt::Key_Left:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_S:
        m_key = event->key();
        if (!m_keyPressed) m_keyPressed = true;
        break;

    // toggle dither control, to avoid switching mouse/keyboard focus back to console
    case Qt::Key_D:
        m_parent->m_parent->toggleDither();
        break;

    default:
        QGLWidget::keyPressEvent(event);
    }
}

void expWidget::mousePressEvent(QMouseEvent *event)
{
    // pass mouse handling to base class if experiment not running
    // and not waiting for subject input
    if (!m_running || m_state != waitingForResponse) {
        QGLWidget::mousePressEvent(event);
        return;
    }

    // check for button presses (only register if previous press has
    // been processed)
    switch (event->button()) {

    case Qt::LeftButton:
    case Qt::MiddleButton:
    case Qt::RightButton:
        m_button = event->button();
        if (!m_mousePressed) m_mousePressed = true;
        if (DEBUG) {
            if      (m_button == Qt::LeftButton)    qDebug() << "  LEFT button pressed";
            else if (m_button == Qt::MiddleButton)  qDebug() << "  MIDDLE button pressed";
            else if (m_button == Qt::RightButton)   qDebug() << "  RIGHT button pressed";
            else                                    qDebug() << "  NO button pressed";
        }
        break;

    default:
        QGLWidget::mousePressEvent(event);
    }
}

void expWidget::wheelEvent(QWheelEvent *event)
{
    // pass scroll-wheel handling to base class if experiment not running
    // and not waiting for subject input
    if (!m_running || m_state != waitingForResponse) {
        QGLWidget::wheelEvent(event);
        return;
    }

    // get degrees scrolled and convert to scroll-wheel "clicks"
    QPoint numDegrees = event->angleDelta() / 4;  // units = [1/4 degree]
    if (!numDegrees.isNull()) {
        m_stepsScrolled = numDegrees.y() / MOUSE_STEP;
        if (DEBUG)  qDebug() << " scrolled" << m_stepsScrolled << "clicks";
    } else {
        QGLWidget::wheelEvent(event);
    }
}
