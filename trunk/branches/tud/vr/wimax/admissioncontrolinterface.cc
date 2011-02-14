
#include "admissioncontrolinterface.h"
#include "mac802_16.h"
#include "serviceflow.h"
#include "peernode.h"

/*
static class AdmissionControlInterfaceClass : public TclClass {
public:
	   AdmissionControlInterfaceClass() : TclClass("AC") {}
	   TclObject* creat(int, const char*const*) {
		   return(new AdmissionControlInterface());
	   }
} class_AdmissionControlInterface;
*/

AdmissionControlInterface::AdmissionControlInterface(Mac802_16 * mac){
	// initial reference mac layer
	mac_ = mac;

}


AdmissionControlInterface::~AdmissionControlInterface(){
	// nothing to do
}



bool AdmissionControlInterface::checkAdmission( ServiceFlow * serviceFlow, PeerNode * peer)
{

	// default implementation
	return true;


}
