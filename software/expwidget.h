#ifndef EXPWIDGET_H
#define EXPWIDGET_H

#include "chai3d.h"
#include "expwindow.h"
#include "exo.h"
#include <cstdlib>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>
#include <array>
#include <vector>
#include <queue>
#include <random>
#include <algorithm>
#include <regex>
#include <QTimer>
#include <QBasicTimer>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#define NUM_JNT     2  // number of joint DOFs being tested
#define NUM_ANG     3  // number of angles tested per joint for 1-D tests
#define NUM_MATCH   4  // number of matches per target joint angle (different starting offset for each) & target position (all center-out)
#define NUM_REACH   8  // number of target positions for 2-D matching (on circle)
                       // NOTE: this implicitly defines the number of matches per target position
                       // ----  (see function 'targToStartPos')

// enumeration of experiment states in FSM
typedef enum
{
    welcome,
    locking,
    pause,
    resetting,
    grounding,
    setting,
    waitingForResponse,
    breaking,
    thanks
} exp_states;

// data to be recorded for offline analysis
typedef struct
{
    // kinematic data (from exo)
    double d_time;              // [sec]
    chai3d::cVector3d d_th;     // [deg]
    chai3d::cVector3d d_thdot;  // [deg/s]
    chai3d::cVector3d d_pos;    // [m]
    chai3d::cVector3d d_vel;    // [m/s]

    // experiment state
    test_params d_test;
    trial_params d_trial;
    bool d_trialComplete;

    // staircase data
    int d_currStaircase;
    int d_numJudgements;
    bool d_subjResp;
    bool d_corrResp;

    // matching data
    bool d_timeOut;
    int d_numTrials;
    double d_subjAng;             // [deg]
    chai3d::cVector3d d_subjPos;  // [m]

} exp_data;

void _expThread(void *arg);  // pointer to thread function (not a class member)

class expWidget : public QGLWidget
{
public:
    ExpWindow* m_parent;                  // pointer to experiment window

    chai3d::cThread m_thread;             // experiment thread
    chai3d::cMutex m_worldLock;           // mutex for graphics updates
    chai3d::cMutex m_runLock;             // mutex for experiment updates

    chai3d::cWorld* m_world;              // CHAI world
    chai3d::cCamera* m_camera;            // camera to render the world
    chai3d::cDirectionalLight* m_light;   // light to illuminate the world
    chai3d::cShapeCylinder* m_angle;      // "infinitely" long cylinder representing test angle for 1-D tests (static in active test, dynamic in passive test)
    chai3d::cShapeSphere* m_target;       // sphere representing target position for 2-D matching (static in active test, dynamic in passive test)
    chai3d::cShapeCylinder* m_upperarm;   // cylinder representing upperarm
    chai3d::cShapeCylinder* m_forearm;    // cylinder representing forearm
    chai3d::cShapeCylinder* m_upperarmL;  // "infinitely" long cylinder representing upperarm for 1-D tests
    chai3d::cShapeCylinder* m_forearmL;   // "infinitely" long cylinder representing forearm for 1-D tests
    chai3d::cShapeSphere* m_shoulder;     // sphere representing shoulder joint
    chai3d::cShapeSphere* m_elbow;        // sphere representing elbow joint
    chai3d::cShapeSphere* m_hand;         // sphere representing hand
    chai3d::cLabel* m_instruct;           // instructional message for subject
    chai3d::cLabel* m_note;               // note for experimenter
    chai3d::cLabel* m_remind;             // reminder for subject about current state/test
    chai3d::cLabel* m_labelPractice;      // large label denoting current trial as practice
    chai3d::cLabel* m_labelCntdwn;        // large label displaying seconds left in current trial
    chai3d::cLabel* m_labelTestType;      // label for number of joints being tested (1 or 2)
    chai3d::cLabel* m_labelJoint;         // label for current joint being tested (S, E, or --)
    chai3d::cLabel* m_labelActive;        // label for current movement condition (active or passive)
    chai3d::cLabel* m_labelVision;        // label for current vision condition (with or without)
    chai3d::cLabel* m_labelNumStair;      // label for current staircase number for current joint & test angle
    chai3d::cLabel* m_labelNumJudge;      // label for number of judgements made during current staircase pair: (above, below)
    chai3d::cLabel* m_labelNumRev;        // label for number of judgement reversals during current staircase pair: (above, below)
    chai3d::cLabel* m_labelNumTrial;      // label for number of matching trials performed for given test (& joint if 1-D test)

