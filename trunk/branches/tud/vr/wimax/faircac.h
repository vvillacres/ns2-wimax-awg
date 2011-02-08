
#ifndef FAIRCAC_H
#define FAIRCAC_H

#include "serviceflowqosset.h"
#include "serviceflow.h"
#include "serviceflowhandler.h"
#include "mac802_16.h"

#include "admissioncontrolinterface.h"

class Mac802_16;

class FairCAC : public AdmissionControlInterface
{

public:

	FairCAC (Mac802_16 * mac);

	virtual bool checkAdmission( ServiceFlowQosSet * serviceFlowQosSet);
};

#endif //FAIRCAC_H
