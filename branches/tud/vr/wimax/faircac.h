
#ifndef FAIRCAC_H
#define FAIRCAC_H

#include "admissioncontrolinterface.h"

class Mac802_16;
class ServiceFlow;
class PeerNode;

class FairCAC : public AdmissionControlInterface
{

public:

	FairCAC (Mac802_16 * mac,

		 /* The reserved bandwidth ratio factor, which is in the range of [0,1].
		    beta = 0.2 according to the simulation resault in References: Xin Guo, Wenchao Ma, Zihua Guo, and Zifeng Hou,
		    Dynamic Bandwidth Reservation Admission Control Scheme for the IEEE 802.16e Broadband Wireless Access Systems,
		    WCNC 2007, 11-15 March 2007 */
		    float beta,

         // The legal bandwidth, there are 10 users (1 with LB_i=2Mbps, 9 with LB_i=1Mbps)
            double LB_1, // user i = 1
            double LB_2,  // user j = 2...9

            int N
		    );

	virtual bool checkAdmission( ServiceFlow * serviceFlow, PeerNode * peer);


private:
	/*
	 * parameters for this algorithm
	 */

      float beta_;         // The reserved bandwidth ratio factor

      // The legal bandwidth, there are 10 users (1 with LB_i=2Mbps, 9 with LB_i=1Mbps)
      double LB_1_;  // user i = 1
      double LB_2_;  // user j = 2...9

      int N_;           // Number of users
};

#endif //FAIRCAC_H
