
#ifndef ADMISSIONCONTROLINTERFACE_H
#define ADMISSIONCONTROLINTERFACE_H

//#include "serviceflowqosset.h"
//#include "serviceflow.h"
//#include "serviceflowhandler.h"
//#include "algorithm1.h"
//#include "algorithm2.h"

class Mac802_16;
class ServiceFlowQosSet;


class AdmissionControlInterface
{

public:

	AdmissionControlInterface (Mac802_16 * mac);

	~AdmissionControlInterface ();

	virtual bool checkAdmission( ServiceFlowQosSet * serviceFlowQosSet);

protected:

	// reference to mac layer
    Mac802_16 * mac_;

};

#endif //ADMISSIONCONTROLINTERFACE_H