    bool m_running;                       // TRUE = experiment thread is running
    QBasicTimer* m_timer;                 // timer for graphics updates
    chai3d::cPrecisionClock* m_clk;       // clock for timing experiment segments
    chai3d::cPrecisionClock* m_cntdwn;    // countdown clock for trials (time remaining to be displayed onscreen)
    chai3d::cPrecisionClock* m_dataClk;   // clock governing frequency of data recording
    int m_width;                          // width of view
    int m_height;                         // height of view
    bool m_keyPressed;                    // TRUE = (relevant) key has been pressed
    int m_key;                            // identity of pressed key
    bool m_mousePressed;                  // TRUE = (relevant) mouse button has been pressed
    Qt::MouseButton m_button;             // identity of pressed button
    bool m_isUpDown;                      // TRUE = scroll wheel moves cursor up/down, FALSE = right/left
    int m_stepsScrolled;                  // "clicks" moved by mouse scroll wheel (+ = away from user, - = towards user)

    exp_snapshot m_snap;
    expWidget(QWidget *parent);
    virtual ~expWidget();

    void start();
    void stop();
    void* expThread();

protected:
    // overall experiment
    exp_states m_state;                                  // current experiment state
    test_params m_test;                                  // parameters for current test
    test_params m_testPrev;                              // parameters for previous test
    trial_params m_trial;                                // parameters for current trial
    trial_params m_trialPrev;                            // parameters for previous trial
    std::queue<trial_params> m_trialsCurr;               // queue of trials for current test
    int m_trialComplete;                                 // 1 = current trial is complete (i.e., subject responded), 2 = skipped trial, 0 = not yet complete
    int m_timeRemaining;                                 // time remaining trial until 'time out' [sec]
    bool m_timeOut;                                      // TRUE = timer expired for current trial (signals end of trial for all but staircase)
    bool m_testComplete;                                 // TRUE = all trials of given test type are complete
    bool m_expComplete;                                  // TRUE = all tests are complete
    bool m_lockSignal;                                   // TRUE = experimenter must MANUALLY (un)lock joint(s)
    int m_breakTime;                                     // break time [min]

    // staircase tests
    int m_stairStatus;                                   // status of staircase test: 2 = beginning new angle/joint, 1 = beginning new (double) staircase, 0 = mid-staircase
    int m_numStaircases;                                 // number of staircases completed for current joint & test angle
    int m_currStaircase;                                 // current staircase (only important if double staircase)
    int m_maxStaircases;                                 // 6 normally (if double-staircase, 3 sets of 2), 1 for practice (1 set of 2)
    int m_numJudgements[2];                              // number of judgements made during current staircase (pair)
    int m_numReversals[2];                               // number of judgement reversals during current staircase (pair)
    int m_refStartSide[2];                               // 1 = visual reference starts CLOSER to body relative to test angle, -1 = FURTHER from body
    int m_refStepDir[2];                                 // (+/-)1 depending on where reference falls relative to test angles
    double m_refStep[2];                                 // step size for visual reference between judgements [deg]
    double m_refAng[2];                                  // angle of visual reference, for current judgement [deg]
    bool m_corrResp[2];                                  // correct response to 2AFC: TRUE = FURTHER, FALSE = CLOSER (reference relative to test angle)
    bool m_subjResp[2];                                  // subject's response to 2AFC: TRUE = FURTHER, FALSE = CLOSER (reference relative to test angle)
    bool m_subjRespPrev[2];                              // subject response from previous judgement
    bool m_oneCorrect[2];                                // TRUE = correctly answered one 2AFC
    double m_boundary;                                   // estimate of subject's proprioceptive boundary, used if adaptive [deg]
    double m_sensitiv;                                   // estimate of subject's proprioceptive sensitivity, used if adaptive [deg]
    std::vector<double> m_refAngsForEstim[2];            // vectors for tracking visual reference angles over staircases 1-2 [deg]
    std::queue<trial_params> m_firstStairTrls[NUM_JNT];  // queues of test angles for joint staircases (randomized) [deg]

    // 1-D matching tests
    int m_numTrials;                                     // number of matching trials performed for given test (& joint if 1-D test)
    double m_subjAng;                                    // angle of subject's joint (active)/angle cursor (passive), for 1-D matching [deg]
    std::queue<trial_params> m_1DMatchTrls_NV[NUM_JNT];  // queues of target angles for 1-D matching without visual feedback (randomized) [deg]
    std::queue<trial_params> m_1DMatchTrls_V[NUM_JNT];   // queues of target angles for 1-D matching WITH visual feeback (randomized) [deg]

