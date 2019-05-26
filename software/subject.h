#ifndef SUBJECT_H
#define SUBJECT_H

#include <string>
#include <array>
#include <regex>
#include <cmath>

#define NUM_JNT        2         // number of joint DOFs being tested (for defining experiment params)
#define NUM_ANG        3         // number of angles tested per joint for 1-D tests (for defining experiment params)
#define INCH_TO_METERS 0.0254    // conversion factor between inches and meters
#define PI             3.141592

// subject's joint limits
typedef struct
{
    double shoul_min;  // minimum shoulder angle, relative to zero [rad]
    double shoul_max;  // maximum shoulder angle, relative to zero [rad]
    double elbow_min;  // minimum elbow angle, relative to zero [rad]
    double elbow_max;  // maximum elbow angle, relative to zero [rad]
} jointLims;

// enumeration of joints to be tested (for defining experiment params)
typedef enum
{
    S,    // shoulder
    E,    // elbow
    both  // shoulder + elbow
} joints;

// structure capturing subject-specific experiment parameters
typedef struct
{
    joints p_firstJnt;                      // joint tested first
    std::array<double,NUM_JNT> p_lockAngs;  // angle at which joint is "locked" when other joint is being tested [deg]
} exp_params;

class subject
{
public:
    std::string m_name;   // subject name
    std::string m_ID;     // unique ID assigned to subject
    int m_age;            // subject age [years]
    bool m_stroke;        // subject stroke history (FALSE = no stroke, TRUE = stroke)
    bool m_female;        // subject gender (TRUE = female, FALSE = male)
    bool m_rightHanded;   // handedness (TRUE = righty, FALSE = lefty)
    double m_Lupper;      // length from shoulder to elbow joint [m]
    double m_LtoEE;       // length from elbow joint to exo end-effector [m]
    double m_Llower;      // length from elbow joint to fingertip of hand (for parallax correction) [m]
    jointLims m_lims;     // joint limits [rad]
    exp_params m_params;  // subject-specific experiment parameters

    // filled with default parameters
    // NOTE: lengths input in inches, angles in degrees
    subject(std::string a_name = "John Doe",
            std::string a_ID = "ctrl_01",
            int a_age = 50,
            bool a_stroke = false,
            bool a_female = false,
            bool a_rightHanded = true,
            double a_Lupper = 13.0,
            double a_LtoEE = 14.0,
            double a_Llower = 17.0);

    void update(std::string a_name, std::string a_ID, int a_age,
                bool a_stroke, bool a_female, bool a_rightHanded,
                double a_Lupper, double a_LtoEE, double a_Llower,
                jointLims a_lims);

    void updateName(std::string a_name) { m_name = a_name; }
    void updateID(std::string a_ID) { m_ID = a_ID; m_params = subjIDToExpParams(a_ID); }
    void updateAge(int a_age) { m_age = a_age; }
    void updateHealth(bool a_stroke) { m_stroke = a_stroke; }
    void updateGender(bool a_female) { m_female = a_female; }
    void updateHandedness(bool a_rightHanded) { m_rightHanded = a_rightHanded; }
    void updateUpperarm(double a_Lupper) { m_Lupper = a_Lupper*INCH_TO_METERS; }
    void updateLtoEE(double a_LtoEE) { m_LtoEE = a_LtoEE*INCH_TO_METERS; }
    void updateForearm(double a_Llower) { m_Llower = a_Llower*INCH_TO_METERS; }

    // for defining experiment params
    std::array<std::array<double,NUM_ANG>,NUM_JNT> m_lockAngs;  // angles at which joints can be locked for 1-D tests [deg]
    exp_params subjIDToExpParams(std::string ID);
};

#endif // SUBJECT_H
