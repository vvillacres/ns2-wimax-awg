/* This software was developed at the National Institute of Standards and
 * Technology by employees of the Federal Government in the course of
 * their official duties. Pursuant to title 17 Section 105 of the United
 * States Code this software is not subject to copyright protection and
 * is in the public domain.
 * NIST assumes no responsibility whatsoever for its use by other parties,
 * and makes no guarantees, expressed or implied, about its quality,
 * reliability, or any other characteristic.
 * <BR>
 * We would appreciate acknowledgement if the software is used.
 * <BR>
 * NIST ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS" CONDITION AND
 * DISCLAIM ANY LIABILITY OF ANY KIND FOR ANY DAMAGES WHATSOEVER RESULTING
 * FROM THE USE OF THIS SOFTWARE.
 * </PRE></P>
 * @author  rouil
 */

#include "ipdestclassifier.h"
#include "mac802_16.h"
#include "scheduling/wimaxscheduler.h"


//vr@tud debug
#define DEBUG_EXT
#ifdef DEBUG_EXT
#define debug_ext printf
#else
#define debug_ext(arg1,...)
#endif

/**
 * TCL Hooks for the simulator for classifier
 */
static class IPDestClassifierClass : public TclClass
{
public:
    IPDestClassifierClass() : TclClass("SDUClassifier/IPDest") {}
    TclObject* create(int, const char*const*) {
        return (new IPDestClassifier());

    }
} class_ipdestclassifier;

/**
 * Create a classifier in the given mac
 * Constructor to be used by TCL
 */
IPDestClassifier::IPDestClassifier (): SDUClassifier ()
{

}


/**
 * Classify a packet and return the CID to use (or -1 if unknown)
 * @param p The packet to classify
 * @return The CID or -1
 */
int IPDestClassifier::classify (Packet * p)
{
    // debug
	debug_ext("IPDestClassifer PacketSize %d MAC Addr %d\n", HDR_CMN(p)->size(), mac_->addr());

    struct hdr_mac *dh = HDR_MAC(p);
    int dst = dh->macDA();
    mac_->debug ("At %f in Mac %d IPDestClassifier classifying packet for %d(size=%d, type=%s)\n",\
                 NOW, mac_->addr(), dst, HDR_CMN(p)->size(), packet_info.name(HDR_CMN(p)->ptype()));
    //here we look at the list of peer nodes until we find the one with
    //the same destination address. Then we return its data communication

    //if broadcast, then send to broadcast CID
    if (dst == -1) {
        if (mac_->getNodeType ()==STA_BS) {
            return BROADCAST_CID;
        } else {
            //I am a MN, check if I am connected
            PeerNode *n = mac_->getPeerNode_head ();
            //i should classify depending on the packet type.TBD
            if (n && n->getSecondary(OUT_CONNECTION)) {
            	// debug
            	debug_ext("find the outgoing secondary connection %d.\n", n->getSecondary(OUT_CONNECTION)->get_cid());
                return n->getSecondary(OUT_CONNECTION)->get_cid();
            }
        }
    }

    for (PeerNode *n = mac_->getPeerNode_head (); n ; n=n->next_entry()) {
        printf ("Checking peer %d for %d\n", n->getAddr(), dst);
        if (dst == n->getAddr ()) {
            switch (HDR_CMN(p)->ptype()) {
            case PT_ARP:
            case PT_MESSAGE: //DSDV routing messages
#ifdef USE_802_21
            case PT_RRED:
            case PT_RADS:
            case PT_RSOL:
#endif
                if (n->getSecondary(OUT_CONNECTION)) {
                	// debug
                	debug_ext("find the outgoing secondary connection %d.\n", n->getSecondary(OUT_CONNECTION)->get_cid());
                    return n->getSecondary(OUT_CONNECTION)->get_cid();
                }
                break;

            default:

            	int portNumber = 0;
            	// TODO: Check for different scenarios
            	if (mac_->getNodeType () == STA_BS) {
            		// downlink connection -> take destination port
            		portNumber = HDR_IP(p)->dport();
            	} else {
            		// uplink connection -> take source port
            		portNumber = HDR_IP(p)->sport() - n->getNbOutDataCon();
            	}

            	// debug
                printf("IPDestClassifier Src Addr %d, Src Port %d, Dest Addr %d, Dest Port %d \n", HDR_IP(p)->saddr(), HDR_IP(p)->sport(), HDR_IP(p)->daddr(), HDR_IP(p)->dport());
                printf("MAC Instants %d NbPeers %d \n", mac_->getNodeType(), mac_->getNbPeerNodes());
                printf("Number of Outgoing Connections %d, Number of Incoming Connections %d \n", n->getNbOutDataCon(), n->getNbInDataCon());

                if (n->getOutDataCon( portNumber)) {
                    // debug
                	debug_ext("find the outcoming data connection %d.\n", n->getOutDataCon( portNumber)->get_cid());
                    return n->getOutDataCon( portNumber)->get_cid();
                } else { //this node is not ready to send data
                    // debug
                	debug_ext("cannt find the outcoming data connection.\n");
                    break;
                }
            }
        }
    }
    return -1;
}
