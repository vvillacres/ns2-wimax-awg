
#ifndef THRESHOLDBASEDCAC_H
#define THRESHOLDBASEDCAC_H

#include "admissioncontrolinterface.h"

class Mac802_16;
class ServiceFlow;
class PeerNode;

class ThresholdBasedCAC : public AdmissionControlInterface
{

public:

	ThresholdBasedCAC (Mac802_16 * mac,
			float thresholdUgs,   //Thresholds for Admission Control
			float thresholdErtPs,
			float thresholdRtPs,
			float thresholdNrtPs,
			float thresholdBe
	);

	virtual ~ThresholdBasedCAC();

	virtual bool checkAdmission( ServiceFlow * serviceFlow, PeerNode * peer);

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
