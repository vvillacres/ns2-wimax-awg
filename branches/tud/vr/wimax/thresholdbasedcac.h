
#ifndef THRESHOLDBASEDCAC_H
#define THRESHOLDBASEDCAC_H

#include "serviceflowqosset.h"
#include "mac802_16.h"
#include "serviceflow.h"
#include "serviceflowhandler.h"

#include "admissioncontrolinterface.h"

class Mac802_16;

class ThresholdBasedCAC : public AdmissionControlInterface
{

public:

	ThresholdBasedCAC (Mac802_16 * mac,
			float thresholdUgs,
			float thresholdErtPs,
			float thresholdRtPs,
			float thresholdNrtPs,
			float thresholdBe
	);

	virtual bool checkAdmission( ServiceFlowQosSet * serviceFlowQosSet);

private:
	/*
	 * Thresholds for reserved Bandwidth
	 */
	float thresholdUgs_;
	float thresholdErtPs_;
	float thresholdRtPs_;
	float thresholdNrtPs_;
	float thresholdBe_;


};

#endif //THRESHOLDBASEDCAC_H
