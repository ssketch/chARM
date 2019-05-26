#include "subject.h"

using namespace std;

subject::subject(std::string a_name, std::string a_ID, int a_age,
                 bool a_stroke, bool a_female, bool a_rightHanded,
                 double a_Lupper, double a_LtoEE, double a_Llower)
{
    // assign parameters
    m_name = a_name;
    m_ID = a_ID;
    m_age = a_age;
    m_stroke = a_stroke;
    m_female = a_female;
    m_rightHanded = a_rightHanded;
    m_Lupper = a_Lupper*INCH_TO_METERS;
    m_LtoEE = a_LtoEE*INCH_TO_METERS;
    m_Llower = a_Llower*INCH_TO_METERS;

    // joint limits always default (only changed via GUI)
    m_lims.shoul_min = -70*(PI/180);
    m_lims.shoul_max = 120*(PI/180);
    m_lims.elbow_min = 0*(PI/180);
    m_lims.elbow_max = 170*(PI/180);

    // define experiment parameters
    m_params = subjIDToExpParams(a_ID);
}

void subject::update(std::string a_name, std::string a_ID, int a_age,
                     bool a_stroke, bool a_female, bool a_rightHanded,
                     double a_Lupper, double a_LtoEE, double a_Llower, jointLims a_lims)
{
    m_name = a_name;
    m_ID = a_ID;
    m_age = a_age;
    m_stroke = a_stroke;
    m_female = a_female;
    m_rightHanded = a_rightHanded;
    m_Lupper = a_Lupper*INCH_TO_METERS;\
    m_LtoEE = a_LtoEE*INCH_TO_METERS;
    m_Llower = a_Llower*INCH_TO_METERS;
    m_lims.shoul_min = a_lims.shoul_min*(PI/180);
    m_lims.shoul_max = a_lims.shoul_max*(PI/180);
    m_lims.elbow_min = a_lims.elbow_min*(PI/180);
    m_lims.elbow_max = a_lims.elbow_max*(PI/180);
    m_params = subjIDToExpParams(a_ID);
}

exp_params subject::subjIDToExpParams(std::string ID)
{
    exp_params p;

    // extract subject number from ID
    regex pattern ("[^0-9]*");
    string filler ("");
    int n = stoi(regex_replace(ID, pattern, filler));

    // define joint order
    if (fmod(n,2) == 1) p.p_firstJnt = S;
    else                p.p_firstJnt = E;

    // define shoulder lock angle
    m_lockAngs = {{ {20, 30, 40}, {105, 115, 125} }};
    int ind = ceil((double(n)/NUM_ANG));
    if (ind > NUM_ANG)  ind = ind - NUM_ANG;
    for (int i = 1; i <= NUM_ANG; i++) {
        if (i == ind)   p.p_lockAngs[S] = m_lockAngs[S][i-1];
    }

    // define elbow lock angle
    ind = fmod(n,NUM_ANG) - 1;
    if (ind < 0) ind = ind + NUM_ANG;
    for (int i = 0; i < NUM_ANG; i++) {
        if (i == ind)   p.p_lockAngs[E] = m_lockAngs[E][i];
    }

    return p;
}
