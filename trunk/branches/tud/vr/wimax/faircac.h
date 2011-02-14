
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
            double RB_i,   // The remaining bandwidth, RB_i = sum_all the connections of Ui ( MRTR + beta * (MSTR - MRTR) )
		    double MSTR_req_i,  // MSTR for request of a user i
		    double MRTR_req_i,  // MRTR for request of a user i

		 /* The reserved bandwidth ratio factor, which is in the range of [0,1].
		    beta = 0.2 according to the simulation resault in References: Xin Guo, Wenchao Ma, Zihua Guo, and Zifeng Hou,
		    Dynamic Bandwidth Reservation Admission Control Scheme for the IEEE 802.16e Broadband Wireless Access Systems,
		    WCNC 2007, 11-15 March 2007 */
		    float beta,

            float B_a,   // available bandwidth in the whole bandwidth

         // The legal bandwidth, there are 10 users (1 with LB_i=2Mbps, 9 with LB_i=1Mbps)
            double LB_1, // user i = 1
            double LB_2,  // user j = 2...9

            int N
		    );

	virtual bool checkAdmission( ServiceFlow * serviceFlow, PeerNode * peer);


   // Bandwidth requirement of a new service for user i, B_req_i= $MRTR_req_i + $beta * ($MSTR_req_i - $MRTR_req_i)
	double B_req_i();


   // Bandwidth Acquirement Ratio (BAR) (the user is forbidden to send any connection request when his BAR is reach 0.85)
    double eta_i();


   // The admitting threshold for user Ui
    float TH_i();



private:
	/*
	 * parameters for this algorithm
	 */

      double RB_i_; // The remaining bandwidth, RB_i = sum_all the connections of Ui ( MRTR + beta * (MSTR - MRTR) )

      double MSTR_req_i_;  // MSTR for request of a user i
      double MRTR_req_i_;  // MRTR for request of a user i

      float beta_;         // The reserved bandwidth ratio factor

      double B_a_;   // available bandwidth percentage of the whole bandwidth

      // The legal bandwidth, there are 10 users (1 with LB_i=2Mbps, 9 with LB_i=1Mbps)
      double LB_1_;  // user i = 1
      double LB_2_;  // user j = 2...9


      double B_req_i_;  // Bandwidth requirement of a new service for user i
      double eta_i_;    // Bandwidth Acquirement Ratio (BAR)

      float TH_i_;      // The admitting threshold for user Ui

      int N_;           // Number of users
};

#endif //FAIRCAC_H
