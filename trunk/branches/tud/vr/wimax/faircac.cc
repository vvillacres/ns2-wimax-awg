
#include "faircac.h"

#include "serviceflow.h"
#include "peernode.h"
#include "mac802_16.h"
#include "serviceflowqosset.h"
#include "wimaxscheduler.h"
#include <iostream>
#include <cmath>

FairCAC::FairCAC ( Mac802_16 * mac,

	      float beta,   // The reserved bandwidth ratio factor

	      // The legal bandwidth for user:
          double LB_1,  // user i = 1
          double LB_2,  // user j = 2...9

          int N  // Number of users

          ) : AdmissionControlInterface ( mac)

{
beta_ = beta;
LB_1_ = LB_1;
LB_2_ = LB_2;
N_    = N;
}

FairCAC::~FairCAC()
{
	// nothing to do
}


/**
 * Don't use this algorithm ! It is faulty implementation of my student.
 */


bool FairCAC::checkAdmission( ServiceFlow * serviceFlow, PeerNode * peer)
{

	Dir_t direction = serviceFlow->getDirection();
	ServiceFlowQosSet * serviceFlowQosSet = serviceFlow->getQosSet();

	if (direction == UL) {
		// Uplink

		// Check if the call comes from a BS
		assert( mac_->getNodeType() == STA_BS);

		// Get current frame utilization
		frameUsageStat_t uplinkStat = mac_->getScheduler()->getUplinkStatistic();

		// Estimate demand of new service flow
		int uiuc = peer->getUIUC();

		// Allocate resources for management data

		// Capacity for one Slot in byte
		int slotCapacity = mac_->getPhy()->getSlotCapacity( mac_->getMap()->getUlSubframe()->getProfile( uiuc)->getEncoding(), UL_);
		printf("Current Slot Capacity %d for Peer %d \n", slotCapacity, peer->getAddr());

		double MSTR_req_i = serviceFlowQosSet->getMaxSustainedTrafficRate(); // MSTR for request of a user i
		double MRTR_req_i = serviceFlowQosSet->getMinReservedTrafficRate();  // MRTR for request of a user i

		// The remaining bandwidth for one user
		double RB_i =  2 * (MRTR_req_i + beta_ * ( MSTR_req_i - MRTR_req_i ));

		// Available bandwidth in the whole bandwidth
		double AvailableBandwidthSlots = uplinkStat.totalNbOfSlots - ( uplinkStat.usedMrtrSlots + uplinkStat.usedMstrSlots * 0.1);
		double AvailableBandwidth = AvailableBandwidthSlots * slotCapacity * 8 / mac_->getFrameDuration();

		// Bandwidth requirement of a new service for user i
		double B_req_i = MRTR_req_i + beta_ * ( MSTR_req_i - MRTR_req_i );

		// Bandwidth Acquirement Ratio (BAR) (the user is forbidden to send any connection request when his BAR is reach 0.85)
		// double eta_i =  1 - ( RB_i / LB_1_);

		N_++;
		std::cout<< N_ <<std::endl;

		// The admitting threshold for user Ui
		double TH_i =  2 * slotCapacity * 8 / mac_->getFrameDuration() * ( uplinkStat.usedMrtrSlots + uplinkStat.usedMstrSlots ) / N_ *  log ( 1+ ((N_-1) * LB_2_) / (LB_1_ + (N_-1) * LB_2_));

		// Conditions of the algorithm:
		if ( RB_i - B_req_i >= 0  &&  AvailableBandwidth - B_req_i >= TH_i ) {

			return true;
			debug2 ("Service is admitted");

		} else {

			return false;
			debug2(" Service is rejected");
	    }

    } else {

    	// Downlink

    	// Check if the call comes from a BS
    	assert( mac_->getNodeType() == STA_BS);

    	// Get current frame utilization
    	frameUsageStat_t downlinkStat = mac_->getScheduler()->getDownlinkStatistic();

    	// Estimate demand of new service flow
    	int uiuc = peer->getUIUC();

    	// Allocate resources for management data

    	// Capacity for one Slot in byte
	    int slotCapacity = mac_->getPhy()->getSlotCapacity( mac_->getMap()->getUlSubframe()->getProfile( uiuc)->getEncoding(), DL_);
	    printf("Current Slot Capacity %d for Peer %d \n", slotCapacity, peer->getAddr());

	    double MSTR_req_i = serviceFlowQosSet->getMaxSustainedTrafficRate(); // MSTR for request of a user i
	    double MRTR_req_i = serviceFlowQosSet->getMinReservedTrafficRate();  // MRTR for request of a user i

	    // The remaining bandwidth for one user

	    // remaining bandwidth for demanding user / user of new service flow
	    // RB_i = LB_i - summe of all conection mrtr + beta (mstr- mrtr)

	    double RB_i =  2 * (MRTR_req_i + beta_ * ( MSTR_req_i - MRTR_req_i ));

	    // Available bandwidth in the whole bandwidth
	    double AvailableBandwidthSlots = downlinkStat.totalNbOfSlots - ( downlinkStat.usedMrtrSlots + downlinkStat.usedMstrSlots * 0.1);
	    double AvailableBandwidth = AvailableBandwidthSlots * slotCapacity * 8 / mac_->getFrameDuration();

	    // Bandwidth requirement of a new service for user i
	    double B_req_i = MRTR_req_i + beta_ * ( MSTR_req_i - MRTR_req_i );

	    // Bandwidth Acquirement Ratio (BAR) (the user is forbidden to send any connection request when his BAR is reach 0.85)
	    // double eta_i =  1 - ( RB_i / LB_1_);

	    // number of all connections for this direction
	    N_ = N_++;
	    std::cout<< N_ <<std::endl;

	    // The admitting threshold for user Ui

	    // two times average bandwidth of all connections * ln ( 1


	    double TH_i =  2 * slotCapacity * 8 / mac_->getFrameDuration() * ( downlinkStat.usedMrtrSlots + downlinkStat.usedMstrSlots ) / N_ *  log ( 1+ ((N_-1) * LB_2_) / (LB_1_ + (N_-1) * LB_2_));

	    // Conditions of the algorithm:
	    if ( RB_i - B_req_i >= 0  &&  AvailableBandwidth - B_req_i >= TH_i ) {

	    	return true;
	    	debug2 ("Service is admitted");

	    } else {

	    	return false;
	    	debug2(" Service is rejected");
	    }

    }

}
