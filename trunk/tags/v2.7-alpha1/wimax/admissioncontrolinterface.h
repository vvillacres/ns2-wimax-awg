
#ifndef ADMISSIONCONTROLINTERFACE_H
#define ADMISSIONCONTROLINTERFACE_H

#include "schedulingalgointerface.h"

class Mac802_16;
class ServiceFlow;
class PeerNode;


class AdmissionControlInterface
{

public:

	AdmissionControlInterface (Mac802_16 * mac);

	~AdmissionControlInterface ();

	virtual bool checkAdmission( ServiceFlow * serviceFlow, PeerNode * peer);

protected:

	// reference to mac layer
    Mac802_16 * mac_;

};

#endif //ADMISSIONCONTROLINTERFACE_H

