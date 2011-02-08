
#ifndef THRESHOLDBASEDCAC_H
#define ALGORITHM1_H

#include "serviceflowqosset.h"
#include "mac802_16.h"
#include "serviceflow.h"
#include "serviceflowhandler.h"

#include "admissioncontrolinterface.h"

class Mac802_16;

class ThresholdBasedCAC : public AdmissionControlInterface
{

public:

	ThresholdBasedCAC (Mac802_16 * mac

/*	float used_bandwidth,
	float threshold_be,
	float threshold_nrtPS,
	float threshold_rtPS,
	float threshold_ertPS,
	float threshold_ugs
*/
	);

	virtual bool checkAdmission( ServiceFlowQosSet * serviceFlowQosSet);

};

#endif //THRESHOLDBASEDCAC_H
