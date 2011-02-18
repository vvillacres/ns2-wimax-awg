
#include "thresholdbasedcac.h"

#include "serviceflow.h"
#include "peernode.h"
#include "serviceflowqosset.h"
#include "mac802_16.h"
#include "wimaxscheduler.h"





ThresholdBasedCAC::ThresholdBasedCAC (Mac802_16 * mac,
										float thresholdUgs,
										float thresholdErtPS,
										float thresholdRtPS,
										float thresholdNrtPS,
										float thresholdBe) : AdmissionControlInterface (mac)
{
thresholdUgs_   = thresholdUgs;
thresholdErtPs_ = thresholdErtPS;
thresholdRtPs_  = thresholdRtPS;
thresholdNrtPs_ = thresholdNrtPS;
thresholdBe_    = thresholdBe;
}

bool ThresholdBasedCAC::checkAdmission( ServiceFlow * serviceFlow, PeerNode * peer)
{

/*	struct frameUsageStat_t {
	    double totalNbOfSlots;
	    double usedMrtrSlots;
	    double usedMstrSlots;
	};
*/
	Dir_t direction = serviceFlow->getDirection();
	ServiceFlowQosSet * serviceFlowQosSet = serviceFlow->getQosSet();

	// thresholds conditions for reserved bandwidth

	if ( direction == UL) {
		// uplink

		// Check if the call comes from a BS
		assert( mac_->getNodeType() == STA_BS);

		// get current frame utilization
		frameUsageStat_t uplinkStat = mac_->getScheduler()->getUplinkStatistic();
	    double usage = ( uplinkStat.usedMrtrSlots + uplinkStat.usedMrtrSlots * 0.1) / uplinkStat.totalNbOfSlots ;

	    // estimate demand of new service flow
	    int uiuc = peer->getUIUC();

        // allocate resources for management data

	    // Capacity for one Slot in byte
   	    int slotCapacity = mac_->getPhy()->getSlotCapacity( mac_->getMap()->getUlSubframe()->getProfile( uiuc)->getEncoding(), UL_);
   	    printf("Current Slot Capacity %d for Peer %d \n", slotCapacity, peer->getAddr());

   	    double ServiceRequestSlots = ((serviceFlowQosSet->getMinReservedTrafficRate()) / 8) / slotCapacity * mac_->getFrameDuration();

   	    double ServiceRequestPercent = ( ServiceRequestSlots / uplinkStat.totalNbOfSlots );

		UlGrantSchedulingType_t schedulingType = serviceFlowQosSet->getUlGrantSchedulingType();
		switch( schedulingType) {
		case UL_UGS:
			if ( (usage + ServiceRequestPercent) <= thresholdUgs_ ) {
				return true;  // UL_UGS service is admitted
			} else {
				return false; // UL_UGS service is rejected
			}

			break;

		case UL_ertPS:
			if ( (usage + ServiceRequestPercent) <= thresholdErtPs_ ) {
				return true;   // UL_ertPS service is admitted
			} else {
				return false;  // UL_ertPS service is rejected
			}

			break;

		case UL_rtPS:
			if ( (usage + ServiceRequestPercent) <= thresholdRtPs_ ) {
				return true;   // UL_rtPS service is admitted
			} else {
				return false;  // UL_rtPS service is rejected
			}

			break;

		case UL_nrtPS:
			if ( (usage + ServiceRequestPercent) <= thresholdNrtPs_ ) {
				return true;   // UL_nrtPS service is admitted
			} else {
				return false;  // UL_nrtPS service is rejected
			}

			break;

		case UL_BE:
			if ( (usage + ServiceRequestPercent) <= thresholdBe_ ) {
				return true;   //  UL_BE service is admitted
			} else {
				return false;  //  UL_BE service is rejected
			}

			break;

		default:

			break;
	    }

	} else  {
		// downlink

		// Check if the call comes from a BS
		assert( mac_->getNodeType() == STA_BS);

		// get current frame utilization
		frameUsageStat_t downlinkStat = mac_->getScheduler()->getDownlinkStatistic();
		double usage = ( downlinkStat.usedMrtrSlots + downlinkStat.usedMrtrSlots * 0.1) / downlinkStat.totalNbOfSlots ;

	    // estimate demand of new service flow
	    int diuc = peer->getDIUC();
	    int slotCapacity = mac_->getPhy()->getSlotCapacity( mac_->getMap()->getDlSubframe()->getProfile( diuc)->getEncoding(), DL_);
   	    printf("Current Slot Capacity %d for Peer %d \n", slotCapacity, peer->getAddr());

   	    double ServiceRequestSlots = ((serviceFlowQosSet->getMinReservedTrafficRate()) / 8) / slotCapacity * mac_->getFrameDuration();

   	    double ServiceRequestPercent = ( ServiceRequestSlots / downlinkStat.totalNbOfSlots );

		DataDeliveryServiceType_t schedulingType = serviceFlowQosSet->getDataDeliveryServiceType();
		switch( schedulingType) {
		case DL_UGS:
			if ( (usage + ServiceRequestPercent) <= thresholdUgs_ ) {
				return true;   // DL_UGS service is admitted
			} else {
				return false;  // DL_UGS service is rejected
			}

			break;

		case DL_ERTVR:
			if ( (usage + ServiceRequestPercent) <= thresholdErtPs_ ) {
				return true;   // DL_ertPS service is admitted
			} else {
				return false;  // DL_ertPS service is rejected
			}

			break;

		case DL_RTVR:
			if ( (usage + ServiceRequestPercent) <= thresholdRtPs_ ) {
				return true;   // DL_rtPS service is admitted
			} else {
				return false;  // DL_rtPS service is rejected
			}

			break;

		case DL_NRTVR:
			if ( (usage + ServiceRequestPercent) <= thresholdNrtPs_ ) {
				return true;   // DL_nrtPS service is admitted
			} else {
				return false;  // DL_nrtPS service is rejected
			}

			break;

		case DL_BE:
			if ( (usage + ServiceRequestPercent) <= thresholdBe_ ) {
				return true;   // DL_BE service is admitted
			} else {
				return false;  // DL_BE service is rejected
			}

			break;

		default:
			break;
		}
    }


	// default for compiler
	return false;
}
