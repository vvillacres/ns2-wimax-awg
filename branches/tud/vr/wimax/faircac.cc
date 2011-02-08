
#include "faircac.h"
#include "serviceflowhandler.h"
#include "mac802_16.h"
#include "serviceflowqosset.h"
#include "serviceflow.h"


FairCAC::FairCAC (Mac802_16 * mac) : AdmissionControlInterface ( mac)
{

}


bool FairCAC::checkAdmission( ServiceFlowQosSet * serviceFlowQosSet)
{
	return true;
}
