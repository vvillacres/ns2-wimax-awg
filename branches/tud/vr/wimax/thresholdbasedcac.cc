
#include "thresholdbasedcac.h"
#include "serviceflowqosset.h"
#include "mac802_16.h"
#include "wimaxscheduler.h"
#include "serviceflow.h"
#include "serviceflowhandler.h"

#include "admissioncontrolinterface.h"



ThresholdBasedCAC::ThresholdBasedCAC (Mac802_16 * mac,
										float thresholdUgs,
										float thresholdErtPS,
										float thresholdRtPS,
										float thresholdNrtPS,
										float thresholdBe) : AdmissionControlInterface (mac)
{

}

bool ThresholdBasedCAC::checkAdmission( ServiceFlowQosSet * serviceFlowQosSet)
{

/*	struct frameUsageStat_t {
	    double totalNbOfSlots;
	    double usedMrtrSlots;
	    double usedMstrSlots;
	};
*/



	// thresholds conditions for reserved bandwidth

	if ( serviceFlowQosSet->getDataDeliveryServiceType() == DL_NONE) {
		// uplink

		// TODO: Check if the call comes form a BS or SS MAC
		// Inform your supervisor if this assertation fails
		assert( mac_->getNodeType() == STA_BS);

		frameUsageStat_t uplinkStat = mac_->getScheduler()->getUplinkStatistic();
	    double usage = ( uplinkStat.usedMrtrSlots + uplinkStat.usedMrtrSlots * 0.1) / uplinkStat.totalNbOfSlots ;

		UlGrantSchedulingType_t schedulingType = serviceFlowQosSet->getUlGrantSchedulingType();
		switch( schedulingType) {
		case UL_UGS:
			if (usage <= thresholdUgs_ ) {
				return true;  // UL_UGS service is admitted
			} else {
				return false; // UL_UGS service is rejected
			}

			break;

		case UL_ertPS:
			if (usage <= thresholdErtPs_ ) {
				return true;   // UL_ertPS service is admitted
			} else {
				return false;  // UL_ertPS service is rejected
			}

			break;

		case UL_rtPS:
			if (usage <= thresholdRtPs_ ) {
				return true;   // UL_rtPS service is admitted
			} else {
				return false;  // UL_rtPS service is rejected
			}

			break;

		case UL_nrtPS:
			if (usage <= thresholdNrtPs_ ) {
				return true;   // UL_nrtPS service is admitted
			} else {
				return false;  // UL_nrtPS service is rejected
			}

			break;

		case UL_BE:
			if (usage <= thresholdBe_ ) {
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

		// TODO: Check if the call comes from a BS or SS MAC
		// Inform your supervisor if this assertation fails
		assert( mac_->getNodeType() == STA_BS);

		frameUsageStat_t downlinkStat = mac_->getScheduler()->getDownlinkStatistic();
		double usage = ( downlinkStat.usedMrtrSlots + downlinkStat.usedMrtrSlots * 0.1) / downlinkStat.totalNbOfSlots ;

		DataDeliveryServiceType_t schedulingType = serviceFlowQosSet->getDataDeliveryServiceType();
		switch( schedulingType) {
		case DL_UGS:
			if (usage <= thresholdUgs_ ) {
				return true;   // DL_UGS service is admitted
			} else {
				return false;  // DL_UGS service is rejected
			}

			break;

		case DL_ERTVR:
			if (usage <= thresholdErtPs_ ) {
				return true;   // DL_ertPS service is admitted
			} else {
				return false;  // DL_ertPS service is rejected
			}

			break;

		case DL_RTVR:
			if (usage <= thresholdRtPs_ ) {
				return true;   // DL_rtPS service is admitted
			} else {
				return false;  // DL_rtPS service is rejected
			}

			break;

		case DL_NRTVR:
			if (usage <= thresholdNrtPs_ ) {
				return true;   // DL_nrtPS service is admitted
			} else {
				return false;  // DL_nrtPS service is rejected
			}

			break;

		case DL_BE:
			if (usage <= thresholdBe_ ) {
				return true;   // DL_BE service is admitted
			} else {
				return false;  // DL_BE service is rejected
			}

			break;

		default:
			break;
		}
	}

}
