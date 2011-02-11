
#include "faircac.h"
#include "serviceflowhandler.h"
#include "mac802_16.h"
#include "serviceflowqosset.h"
#include "serviceflow.h"

#include "admissioncontrolinterface.h"

FairCAC::FairCAC ( Mac802_16 * mac,

		  double RB_i,      // The remaining bandwidth

	      double MSTR_req_i,  // MSTR for request of a user i
	      double MRTR_req_i,  // MRTR for request of a user i

	      float beta,   // The reserved bandwidth ratio factor

          float B_a,   // available bandwidth in the whole bandwidth

          double LB_1,  // user i = 1
          double LB_2,  // user j = 2...9

          int N  // Number of users
          ) : AdmissionControlInterface ( mac)

{

}

// Bandwidth requirement of a new service for user i
double FairCAC::B_req_i()
{
    B_req_i_ = MRTR_req_i_ + beta_ * ( MSTR_req_i_ - MRTR_req_i_ );
	return B_req_i_;
}

// Bandwidth Acquirement Ratio (BAR)
double FairCAC::eta_i()
{
    eta_i_ =  1 - ( RB_i_ / LB_1_);
    return eta_i_;
}


// The admitting threshold for user Ui
float FairCAC::TH_i()
{
    TH_i_ = (MSTR_req_i_ + MRTR_req_i_) *log10( (LB_1_ + (N_-1) * LB_2_) / ((N_-1) * LB_2_) + 1) / log10(2);
    return TH_i_;
}



bool FairCAC::checkAdmission( ServiceFlowQosSet * serviceFlowQosSet)
{

	if ( RB_i_ - B_req_i_ >= 0  &&  B_a_ - B_req_i_ >= TH_i_ ) {

		return true;
		debug2 ("Service is admitted");

	    } else {

		return false;
		debug2(" Service is rejected");
	    }
}

