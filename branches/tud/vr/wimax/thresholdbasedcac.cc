
#include "thresholdbasedcac.h"
#include "serviceflowqosset.h"
#include "mac802_16.h"
#include "serviceflow.h"
#include "serviceflowhandler.h"

ThresholdBasedCAC::ThresholdBasedCAC (Mac802_16 * mac) : AdmissionControlInterface (mac)
{

}

bool ThresholdBasedCAC::checkAdmission( ServiceFlowQosSet * serviceFlowQosSet)
{


/*
    Dir_t RichtungUplink = UL;
    Dir_t RichtungDownlink = DL;

	if (used_bandwidth <= threshold_ugs && RichtungUplink) {

   // UGS
		UlGrantSchedulingType_t	ulGrantSchedulingType = UL_UGS;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("UL_UGS service is Admitted");

	   } else if (used_bandwidth <= threshold_ugs && RichtungDownlink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = DL_UGS;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("DL_UGS service is Admitted");

   // ertPS
	   } else if (used_bandwidth <= threshold_ertps && RichtungUplink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = UL_ertPS;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("UL_ertPS service is Admitted");

	   } else if (used_bandwidth <= threshold_ertps && RichtungDownlink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = DL_ERTVR;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("DL_ertPS service is Admitted");

   // rtPS
	   } else if (used_bandwidth <= threshold_rtps && RichtungUplink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = UL_rtPS;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("UL_rtPS service is Admitted");

	   } else if (used_bandwidth <= threshold_rtps && RichtungDownlink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = DL_RTVR;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("DL_rtPS service is Admitted");

    // nrtPS
	   } else if (used_bandwidth <= threshold_nrtps && RichtungUplink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = UL_nrtPS;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("UL_nrtPS service is Admitted");

	   } else if (used_bandwidth <= threshold_nrtps && RichtungDownlink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = DL_NRTVR;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("DL_nrtPS service is Admitted");

     // BE
	   } else if (used_bandwidth <= threshold_nrtps && RichtungUplink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = UL_BE;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("UL_BE service is Admitted");

	   } else if (used_bandwidth <= threshold_nrtps && RichtungDownlink) {

		UlGrantSchedulingType_t	ulGrantSchedulingType = DL_BE;
		result = serviceFlowHandler_->addStaticFlow(argc, argv);
		return result;
	    debug2("DL_BE service is Admitted");
	    } else {

	    }
	    */
}


