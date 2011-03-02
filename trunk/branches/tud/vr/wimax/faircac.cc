
#include "faircac.h"

#include "serviceflow.h"
#include "peernode.h"
#include "mac802_16.h"
#include "serviceflowqosset.h"
#include "wimaxscheduler.h"

FairCAC::FairCAC ( Mac802_16 * mac,

	      float beta,   // The reserved bandwidth ratio factor

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


bool FairCAC::checkAdmission( ServiceFlow * serviceFlow, PeerNode * peer)
{

	Dir_t direction = serviceFlow->getDirection();
	ServiceFlowQosSet * serviceFlowQosSet = serviceFlow->getQosSet();

	if (direction == UL) {
	// uplink

	// Check if the call comes from a BS
	assert( mac_->getNodeType() == STA_BS);

	// get current frame utilization
	frameUsageStat_t uplinkStat = mac_->getScheduler()->getUplinkStatistic();

    // estimate demand of new service flow
    int uiuc = peer->getUIUC();

    // allocate resources for management data

    // Capacity for one Slot in byte
	    int slotCapacity = mac_->getPhy()->getSlotCapacity( mac_->getMap()->getUlSubframe()->getProfile( uiuc)->getEncoding(), UL_);
	    printf("Current Slot Capacity %d for Peer %d \n", slotCapacity, peer->getAddr());

	double MSTR_req_i = serviceFlowQosSet->getMaxSustainedTrafficRate(); // MSTR for request of a user i
	double MRTR_req_i = serviceFlowQosSet->getMinReservedTrafficRate();  // MRTR for request of a user i

	// The remaining bandwidth for one user
	double RB_i =  2 * (MRTR_req_i + beta_ * ( MSTR_req_i - MRTR_req_i ));

	// available bandwidth in the whole bandwidth
	double AvailableBandwidthSlots = uplinkStat.totalNbOfSlots - ( uplinkStat.usedMrtrSlots + uplinkStat.usedMrtrSlots * 0.1);
    double AvailableBandwidth = AvailableBandwidthSlots * slotCapacity * 8 / mac_->getFrameDuration();

	// Bandwidth requirement of a new service for user i
	double B_req_i = MRTR_req_i + beta_ * ( MSTR_req_i - MRTR_req_i );

	// Bandwidth Acquirement Ratio (BAR) (the user is forbidden to send any connection request when his BAR is reach 0.85)
    // double eta_i =  1 - ( RB_i / LB_1_);

	// The admitting threshold for user Ui
	double TH_i = ( (uplinkStat.usedMstrSlots + uplinkStat.usedMrtrSlots) / N_ ) * log10( (LB_1_ + (N_-1) * LB_2_) / ((N_-1) * LB_2_) + 1) / log10(2);


	if ( RB_i - B_req_i >= 0  &&  AvailableBandwidth - B_req_i >= TH_i ) {

		return true;
		debug2 ("Service is admitted");

	    } else {

		return false;
		debug2(" Service is rejected");
	    }

    } else {
	// downlink

	// Check if the call comes from a BS
	assert( mac_->getNodeType() == STA_BS);

	// get current frame utilization
	frameUsageStat_t downlinkStat = mac_->getScheduler()->getUplinkStatistic();

    // estimate demand of new service flow
    int uiuc = peer->getUIUC();

    // allocate resources for management data

    // Capacity for one Slot in byte
	    int slotCapacity = mac_->getPhy()->getSlotCapacity( mac_->getMap()->getUlSubframe()->getProfile( uiuc)->getEncoding(), UL_);
	    printf("Current Slot Capacity %d for Peer %d \n", slotCapacity, peer->getAddr());

	double MSTR_req_i = serviceFlowQosSet->getMaxSustainedTrafficRate(); // MSTR for request of a user i
	double MRTR_req_i = serviceFlowQosSet->getMinReservedTrafficRate();  // MRTR for request of a user i

	// The remaining bandwidth for one user
	double RB_i =  2 * (MRTR_req_i + beta_ * ( MSTR_req_i - MRTR_req_i ));

	// available bandwidth in the whole bandwidth
	double AvailableBandwidthSlots = downlinkStat.totalNbOfSlots - ( downlinkStat.usedMrtrSlots + downlinkStat.usedMrtrSlots * 0.1);
    double AvailableBandwidth = AvailableBandwidthSlots * slotCapacity * 8 / mac_->getFrameDuration();

	// Bandwidth requirement of a new service for user i
	double B_req_i = MRTR_req_i + beta_ * ( MSTR_req_i - MRTR_req_i );

	// Bandwidth Acquirement Ratio (BAR) (the user is forbidden to send any connection request when his BAR is reach 0.85)
    // double eta_i =  1 - ( RB_i / LB_1_);

	// The admitting threshold for user Ui
	double TH_i = ( (downlinkStat.usedMstrSlots + downlinkStat.usedMrtrSlots) / N_ ) * log10( (LB_1_ + (N_-1) * LB_2_) / ((N_-1) * LB_2_) + 1) / log10(2);


	if ( RB_i - B_req_i >= 0  &&  AvailableBandwidth - B_req_i >= TH_i ) {

		return true;
		debug2 ("Service is admitted");

	    } else {

		return false;
		debug2(" Service is rejected");
	    }

    }

}