    // 2-D matching tests
    int m_numMatch;                                      // number of matches for a given target (depends on whether reaches are center-out or not)
    chai3d::cVector3d m_center;                          // center-out position for 2-D matching [cm]
    chai3d::cVector3d m_subjPos;                         // position of subject's hand (active)/position cursor (passive), for 2-D matching [cm]
    std::queue<trial_params> m_2DMatchTrls_NV;           // queue of target positions (coded as angles) for 2-D matching without visual feedback (randomized) [deg]
    std::queue<trial_params> m_2DMatchTrls_V;            // queue of target positions (coded as angles) for 2-D matching WITH visual feedback (randomized) [deg]

    // parameters for defining tests
    // NOTE: those initialized here cannot (easily) be defined
    // ----  via nested loops (as in the 'createTests' function)
    double m_targAngs[NUM_JNT][NUM_ANG] =                // target joint angles [deg]
        { {20, 30, 40},
          {105, 115, 125} };
    double m_startOff[NUM_JNT][NUM_ANG][NUM_MATCH] =     // angular offsets to test for each target joint angle [deg]
        { { {-10, -5, 5, 10},                            // NOTE: only a subset of these tested WITH visual feedback
            {-20, -10, 10, 20},
            {-10, -5, 5, 10} },
          { {-10, -5, 5, 10},
            {-20, -10, 10, 20},
            {-10, -5, 5, 10} } };
    double m_targPos[NUM_REACH];                         // angles of target positions (on circle) [deg]
    double m_startPos[NUM_REACH][NUM_REACH-3];           // starting angles to test for each target position (on circle) [deg]
                                                         // NOTE: only a subset of these tested WITH visual feedback

    // graphics
    // (NOTE: we move camera to align graphics with real world
    //  instead of continually adding on an (x,y) offset)
    double m_camX;                                       // x-coordinate of camera viewing the world [m]
    double m_camY;                                       // y-coordinate of camera viewing the world [m]
    double m_scaleFactor;                                // factor by which to scale graphics, used for parallax correction
    chai3d::cVector3d m_scalePoint;                      // point about which to scale graphics, used for parallax correction [m]
    chai3d::cVector3d m_camPos;                          // position of camera viewing the world [m]
    chai3d::cVector3d m_camLookAt;                       // point at which camera is pointing [m]
    chai3d::cVector3d m_camUp;                           // vector pointing to top of field of view (FOV) [m]
    chai3d::cVector3d m_shldOff;                         // offset of shoulder relative to graphics origin [m]
    chai3d::cVector3d m_groundPos;                       // hand position for visual grounding (constant across experiment) [cm]
    chai3d::cVector3d m_groundAng;                       // joint angles for visual grounding (correspond exactly to position above) [deg]

    // data saving
    char filename[100];                                  // output filename
    FILE* m_outputFile;                                  // data file for entire experiment (all tests)
    std::vector<exp_data> m_testData;                    // data for one test
    bool m_headerWritten[NUM_TESTS] = {false};           // TRUE = header written for associated test type

    // initialization functions
    void initExperiment();
    void prepForSubj();
    void createTests();

    // update functions
    void updateExperiment();
    void updateGraphics();
    void updateLabels();
    void updateTestParams();
    void updateTrialParams();
    void prepForTest();
    exp_states prepForNextTrial();
    void updateStaircase();
    void resetStaircase();

    // exo control functions
    void sendToGround();
    void sendToStart();
    void startTrialControl();

    // data recording functions
    void recordSubjParams();
    void recordExpParams();
    void writeHeaderForData();
    void saveData();
    void recordData();

    // user-input functions
    void checkForResp();
    void processKey();
    void processButton();
    void processScroll();

    // "small" helper functions
    int getStairIndex(int currStaircase);
    void estimatePerceptParams();
    chai3d::cVector3d angleToTarg(double ang);
    std::vector<double> getStartTargs(double ang);
    void hideTestStateLabels();
    int randInRange(int low, int high);
    bool randBool();
    int randSign();

    // "invisible" functions
    void initializeGL();
    void paintGL();
    void resizeGL(int a_width, int a_height);
    void keyPressEvent(QKeyEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    void timerEvent(QTimerEvent *event) { updateGL(); }
};

#endif // EXPWIDGET_H
