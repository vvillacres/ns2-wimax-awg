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
 * @author  Richard Rouil
 * @modified by  Chakchai So-In
 * @modified vy Volker Richter TUD
 */

#include "bsscheduler.h"
#include "burst.h"
#include "dlburst.h"
#include "ulburst.h"
#include "random.h"
#include "wimaxctrlagent.h"
#include "mac802_16BS.h"
#include "mac802_16.h"
#include <stdlib.h>

// algorithm implementations
// traffic shaping
#include "trafficshapingaccurate.h"
#include "trafficshapingtswtcm.h"
#include "trafficshapingnone.h"


// scheduling algorithm downlink
#include "schedulingalgodualequalfill.h"
#include "schedulingalgodualedf.h"
#include "schedulingalgoproportionalfair.h"

// scheduling algorithm upling
#include "schedulingproportionalfairul.h"
#include "schedulingdualequalfillul.h"

// downlink burst mapping algorithm
#include "dlburstmappingsimple.h"
#include "dlburstmappingocsa.h"


// Ranging Region (cp. Table 376 IEEE 802.16-2009)
// 0b00: Initial ranging/Handover Ranging over two symbols
// 0b01: Initial ranging/Handover Ranging over four symbols
// 0b10: BR/periodic ranging over one symbol
// 0b11: BR/periodic ranging over three symbols

#define INITIAL_RANGING_SYM 2
#define BW_RANGING_SYM 1

#define CODE_SIZE 256
#define CDMA_SLOTS 16
#define CDMA_6SUB 6
#define MAX_CONN 2048
#define ROLL_DOWN_FACTOR_1 2
#define ROLL_DOWN_FACTOR_2 3

// to avoid error of grant and polling interval
#define TIMEERRORTOLERANCE 1

#define DEBUG_EXT
//vr@tud debug
#ifdef DEBUG_EXT
#define debug_ext printf
#else
#define debug_ext(arg1,...)
#endif

//vr@tud dedug for bsscheduler
#define debug10 printf
#define debug2 printf

//Scheduler allocates CBR every frame
#define UGS_AVG

/**
 * Bridge to TCL for BSScheduler
 */
int frame_number=0;

//This structure is used for virtual allocation and "index_burst" is used for #counting bursts
int index_burst=0;
struct virtual_burst {
    int alloc_type; //0 = DL_MAP, 1 = UL_MAP, 2 = DCD, 3 = UCD, 4 = other broadcast, 5 = DL_burst, 6 = UL_burst
    int cid;
    int n_cid;
    int iuc;
    Ofdm_mod_rate mod;
    bool preamble;
    int symboloffset;
    int suboffset;
    int numsymbol;
    int numsub;
    int numslots;
    float byte;
    int rep;
    int dl_ul;    //0 = DL, 1 = UL
    int ie_type;
} virtual_alloc[MAX_MAP_IE], ul_virtual_alloc[MAX_MAP_IE];


static class BSSchedulerClass : public TclClass
{
public:
    BSSchedulerClass() : TclClass("WimaxScheduler/BS") {}
    TclObject* create(int, const char*const*) {
        return (new BSScheduler());

    }
} class_bsscheduler;

/*
 * Create a scheduler
 */
BSScheduler::BSScheduler () : WimaxScheduler ()
{
    debug2 ("BSScheduler created\n");
    default_mod_ = OFDM_QPSK_1_2;
    //bind ("dlratio_", &dlratio_);
    bind("repetition_code_", &repetition_code_);
    bind("init_contention_size_", &init_contention_size_);
    bind("bw_req_contention_size_", &bw_req_contention_size_);


    nextDL_ = -1;
    nextUL_ = -1;

    // create default algorithm objects

    // traffic policing
    trafficShapingAlgorithm_ =  NULL;

    // Scheduling Algorithm for Downlink Direction
    dlSchedulingAlgorithm_ = NULL;

    // Scheduling Alorithm for Uplink Direction
    ulSchedulingAlgorithm_ = NULL;

    // Burst Mapping Algorithm for Downlink Direction
    dlBurstMappingAlgorithm_ = NULL;

}

/*
 * Delete a scheduler
 */
BSScheduler::~BSScheduler ()
{
    // delate algorithm objects
    delete trafficShapingAlgorithm_;
    delete dlSchedulingAlgorithm_;
    delete ulSchedulingAlgorithm_;
    delete dlBurstMappingAlgorithm_;
}

/**
 * Return the MAC casted to BSScheduler
 * @return The MAC casted to BSScheduler
 */
Mac802_16BS* BSScheduler::getMac()
{
    return (Mac802_16BS*)mac_;
}

/*
 * Interface with the TCL script
 * @param argc The number of parameter
 * @param argv The list of parameters
 */
int BSScheduler::command(int argc, const char*const* argv)
{
    if (argc == 3) {
        if (strcmp(argv[1], "set-default-modulation") == 0) {
            if (strcmp(argv[2], "OFDM_BPSK_1_2") == 0) {
                default_mod_ = OFDM_BPSK_1_2;
            } else if (strcmp(argv[2], "OFDM_QPSK_1_2") == 0) {
                default_mod_ = OFDM_QPSK_1_2;
            } else if (strcmp(argv[2], "OFDM_QPSK_3_4") == 0) {
                default_mod_ = OFDM_QPSK_3_4;
            } else if (strcmp(argv[2], "OFDM_16QAM_1_2") == 0) {
                default_mod_ = OFDM_16QAM_1_2;
            } else if (strcmp(argv[2], "OFDM_16QAM_3_4") == 0) {
                default_mod_ = OFDM_16QAM_3_4;
            } else if (strcmp(argv[2], "OFDM_64QAM_2_3") == 0) {
                default_mod_ = OFDM_64QAM_2_3;
            } else if (strcmp(argv[2], "OFDM_64QAM_3_4") == 0) {
                default_mod_ = OFDM_64QAM_3_4;
            } else {
                return TCL_ERROR;
            }
            return TCL_OK;

        } else if (strcmp(argv[1], "set-init-contention-size") == 0) {
        	init_contention_size_ = atoi (argv[2]);
        	return TCL_OK;

        } else if (strcmp(argv[1], "set-bw-req-contention-size") == 0) {
        	bw_req_contention_size_ = atoi (argv[2]);
        	return TCL_OK;

        } else if (strcmp(argv[1], "set-repetition-code") == 0) {
        	repetition_code_ = atoi (argv[2]);
        	return TCL_OK;


        } else if (strcmp(argv[1], "set-traffic-shaping") == 0) {
            if (strcmp(argv[2], "accurate") == 0) {
                // delete previous algorithm
                delete trafficShapingAlgorithm_;
                // create new alogrithm object
                trafficShapingAlgorithm_ =  new TrafficShapingAccurate( mac_->getFrameDuration());
                printf("New Traffic Shaping Algorithm: Traffic Shaping Accurate \n");
            } else if (strcmp(argv[2], "tswtcm") == 0) {
                // delete previous algorithm
                delete trafficShapingAlgorithm_;
                // create new alogrithm object
                trafficShapingAlgorithm_ =  new TrafficShapingTswTcm( mac_->getFrameDuration());
                printf("New Traffic Shaping Algorithm: Time Sliding Window Three Color Marker \n");
            } else if (strcmp(argv[2], "none") == 0) {
                // delete previous algorithm
                delete trafficShapingAlgorithm_;
                // create new alogrithm object
                trafficShapingAlgorithm_ =  new TrafficShapingNone( mac_->getFrameDuration());
                printf("New Traffic Shaping Algorithm: None \n");
            } else {
                fprintf(stderr, "Specified Traffic Shaping Algorithm NOT found ! \n");
                return TCL_ERROR;
            }
            return TCL_OK;
        } else if (strcmp(argv[1], "set-downlink-scheduling") == 0) {
            if (strcmp(argv[2], "dual-equal-fill") == 0) {
                // delete previous algorithm
                delete dlSchedulingAlgorithm_;
                // create new alogrithm object
                dlSchedulingAlgorithm_ =  new SchedulingAlgoDualEqualFill();
                printf("New Downlink Scheduling Algorithm: Dual Equal Fill \n");
            } else if (strcmp(argv[2], "dual-edf") == 0) {
                // delete previous algorithm
                delete dlSchedulingAlgorithm_;
                // create new alogrithm object
                dlSchedulingAlgorithm_ =  new SchedulingAlgoDualEdf();
                printf("New Downlink Scheduling Algorithm: Dual Earliest Deadline First \n");
            } else if (strcmp(argv[2], "proportional-fair") == 0) {
                // delete previous algorithm
                delete dlSchedulingAlgorithm_;
                // create new alogrithm object
                dlSchedulingAlgorithm_ =  new SchedulingAlgoProportionalFair();
                printf("New Downlink Scheduling Algorithm: ProportionalFair \n");
            } else {
                fprintf(stderr, "Specified Downlink Scheduling Policing Algorithm NOT found ! \n");
                return TCL_ERROR;
            }
            return TCL_OK;
        } else if (strcmp(argv[1], "set-uplink-scheduling") == 0) {
            if (strcmp(argv[2], "dual-equal-fill") == 0) {
                // delete previous algorithm
                delete ulSchedulingAlgorithm_;
                // create new alogrithm object
                ulSchedulingAlgorithm_ =  new SchedulingAlgoDualEqualFill();
                printf("New Uplink Scheduling Algorithm: Dual Equal Fill");
            } else if (strcmp(argv[2], "dual-edf") == 0) {
                // delete previous algorithm
                delete ulSchedulingAlgorithm_;
                // create new alogrithm object
                ulSchedulingAlgorithm_ =  new SchedulingAlgoDualEdf();
                printf("New Uplink Scheduling Algorithm: Dual Earliest Deadline First");
            } else if (strcmp(argv[2], "proportional-fair") == 0) {
                // delete previous algorithm
                delete ulSchedulingAlgorithm_;
                // create new alogrithm object
                ulSchedulingAlgorithm_ =  new SchedulingAlgoProportionalFair();
                printf("New Uplink Scheduling Algorithm: ProportionalFair");
            } else {
                fprintf(stderr, "Specified Downlink Scheduling Policing Algorithm NOT found !");
                return TCL_ERROR;
            }
            return TCL_OK;
        }

    }
    return TCL_ERROR;
}

/**
 * Initializes the scheduler
 */
void BSScheduler::init ()
{
    WimaxScheduler::init();

    // Create default alogrithm objects

    // traffic policing
    if ( !trafficShapingAlgorithm_) {
    	trafficShapingAlgorithm_ =  new TrafficShapingAccurate( mac_->getFrameDuration());
    }

    // Scheduling Algorithm for Downlink Direction
    if ( !dlSchedulingAlgorithm_) {
    	dlSchedulingAlgorithm_ = new SchedulingAlgoDualEqualFill();
    }

    // Scheduling Alorithm for Uplink Direction
    if ( !ulSchedulingAlgorithm_) {
    	//ulSchedulingAlgorithm_ = new SchedulingProportionalFairUl();
    	ulSchedulingAlgorithm_ = new SchedulingDualEqualFillUl();
    }

    // Downlink Burst Mapping Algorithm
    if ( !dlBurstMappingAlgorithm_) {
    	 //dlBurstMappingAlgorithm_ = new DlBurstMappingOcsa( mac_, this); // OSCA
    	 dlBurstMappingAlgorithm_ = new DlBurstMappingSimple( mac_, this); // Simple

    	 // Change Line 1226 according to the choosen algorithm
    }

    printf("Algorithm Objects created \n");

    // If the user did not set the profiles by hand, let's do it
    // automatically
    if (getMac()->getMap()->getDlSubframe()->getProfile (DIUC_PROFILE_1)==NULL) {
        //#ifdef SAM_DEBUG
        debug2 ("Adding profiles\n");
        //#endif
        Profile *p = getMac()->getMap()->getDlSubframe()->addProfile ((int)round((getMac()->getPhy()->getFreq()/1000)), OFDM_BPSK_1_2);
        p->setIUC (DIUC_PROFILE_1);
        p = getMac()->getMap()->getDlSubframe()->addProfile ((int)round((getMac()->getPhy()->getFreq()/1000)), OFDM_QPSK_1_2);
        p->setIUC (DIUC_PROFILE_2);
        p = getMac()->getMap()->getDlSubframe()->addProfile ((int)round((getMac()->getPhy()->getFreq()/1000)), OFDM_QPSK_3_4);
        p->setIUC (DIUC_PROFILE_3);
        p = getMac()->getMap()->getDlSubframe()->addProfile ((int)round((getMac()->getPhy()->getFreq()/1000)), OFDM_16QAM_1_2);
        p->setIUC (DIUC_PROFILE_4);
        p = getMac()->getMap()->getDlSubframe()->addProfile ((int)round((getMac()->getPhy()->getFreq()/1000)), OFDM_16QAM_3_4);
        p->setIUC (DIUC_PROFILE_5);
        p = getMac()->getMap()->getDlSubframe()->addProfile ((int)round((getMac()->getPhy()->getFreq()/1000)), OFDM_64QAM_2_3);
        p->setIUC (DIUC_PROFILE_6);
        p = getMac()->getMap()->getDlSubframe()->addProfile ((int)round((getMac()->getPhy()->getFreq()/1000)), OFDM_64QAM_3_4);
        p->setIUC (DIUC_PROFILE_7);



        // p = getMac()->getMap()->getUlSubframe()->addProfile (0, OFDM_QPSK_1_2);
        //p->setIUC (UIUC_FFB_REGION);
//	debug2 ("TOTO Profile Object @ %x  Encoding %d\n",
//				getMac()->getMap()->getUlSubframe()->getProfile(UIUC_FFB_REGION),getMac()->getMap()->getUlSubframe()->getProfile(UIUC_FFB_REGION)->getEncoding());

        p = getMac()->getMap()->getUlSubframe()->addProfile (0, default_mod_);
        p->setIUC (UIUC_INITIAL_RANGING);
        p = getMac()->getMap()->getUlSubframe()->addProfile (0, default_mod_);
        p->setIUC (UIUC_REQ_REGION_FULL);

        p = getMac()->getMap()->getUlSubframe()->addProfile (0, default_mod_);
        p->setIUC (UIUC_EXT_UIUC);

        p = getMac()->getMap()->getUlSubframe()->addProfile (0, OFDM_BPSK_1_2);
        p->setIUC (UIUC_PROFILE_1);
        p = getMac()->getMap()->getUlSubframe()->addProfile (0, OFDM_QPSK_1_2);
        p->setIUC (UIUC_PROFILE_2);
        p = getMac()->getMap()->getUlSubframe()->addProfile (0, OFDM_QPSK_3_4);
        p->setIUC (UIUC_PROFILE_3);
        p = getMac()->getMap()->getUlSubframe()->addProfile (0, OFDM_16QAM_1_2);
        p->setIUC (UIUC_PROFILE_4);
        p = getMac()->getMap()->getUlSubframe()->addProfile (0, OFDM_16QAM_3_4);
        p->setIUC (UIUC_PROFILE_5);
        p = getMac()->getMap()->getUlSubframe()->addProfile (0, OFDM_64QAM_2_3);
        p->setIUC (UIUC_PROFILE_6);
        p = getMac()->getMap()->getUlSubframe()->addProfile (0, OFDM_64QAM_3_4);
        p->setIUC (UIUC_PROFILE_7);
    }

    //init contention slots
    ContentionSlot *slot = getMac()->getMap()->getUlSubframe()->getRanging ();
//  slot->setSize (getInitRangingopportunity ());
    slot->setSize (getMac()->macmib_.init_contention_size);
    slot->setBackoff_start (getMac()->macmib_.rng_backoff_start);
    slot->setBackoff_stop (getMac()->macmib_.rng_backoff_stop);

    slot = getMac()->getMap()->getUlSubframe()->getBw_req ();
//  slot->setSize (getBWopportunity ());
    slot->setSize (getMac()->macmib_.bw_req_contention_size);
    slot->setBackoff_start (getMac()->macmib_.bw_backoff_start);
    slot->setBackoff_stop (getMac()->macmib_.bw_backoff_stop);

}

/**
 * Compute and return the bandwidth request opportunity size
 * @return The bandwidth request opportunity size
 */
int BSScheduler::getBWopportunity ()
{
    int nbPS = BW_REQ_PREAMBLE * getMac()->getPhy()->getSymbolPS();
    //add PS for carrying header
    nbPS += (int) round((getMac()->getPhy()->getTrxTime (HDR_MAC802_16_SIZE, getMac()->getMap()->getUlSubframe()->getProfile(UIUC_REQ_REGION_FULL)->getEncoding())/getMac()->getPhy()->getPS ()));
    debug2 ("BWopportunity size=%d\n", nbPS);
    debug10 ("BWopportunity size 1 oppo :%d nbPS, BW_PREAMBLE :%d + HDR_MAC802.16 :%d, with UIUC_REQ_REGION_FULL :%d\n", nbPS, BW_REQ_PREAMBLE, HDR_MAC802_16_SIZE, UIUC_REQ_REGION_FULL);
    return nbPS;
}

/**
 * Compute and return the initial ranging opportunity size
 * @return The initial ranging opportunity size
 */
int BSScheduler::getInitRangingopportunity ()
{
    int nbPS = INIT_RNG_PREAMBLE * getMac()->getPhy()->getSymbolPS();
    debug2("nbPS = %d " , nbPS);
    //add PS for carrying header
    nbPS += (int) round((getMac()->getPhy()->getTrxTime (RNG_REQ_SIZE+HDR_MAC802_16_SIZE, getMac()->getMap()->getUlSubframe()->getProfile(UIUC_INITIAL_RANGING)->getEncoding())/getMac()->getPhy()->getPS ()));
    debug2 ("Init ranging opportunity size=%d\n", nbPS);
    debug10 ("InitRanging opportunity size 1 oppo :%d nbPS, INIT_RNG_PREAMBLE :%d + RNG_REQ_SIZE :%d + HDR_MAC802.16 :%d, with UIUC_INITIAL_RANGING :%d\n", nbPS, INIT_RNG_PREAMBLE, RNG_REQ_SIZE, HDR_MAC802_16_SIZE, UIUC_INITIAL_RANGING);
    return nbPS;
}




/**
 * Schedule bursts/packets
 */
void BSScheduler::schedule ()
{
    debug2("\n\n======================BSScheduler::schedule () Begin ============================\n");
    //The scheduler will perform the following steps:
    //1-Clear UL map
    //2-Allocate CDMA-Ranging region for initial ranging (2 OFDMs) and bw-req (1 OFDMs)
    //  In this version we reserved a whold column resulting 5-subchannel waste
    //3-Allocate UL data allocation (all-cid) per-MS allocation
    //  In this version we do not consider horizontal stripping yet
    //4-Clear DL map
    //5-Allocate Burst for Broadcast message
    //  In this version we allocate one burst for each broadcast message type say DL_MAP, UL_MAP, DCD, UCD, other other broadcast message
    //6-Allocate DL data allocation (all-cid) per-cid allocation
    //7-Assign burst -> physical allocation
    //Note that, we do not simulate FCH in this version. We will do that in next version.

    // Packet *p;
    //  struct hdr_cmn *ch;
    //  double txtime; //tx time for some data (in second)
    //  int txtime_s;  //number of symbols used to transmit the data
    //  DlBurst *db;
    //  PeerNode *peer;


    printf("Simulation Time %g \n", NOW);

    // ARQ will be reviewed later vr@tud
/*
    // We will try to Fill in the ARQ Feedback Information now...
    PeerNode * peerNode;
    Packet * ph = NULL;
    Connection * basic ;
    Connection * OutData;
    Packet * pfb = NULL;
    hdr_mac802_16 *wimaxHdrMap ;
    u_int16_t   temp_num_of_acks = 0;
    bool out_datacnx_exists = false;
    for (Connection *n= mac_->getCManager ()->get_in_connection (); n; n=n->next_entry()) {
        if (n->getArqStatus () != NULL && n->getArqStatus ()->isArqEnabled() == 1) {
            if (!(n->getArqStatus ()->arq_feedback_queue_) || (n->getArqStatus ()->arq_feedback_queue_->length() == 0)) {
                continue;
            } else {
                peerNode = n->getPeerNode ();
                if (peerNode->getOutDataCon() != NULL && peerNode->getOutDataCon()->queueLength() != 0 && getMac()->isArqFbinDlData()) {
                    out_datacnx_exists = true;
                }

                if (out_datacnx_exists == false) {
                    //debug2("ARQ BS: Feedback in Basic Cid \n");
                    basic = peerNode->getBasic (OUT_CONNECTION);
                    pfb = mac_->getPacket ();
                    wimaxHdrMap= HDR_MAC802_16(pfb);
                    wimaxHdrMap->header.cid = basic->get_cid ();
                    wimaxHdrMap->num_of_acks = 0;
                } else {
                    debug2("ARQ BS : Feedback in data Cid \n");
                    OutData = peerNode->getOutDataCon();
                    pfb = OutData->dequeue ();
                    wimaxHdrMap= HDR_MAC802_16(pfb);
                    if (wimaxHdrMap->header.type_arqfb == 1) {
                        debug2("ARQ BS: Feedback already present, do nothing \n");
                        OutData->enqueue_head (pfb);
                        continue;
                    }
                    wimaxHdrMap->num_of_acks = 0;
                }

                //Dequeue is only once as one feedback in 5ms frame is permitted
                ph = n->getArqStatus ()->arq_feedback_queue_->deque();
                wimaxHdrMap->header.type_arqfb = 1;
                hdr_mac802_16 *wimaxHdr= HDR_MAC802_16(ph);
                for (temp_num_of_acks = 0; temp_num_of_acks < wimaxHdr->num_of_acks; temp_num_of_acks++) {
                    wimaxHdrMap->arq_ie[temp_num_of_acks].cid = wimaxHdr->arq_ie[temp_num_of_acks].cid;
                    wimaxHdrMap->arq_ie[temp_num_of_acks].last = wimaxHdr->arq_ie[temp_num_of_acks].last;
                    wimaxHdrMap->arq_ie[temp_num_of_acks].ack_type = wimaxHdr->arq_ie[temp_num_of_acks].ack_type;
                    wimaxHdrMap->arq_ie[temp_num_of_acks].fsn = wimaxHdr->arq_ie[temp_num_of_acks].fsn;
                }
                wimaxHdrMap->num_of_acks = wimaxHdr->num_of_acks;
                HDR_CMN(pfb)->size()	+= (wimaxHdr->num_of_acks * HDR_MAC802_16_ARQFEEDBK_SIZE);
                debug2("ARQ : In BSScheduler: Enqueueing an feedback cid: %d arq_ie->fsn:%d \n", wimaxHdr->arq_ie[0].cid,  wimaxHdr->arq_ie[0].fsn);
                if (out_datacnx_exists == false) {
                    // If I am here then the Ack packet has been created, so we will enqueue it in the Basic Cid
                    basic->enqueue (pfb);
                } else {
                    OutData->enqueue_head (pfb);

                }

                            }
        }
    }
               */

    // handel ARQ feedback Information
    sendArqFeedbackInformation();

    PeerNode * peernode = mac_->getPeerNode_head();
    if (peernode) {
        for (int i=0; i<mac_->getNbPeerNodes() ; i++) {
            //peer->setchannel(++Channel_num);
            peernode->setchannel((peernode->getchannel()) + 1);

            if (peernode->getchannel()>999)
                peernode->setchannel(2);

            peernode = peernode->next_entry();
        }
    }

    //====================== Calculation of the frame format =============================//

    OFDMAPhy *phy = mac_->getPhy();

    // number of physical slots shall be 14000 for 5 ms frame duration
    int nbPS = int( floor((mac_->getFrameDuration()/phy->getPS())));
    // number of physical slots after rtg and ttg ( rtg=ttg=231 according to wimax forum system evaluation methodology p. 23)
    int nbPSLeft = nbPS - mac_->phymib_.rtg - mac_->phymib_.ttg;
    // maximum number of OFDMA symbols available per frame ( 47 according to system evalutation methodology p. 23)
    int maxNbOFDMASymbols = int( floor(( phy->getPS() * nbPSLeft) / phy->getSymbolTime()));

    // maximum number of OFDMA symbols available per downlink frame ( dl 47 - n, n  for 12 <= n <= 21 )
    int maxNbDlSymbols = int( floor(maxNbOFDMASymbols * dlratio_));
    // maximum number of OFDMA symbols available per uplink frame
    int maxNbUlSymbols = maxNbOFDMASymbols - maxNbDlSymbols;

    // Useful number of OFDMA symbols per downlink frame for PUSC ofdm = 2 * slots + preamble
    int useNbDlSymbols = int( floor ( (maxNbDlSymbols - DL_PREAMBLE) / phy->getSlotLength(DL_))) * phy->getSlotLength(DL_) + DL_PREAMBLE;
    debug_ext("Maximum Number of OFDM Symbols in DL Frame : %d Useful Number of Symbols in DL Frame : %d \n", maxNbDlSymbols, useNbDlSymbols);

    // Useful number of OFDMA symbols per uplink frame for PUSC  ofdm = 3 * slots
    int useNbUlSymbols = int( floor (maxNbUlSymbols / phy->getSlotLength(UL_)) * phy->getSlotLength(UL_));
    debug_ext("Maximum Number of OFDM Symbols in UL Frame : %d Useful Number of Symbols in UL Frame : %d \n", maxNbUlSymbols, useNbUlSymbols);

    // OFDMA Symbol offset (cp. IEEE 802.16Rev2 8.4.5.3 DL-MAP IE format)
    // The offset of the OFDMA symbol in which the burst starts, measured in OFDMA symbols
    // from the DL symbol in which the preamble is transmitted with the symbol immediately following
    // the preamble being offset 1.
    int dlSymbolOffset = DL_PREAMBLE;
    // Subchannel offset (cp. IEEE 802.16Rev2 8.4.5.3 DL-MAP IE format)
    // The lowest index OFDMA subchannel used for carrying the burst, starting from subchannel 0.
    int dlSubchannelOffset = FCH_NB_OF_SLOTS;

    int ulSymbolOffset = 0;

    int ulSubchannelOffset = 0;

    // number of slots for data transmission in downlink direction
    int totalDlSlots = int( floor((useNbDlSymbols - dlSymbolOffset) / phy->getSlotLength(DL_))) * phy->getNumsubchannels(DL_) - dlSubchannelOffset;
    debug_ext("Free Slot within the Downlink Frame : %d \n",totalDlSlots);

    // number of slots for data transmission in uplink direction
    int totalUlSlots = int( floor((useNbUlSymbols / phy->getSlotLength(UL_))) * phy->getNumsubchannels(UL_));

    int freeDlSlots = totalDlSlots;
    int freeUlSlots = totalUlSlots;

    // --> adapt SymbolOffset, SubchannelOffset and freeSlots after every allocation !!!

    mac_->setMaxDlduration (useNbDlSymbols - DL_PREAMBLE); // dl duration left after preamble for dl timer
    mac_->setMaxUlduration (useNbUlSymbols);

    //============================UL allocation=================================
    //1 and 2 - Clear Ul_MAP and Allocate inital ranging and bw-req regions
//    int ul_subframe_subchannel_offset = 0;
//    int total_UL_num_subchannel = phy->getNumsubchannels(UL_);

    mac_->getMap()->getUlSubframe()->setStarttime ( maxNbDlSymbols * phy->getSymbolPS() + mac_->phymib_.rtg);

    while ( mac_->getMap()->getUlSubframe()->getNbPdu() > 0) {
        PhyPdu *pdu = mac_->getMap()->getUlSubframe()->getPhyPdu( 0);
        pdu->removeAllBursts();
        mac_->getMap()->getUlSubframe()->removePhyPdu( pdu);
        delete ( pdu);
    }

    //Set contention slots for cdma initial ranging
    int nbUlPdus = 0;
    UlBurst *ub = (UlBurst*)mac_->getMap()->getUlSubframe()->addPhyPdu ( nbUlPdus++ , 0)->addBurst ( 0);
    ub->setIUC (UIUC_INITIAL_RANGING);
    int int_rng_num_sub = int(init_contention_size_*CDMA_6SUB);		//init_contention_size is set to 5 in this version
    int int_rng_num_sym = 2;
    ub->setDuration (int_rng_num_sym);
    ub->setStarttime ( ulSymbolOffset); // vr@tud
    ub->setSubchannelOffset ( 0); //ulSubchannelOffset //vr@tud
    ub->setnumSubchannels(int_rng_num_sub);
    debug_ext ("UL.Initial Ranging, contention_size :%d, 1opportunity = 6sub*2symbols, ulduration :%d, initial_rng_duration :%d, updated ulduration :%d\n", init_contention_size_, ulSymbolOffset, int_rng_num_sym, ulSymbolOffset + int_rng_num_sym);

    ulSymbolOffset += int_rng_num_sym;
    ulSubchannelOffset += 0; // vr@tud ???
    //freeUlSlots -= 35; // vr@tud ??? 35 x 2 symbols

    //Set contention slots for cdma bandwidth request
    ub = (UlBurst*)mac_->getMap()->getUlSubframe()->addPhyPdu (nbUlPdus++,0)->addBurst (0);
    ub->setIUC (UIUC_REQ_REGION_FULL);
    int bw_rng_num_sub = int(bw_req_contention_size_*CDMA_6SUB);	//bw_req_contention_size is set to 5 in this version
    int bw_rng_num_sym = 1;
    ub->setDuration ( bw_rng_num_sym);
    ub->setStarttime ( ulSymbolOffset);
    ub->setSubchannelOffset ( 0); //ulSubchannelOffset
    ub->setnumSubchannels( bw_rng_num_sub);
    debug10 ("UL.Bw-req, contention_size :%d, 1opportunity = 6sub*1symbols, ulduration :%d, bw-req_duration :%d, updated ulduration :%d\n", bw_req_contention_size_, ulSymbolOffset, bw_rng_num_sym, ulSymbolOffset + bw_rng_num_sym);

    ulSymbolOffset += bw_rng_num_sym;
    ulSubchannelOffset += 0; // vr@tud ???

    // together with initial ranging
    freeUlSlots -= 35; // vr@tud ??? 35 * 1 symbols


    //Set CQICH slot for feedback from SS.
    bool has_cqich_to_allocate = FALSE;
    int sync_cqich_slot_allocated = 0;
    int cqich_subchannel_offset = 0;

    if (mac_->amc_enable_ == 1) {
        /*firstly make sure there is a request to allocate a CQICH slot.*/
        for (int i=0; i<MAX_SYNC_CQICH_ALLOC_INFO; i++) {
            //if(mac_->cqich_alloc_info_buffer[i].ext_uiuc == UIUC_CQICH_ALLOCATION_IE)
            if (mac_->cqich_alloc_info_buffer[i].need_cqich_slot == 1) {
                has_cqich_to_allocate = TRUE;
                debug2("\nNeed to allocate CQICH slot for some SS.\n\n ");
                break;
            }
        }

        debug2("has_cqich_to_allocate is %d\n", has_cqich_to_allocate);
        int cqich_symbol_num = 1;
        if (has_cqich_to_allocate == TRUE) {
            /*Add UL burst for each SS which is asked to send channel feedback.*/
            /*Only at most 11 CQICH slot can be allocated in a frame.*/

            for (int i=0; i<MAX_SYNC_CQICH_ALLOC_INFO; i++) {
                //debug2("has_sent is %d   ext_uiuc %d \n", mac_->cqich_alloc_info_buffer[i].has_sent, mac_->cqich_alloc_info_buffer[i].ext_uiuc);
                if ((mac_->cqich_alloc_info_buffer[i].need_cqich_slot == 1)
                        &&  (sync_cqich_slot_allocated < MAX_SYNC_CQICH_SLOT)) { // has sent, so start allocating CQICH slot
                    debug2("generate a UL Burst for CQICH slot.\n");
                    ub = (UlBurst*) mac_->getMap()->getUlSubframe()->addPhyPdu (nbUlPdus++,0)->addBurst (0);
                    ub->setIUC (UIUC_PROFILE_2);  // Enhanced CQICH profile.
                    ub->setDuration(1);
                    ub->setStarttime ( ulSymbolOffset);
                    ub->setSubchannelOffset(cqich_subchannel_offset);
                    ub->setnumSubchannels(3); // 3 fixed according to the standard.
                    ub->setCqichSlotFlag(TRUE);

                    u_int16_t cqich_id = getMac()->cqich_alloc_info_buffer[i].cqich_id;
                    ub->set_cqich_id(cqich_id);
                    ub->setBurstSsMacId(i);

                    cqich_subchannel_offset+=3;
                    sync_cqich_slot_allocated++;
                    debug2("setCqichSlotFlag is %d, cqich_id in burst is %d\n", ub->getCqichSlotFlag(), ub->get_cqich_id());
                }
                if (sync_cqich_slot_allocated >= MAX_SYNC_CQICH_SLOT)
                    break;
            }
            ulSymbolOffset += cqich_symbol_num;
            ulSubchannelOffset += 0; // vr@tud ???
            freeUlSlots -= 35; // vr@tud ???
        }
    }

    mac_->setStartUlduration (ulSymbolOffset);

    if (ulSymbolOffset > useNbUlSymbols ) {
        debug2 (" not enough UL symbols to allocate \n " );
    }


    // This cdma_flag is used in case there is no peer yet however the allocation for cdma_transmission oppportunity needed to be allocated by the BS-UL scheduler
    int cdma_flag = 0;
    Connection *con1;
    con1 = mac_->getCManager()->get_in_connection();
    while (con1!=NULL) {
        if (con1->get_category() == CONN_INIT_RANGING) {
            if (con1->getCdma()>0) {
                cdma_flag = 2;
            }
        }
        con1 = con1->next_entry();
    }


    //============================UL scheduling =================================
    int nbOfUlMapIes = 0;

    PeerNode * peer = mac_->getPeerNode_head();
    //Call ul_stage2 to allocate the uplink resource
    if ( (peer)  || (cdma_flag>0) ) {
        if (ulSymbolOffset < useNbUlSymbols) {
            debug2 ("UL.Before going to ul_stage2 (data allocation) Frame duration :%f, PSduration :%e, Symboltime :%e, nbPS :%d, rtg :%d, ttg :%d, nbPSleft :%d, nbSymbols :%d\n", mac_->getFrameDuration(), phy->getPS(), phy->getSymbolTime(), nbPS, mac_->phymib_.rtg, mac_->phymib_.ttg, nbPSLeft, useNbUlSymbols);
            //debug2 ("\tmaxdlduration :%d, maxulduration :%d, numsubchannels :%d, ulduration :%d\n", maxdlduration, (maxulduration), phy->getNumsubchannels(/*phy->getPermutationscheme (),*/ UL_), ulduration);

            /*UL_stage2()*/
            //mac802_16_ul_map_frame * ulMap = ul_stage2 (mac_->getCManager()->get_in_connection (),phy->getNumsubchannels( UL_), (useNbUlSymbols-ulSymbolOffset), useNbUlSymbols, VERTICAL_STRIPPING );
            mac802_16_ul_map_frame * ulMap = buildUplinkMap( mac_->getCManager()->get_in_connection (), phy->getNumsubchannels( UL_), useNbUlSymbols,  ulSymbolOffset, ulSubchannelOffset, freeUlSlots);

            // From UL_MAP_IE, we map the allocation into UL_BURST
            debug2("\nafter ul_stage2().ie_num is %d \n\n", ulMap->nb_ies);
            nbOfUlMapIes = int( ulMap->nb_ies);
            for (int numie = 0 ; numie < nbOfUlMapIes ; numie++) {
                mac802_16_ulmap_ie ulmap_ie = ulMap->ies[numie];
                ub = (UlBurst*) mac_->getMap()->getUlSubframe()->addPhyPdu (nbUlPdus++,0)->addBurst (0);


                debug2 ("In side ulmap adding to burst,  UL_MAP_IE.UIUC = %d, num_UL_MAP_IE = %d\n", ulmap_ie.uiuc, ulMap->nb_ies);

                ub->setCid (ulmap_ie.cid);
                ub->setIUC (ulmap_ie.uiuc);
                ub->setStarttime (ulmap_ie.symbol_offset);
                ub->setDuration (ulmap_ie.num_of_symbols);
                ub->setSubchannelOffset (ulmap_ie.subchannel_offset);
                ub->setnumSubchannels (ulmap_ie.num_of_subchannels);
                ub->setBurstCdmaTop (ulmap_ie.cdma_ie.subchannel);
                ub->setBurstCdmaCode (ulmap_ie.cdma_ie.code);


                /*By using the UL-MAP IE to bring the CQICH Allocation IE information down to the corresponding SS.*/
                debug2("mac_ get cqich alloc ie ind %d\n", mac_->get_cqich_alloc_ie_ind());
                //printf("amc_enable flag is %d\n", amc_enable_);
                /*Now we add the CQICH allocation IE to the corresponding UL-MAP IE.*/
                if (getMac()->get_cqich_alloc_ie_ind() == true && mac_->amc_enable_ ==1) {
                    debug2("1.1 BSscheduler is gonna sent CQICH Allocation IE via UL-MAP.\n");
                    for (int i=0; i<=(int) ulMap->nb_ies; ++i) {
                        int ss_mac_id = 0;
                        if ((getMac()->getCManager()->get_connection(ulmap_ie.cid, true)!= NULL)
                                && (getMac()->getCManager()->get_connection(ulmap_ie.cid, true)->getPeerNode()!= NULL)) {
                            ss_mac_id = getMac()->getCManager()->get_connection(ulmap_ie.cid, true)->getPeerNode()->getAddr();
                            debug2("1.1.1 bssceduler handles ss[%d]  has_sent  is [%d] cid is [%d]\n", ss_mac_id,getMac()->cqich_alloc_info_buffer[ss_mac_id].has_sent,ulmap_ie.cid);

                            /*if the CQICH_Alloc_IE is still not sent, send it now.*/
                            if ((getMac()->cqich_alloc_info_buffer[ss_mac_id].has_sent == false)
                                    && (getMac()->cqich_alloc_info_buffer[ss_mac_id].ext_uiuc == UIUC_CQICH_ALLOCATION_IE)) {
                                ub->setIUC(UIUC_EXT_UIUC);
                                ub->setExtendedUIUC(UIUC_CQICH_ALLOCATION_IE);
                                ub->set_cqich_ext_uiuc(UIUC_CQICH_ALLOCATION_IE);

                                /*these two params need to do when CQICH mechanism is done.*/
                                int cqich_id = getMac()->cqich_alloc_info_buffer[ss_mac_id].cqich_id;
                                int cqich_alloc_offset = getMac()->cqich_alloc_info_buffer[ss_mac_id].alloc_offset;
                                ub->set_cqich_id(cqich_id);
                                ub->set_cqich_alloc_offset(cqich_alloc_offset);
                                ub->setBurstSsMacId(ss_mac_id);

                                int period = getMac()->cqich_alloc_info_buffer[ss_mac_id].period;
                                int duration = getMac()->cqich_alloc_info_buffer[ss_mac_id].duration;
                                int type = getMac()->cqich_alloc_info_buffer[ss_mac_id].fb_type;
                                debug2("ss_id [%d]       period[%d]   duration [%d]      type[%d]\n ", ss_mac_id, period, duration, type);
                                ub->set_cqich_period(period);
                                ub->set_cqich_duration(duration);
                                ub->set_cqich_fb_type(type);
                                getMac()->cqich_alloc_info_buffer[ss_mac_id].has_sent = 1;
                                getMac()->cqich_alloc_info_buffer[ss_mac_id].need_cqich_slot = 1;
                                debug2("1.1.1.1 BSScheduler 1 schedule() has sent out CQICH Allocation IE via UL-MAP.\n");
                            }
                        }
                    }

                    /*scan all the ss cqich record, if all cqich allocation ies have been sent out,
                        disable cqich_alloc_ie_indicator so that next frame scheduler does not need
                        to scan the cqich_alloc_ie_buffer again.
                    */
                    bool all_clear = true;
                    for (int i=1; i<MAX_SYNC_CQICH_ALLOC_INFO; i++) {
                        if (getMac()->cqich_alloc_info_buffer[i].has_sent == false) {
                            all_clear = false;
                            debug2("There are still CQICH_ALLOC_IE command not sent. ss index is %d\n", i);
                            break;
                        }
                    }

                    if (all_clear == true)
                        getMac()->set_cqich_alloc_ie_ind(false);
                    else
                        getMac()->set_cqich_alloc_ie_ind(true);
                }


                debug2("UL.Data region (Addburst): symbol offset[%d]\t symbol num[%d]\t subchannel offset[%d]\t subchannel num[%d]\n", ulmap_ie.symbol_offset, ulmap_ie.num_of_symbols, ulmap_ie.subchannel_offset, ulmap_ie.num_of_subchannels);
                debug2("   Addburst cdma code :%d, cdma top :%d\n", ub->getBurstCdmaCode(), ub->getBurstCdmaTop());
            }
            delete ulMap;
        }
    }

    //End of map
    // does not exist in IEEE 802.16Rev2, but necessary for synchronization

    //Note that in OFDMA, there is no end of UL map, it'll be removed in next version; however, this is a virtual end of map say there is no transmitted packet/message
    ub = (UlBurst*)mac_->getMap()->getUlSubframe()->addPhyPdu (nbUlPdus,0)->addBurst (0);
    ub->setIUC (UIUC_END_OF_MAP);
    ub->setStarttime (useNbUlSymbols);
    ub->setSubchannelOffset (0); //Richard: changed 1->0
    ub->setnumSubchannels (phy->getNumsubchannels(UL_));
    debug_ext ("UL.EndofMAP: (addBurst_%d) maxuluration :%d, lastbursts :%d, nbulpdus :%d\n", nbOfUlMapIes, useNbUlSymbols, nbOfUlMapIes, nbUlPdus);





    //============================DL allocation=================================
    debug10 ("DL.Scheduler: FrameDuration :%5.4f, PSduration :%e, symboltime :%e, nbPS :%d, rtg :%d, ttg :%d, nbPSleft :%d, useNbDlSymbols :%d, dlratio_ :%5.2f\n", mac_->getFrameDuration(), phy->getPS(), phy->getSymbolTime(), nbPS, mac_->phymib_.rtg, mac_->phymib_.ttg, nbPSLeft, useNbDlSymbols, dlratio_);

    FrameMap *map = mac_->getMap();
    map->getDlSubframe()->getPdu()->removeAllBursts();



    // Create virtual allocation container
    VirtualAllocation * virtualAlloc = new VirtualAllocation;


    // Build broadcast burst first

    // Set capacity for broadcast burst
    int broadcastSlotCapacity = phy->getSlotCapacity(map->getDlSubframe()->getProfile ( map->getDlSubframe()->getProfile (default_mod_)->getIUC())->getEncoding(), DL_);

    // TODO: May result in false values
    broadcastSlotCapacity = int( ceil( float(broadcastSlotCapacity) / repetition_code_));
    virtualAlloc->setBroadcastSlotCapacity( broadcastSlotCapacity);

    // 1. Virtual DL_MAP
    int sizeOfDlMap = GENERIC_HEADER_SIZE + DL_MAP_HEADER_SIZE;
    freeDlSlots -= virtualAlloc->increaseBroadcastBurst( sizeOfDlMap);

    // 2. Virtual UL_MAP
    int sizeOfUlMap = GENERIC_HEADER_SIZE + UL_MAP_HEADER_SIZE + UL_MAP_IE_SIZE * nbOfUlMapIes;
    freeDlSlots -= virtualAlloc->increaseBroadcastBurst( sizeOfUlMap + DL_MAP_IE_SIZE);

    // 3. Virtual DCD
    if (getMac()->isSendDCD() || map->getDlSubframe()->getCCC()!= getMac()->getDlCCC()) {
        Packet *p = map->getDCD();
        struct hdr_cmn *ch = HDR_CMN(p);

        int sizeOfDCD = ch->size();
        debug_ext("Size of DCD : %d \n", sizeOfDCD);

        // increase broadcast burst
        freeDlSlots -= virtualAlloc->increaseBroadcastBurst( sizeOfDCD);
    }

    // 4. Virtual UCD
    if (getMac()->isSendUCD() || map->getUlSubframe()->getCCC()!= getMac()->getUlCCC()) {
        Packet *p = map->getUCD();
        struct hdr_cmn *ch = HDR_CMN(p);

        int sizeOfUCD = ch->size();
        debug_ext("Size of UCD: %d \n", sizeOfUCD);

        freeDlSlots -= virtualAlloc->increaseBroadcastBurst( sizeOfUCD);
    }

    //5. Virtual Other broadcast
    if (mac_->getCManager()->get_connection (BROADCAST_CID, OUT_CONNECTION)->queueByteLength() > 0) {

        Connection * currentCon;
        currentCon = mac_->getCManager()->get_connection (BROADCAST_CID, OUT_CONNECTION);


        // get first packed
        Packet * currentPacket = currentCon->get_queue()->head();
        int allocatedBytes = 0;

        // go through the queue to get all packet sizes
        while  ( currentPacket != NULL) {
            int packetSize = HDR_CMN(currentPacket)->size();
            allocatedBytes += packetSize;

            currentPacket = currentPacket->next_;
        }

        // debug
        int queueLength = currentCon->get_queue()->length();
        debug_ext("Broadcast Allocation Bytes %d Packetes %d FreeSlots %d broadcastSlotCapacity %d\n", allocatedBytes, queueLength, freeDlSlots, broadcastSlotCapacity);

        // limit broadcast size to frame size
        if (allocatedBytes > broadcastSlotCapacity * freeDlSlots) {
        	fprintf(stderr, "Not enough dl_slots to send broadcast messages \n");
        	allocatedBytes = broadcastSlotCapacity * freeDlSlots;
        }

        freeDlSlots -= virtualAlloc->increaseBroadcastBurst( allocatedBytes);
        debug_ext("Free Downlink Slots after virtual Map %d \n", freeDlSlots );
    }




    //============================DL scheduling =================================


    // from here start ofdma DL -- call the scheduler API- here DLMAP    ------------------------------------
    PeerNode * firstPeer = mac_->getPeerNode_head();

    mac802_16_dl_map_frame * dlMap = NULL;

    // Call buildDownlinkMap to allocate the downlink resource
    int nbOfDlBursts = 0;
    if ( firstPeer &&  ( freeDlSlots > 0) ) {
		debug2 ("DL.Before going to dl-stage2 (data allocation), Frame Duration :%5.4f, PSduration :%e, symboltime :%e, nbPS :%d, rtg :%d, ttg :%d, PSleft :%d, useNbDlSymbols :%d\n", mac_->getFrameDuration(), phy->getPS(), phy->getSymbolTime(), nbPS, mac_->phymib_.rtg, mac_->phymib_.ttg, nbPSLeft, useNbDlSymbols);
		//debug10 ("\tStartdld :%d, Enddld :%d, Dldsize :%d, Uldsize :%d, Numsubchannels (35 or 30) :%d \n", useNbDlSymbols,  phy->getNumsubchannels(/*phy->getPermutationscheme (), */DL_));

		dlMap = buildDownlinkMap( virtualAlloc, mac_->getCManager()->get_out_connection (), phy->getNumsubchannels(DL_), useNbDlSymbols, dlSymbolOffset, dlSubchannelOffset, freeDlSlots);
		if (NOW > 10.0) {
			printf("ok \n");
		}

		// create physical DL burst
		// TODO: Note that #subchannels = #slots in this version
		DlBurst *db;
		int indexDlMapIe = dlMap->nb_ies;
		assert (dlMap->nb_ies < MAX_MAP_IE);
		for (int i = 0; i < indexDlMapIe ; i++) {
			mac802_16_dlmap_ie  dlMapIe = dlMap->ies[i];

			debug_ext ("Add DL Burst, #bursts %d, CID :%d, DIUC :%d, SymbolOffset :%d, SubchannelOffset :%d, #symbols :%d,  #subchannels :%d\n", nbOfDlBursts, dlMapIe.cid, dlMapIe.diuc, dlMapIe.symbol_offset,  dlMapIe.subchannel_offset, dlMapIe.num_of_symbols, dlMapIe.num_of_subchannels);

			db = (DlBurst*) map->getDlSubframe()->getPdu ()->addBurst (nbOfDlBursts++);
			db->setCid (dlMapIe.cid);
			db->setIUC (dlMapIe.diuc);
			db->setPreamble(dlMapIe.preamble);
			db->setStarttime(dlMapIe.symbol_offset);
			db->setSubchannelOffset(dlMapIe.subchannel_offset);
			db->setnumSubchannels(dlMapIe.num_of_subchannels);
			db->setDuration(dlMapIe.num_of_symbols);

		}
    } else {
        // Allocate Broadcast Burst if Downlink Map was not created
        DlBurst *db;
        db = (DlBurst*) map->getDlSubframe()->getPdu ()->addBurst (nbOfDlBursts++);
        db->setCid ( BROADCAST_CID);
        db->setIUC ( map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC());
        db->setPreamble( true);
        db->setStarttime( dlSymbolOffset);
        db->setSubchannelOffset( dlSymbolOffset);
        db->setnumSubchannels( virtualAlloc->getNbOfBroadcastSlots());
        db->setDuration( int( ceil( double( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) /  phy->getNumsubchannels(DL_)) * phy->getSlotLength(DL_)));

    }


    debug_ext("Number of Downlink Bursts %d !!!!!!!!!!!!!!!!!!!!! \n",nbOfDlBursts++);

    // delete virtual allocation container
    delete virtualAlloc;
    // delete Downlink Map
    if ( dlMap != NULL) {
        delete dlMap;
    }












    // ============================= Transfer Packets to the Bursts ===========================

    // *** Transfer Broadcast Packets to the Broadcast burst
    // Now transfer the packets to the physical bursts starting with broadcast messages
    // In this version, we can directly map burst information from virtual burst into physical burst.
    // 2D rectangular allocation will be considered in next versio


    Burst *b= map->getDlSubframe()->getPdu ()->getBurst (0);
    hdr_mac802_16 *wimaxHdr;

    // Get First Slot of the current Burst
    int burstSymbolOffset = b->getStarttime();
    int burstSubchannelOffset = b->getSubchannelOffset();
    // Get Size of the current Burst
    // int burstNbSymbols = b->getDuration();
    int burstNbSubchannels = b->getnumSubchannels();
    debug_ext("Start Transfer packets into burst => SymbolOffset :%d SubchannelOffset %d \n", burstSymbolOffset, burstSubchannelOffset);

    // Capacity of one Slot for this Burst
    int slotCapacityBurst = phy->getSlotCapacity(map->getDlSubframe()->getProfile (b->getIUC())->getEncoding() , DL_);
    // Get the data capacity of this Burst in byte
    // TODO: based on number of subchannels = number of bursts
    int freeBurstSize = burstNbSubchannels * slotCapacityBurst;
    debug_ext("Free Space in the current Burst %d \n", freeBurstSize);


    Packet *p;
    struct hdr_cmn *ch;

    // *** Start with Downlink Map ***
    p = map->getDL_MAP();
    ch = HDR_CMN(p);

    // Size of this packet
    int packetSize = ch->size();
    debug_ext("Size of the Packet containing the Downlink Map %d \n", packetSize);
    // Number of Slot required for this packet
    int nbOfSlotsPacket =  int( ceil( double(packetSize) / slotCapacityBurst));
    // TODO: Only correct for vertical stripping
    int nbOfSymbolsPacket = int( ceil(double( burstSubchannelOffset + nbOfSlotsPacket ) / phy->getNumsubchannels(DL_))) * phy->getSlotLength(DL_);

    // Set the transmission time in the packet
    ch->txtime() = nbOfSymbolsPacket * phy->getSymbolTime();

    // assuring that the burst has enough space for the packet
    assert( freeBurstSize > packetSize);

    // Put information into physical info header
    wimaxHdr = HDR_MAC802_16(p);
    if (wimaxHdr) {
        wimaxHdr->phy_info.num_subchannels = nbOfSlotsPacket;
        wimaxHdr->phy_info.subchannel_offset = burstSubchannelOffset;
        wimaxHdr->phy_info.num_OFDMSymbol = nbOfSymbolsPacket;
        wimaxHdr->phy_info.OFDMSymbol_offset = burstSymbolOffset;
        wimaxHdr->phy_info.channel_index = 1; //broadcast packet
        wimaxHdr->phy_info.direction = 0;
    }
    ch->timestamp() = NOW; //add timestamp since it bypasses the queue
    b->enqueue(p);      //enqueue into burst

    debug_ext("The length of the queue of burst is [%d]\n",b->getQueueLength_packets());

    //update freeBurstSize
    freeBurstSize -= packetSize;

    // !!! TODO: There might be some space left in the last Slot

    // calculate new Symbol and Subchannel Offset
    burstSymbolOffset = burstSymbolOffset + ((burstSubchannelOffset +  nbOfSlotsPacket) / (phy->getNumsubchannels(DL_))) * phy->getSlotLength(DL_);
    burstSubchannelOffset = ( burstSubchannelOffset + nbOfSlotsPacket) % (phy->getNumsubchannels(DL_));


    // *** Second Uplink Map ***
    p = map->getUL_MAP();
    ch = HDR_CMN(p);

    // Size of this packet
    packetSize = ch->size();
    debug_ext("Size of the Packet containing the Uplink Map %d \n", packetSize);
    // Number of Slot required for this packet
    nbOfSlotsPacket =  int( ceil( double(packetSize) / slotCapacityBurst));
    // TODO: Only correct for vertical stripping
    nbOfSymbolsPacket = int( ceil(double( burstSubchannelOffset + nbOfSlotsPacket ) / phy->getNumsubchannels(DL_))) * phy->getSlotLength(DL_);

    // Set the transmission time in the packet
    ch->txtime() = nbOfSymbolsPacket * phy->getSymbolTime();

    // assuring that the burst has enough space for the packet
    assert( freeBurstSize > packetSize);

    // Put information into physical info header
    wimaxHdr = HDR_MAC802_16(p);
    if (wimaxHdr) {
        wimaxHdr->phy_info.num_subchannels = nbOfSlotsPacket;
        wimaxHdr->phy_info.subchannel_offset = burstSubchannelOffset;
        wimaxHdr->phy_info.num_OFDMSymbol = nbOfSymbolsPacket;
        wimaxHdr->phy_info.OFDMSymbol_offset = burstSymbolOffset;
        wimaxHdr->phy_info.channel_index = 1; //broadcast packet
        wimaxHdr->phy_info.direction = 0;
    }
    ch->timestamp() = NOW; //add timestamp since it bypasses the queue
    b->enqueue(p);      //enqueue into burst

    debug_ext("The length of the queue of burst is [%d]\n",b->getQueueLength_packets());

    //update freeBurstSize
    freeBurstSize -= packetSize;

    // !!!  There might be some space left in the last Slot

    // calculate new Symbol and Subchannel Offset
    burstSymbolOffset = burstSymbolOffset + ((burstSubchannelOffset + nbOfSlotsPacket) / (phy->getNumsubchannels(DL_))) * phy->getSlotLength(DL_);
    burstSubchannelOffset = ( burstSubchannelOffset + nbOfSlotsPacket) % (phy->getNumsubchannels(DL_));


    // Downlink Channel Descriptor
    if (getMac()->isSendDCD() || map->getDlSubframe()->getCCC()!= getMac()->getDlCCC()) {
        p = map->getDCD();
        ch = HDR_CMN(p);
        // Size of this packet
        packetSize = ch->size();
        debug_ext("Size of the Packet containing the DCD %d \n", packetSize);
        // Number of Slot required for this packet
        nbOfSlotsPacket =  int( ceil( double(packetSize) / slotCapacityBurst));
        // TODO: Only correct for vertical stripping
        nbOfSymbolsPacket = int( ceil(double( burstSubchannelOffset + nbOfSlotsPacket ) / phy->getNumsubchannels(DL_))) * phy->getSlotLength(DL_);

        // Set the transmission time in the packet
        ch->txtime() = nbOfSymbolsPacket * phy->getSymbolTime();

        // assuring that the burst has enough space for the packet
        assert( freeBurstSize > packetSize);

        // Put information into physical info header
        wimaxHdr = HDR_MAC802_16(p);
        if (wimaxHdr) {
            wimaxHdr->phy_info.num_subchannels = nbOfSlotsPacket;
            wimaxHdr->phy_info.subchannel_offset = burstSubchannelOffset;
            wimaxHdr->phy_info.num_OFDMSymbol = nbOfSymbolsPacket;
            wimaxHdr->phy_info.OFDMSymbol_offset = burstSymbolOffset;
            wimaxHdr->phy_info.channel_index = 1; //broadcast packet
            wimaxHdr->phy_info.direction = 0;
        }
        ch->timestamp() = NOW; //add timestamp since it bypasses the queue
        b->enqueue(p);      //enqueue into burst

        debug_ext("The length of the queue of burst is [%d]\n",b->getQueueLength_packets());

        //update freeBurstSize
        freeBurstSize -= packetSize;

        // !!!There might be some space left in the last Slot

        // calculate new Symbol and Subchannel Offset
        burstSymbolOffset = burstSymbolOffset + ((burstSubchannelOffset + nbOfSlotsPacket) / (phy->getNumsubchannels(DL_))) * phy->getSlotLength(DL_);
        burstSubchannelOffset = ( burstSubchannelOffset + nbOfSlotsPacket) % (phy->getNumsubchannels(DL_));
    }

    // Uplink Channel Descriptor
    if (getMac()->isSendUCD() || map->getUlSubframe()->getCCC()!= getMac()->getUlCCC()) {
        p = map->getUCD();
        ch = HDR_CMN(p);
        // Size of this packet
        packetSize = ch->size();
        debug_ext("Size of the Packet containing the DCD %d \n", packetSize);
        // Number of Slot required for this packet
        nbOfSlotsPacket =  int( ceil( double(packetSize) / slotCapacityBurst));
        // TODO: Only correct for vertical stripping
        nbOfSymbolsPacket = int( ceil(double( burstSubchannelOffset + nbOfSlotsPacket ) / phy->getNumsubchannels(DL_))) * phy->getSlotLength(DL_);

        // Set the transmission time in the packet
        ch->txtime() = nbOfSymbolsPacket * phy->getSymbolTime();

        // assuring that the burst has enough space for the packet
        assert( freeBurstSize > packetSize);

        // Put information into physical info header
        wimaxHdr = HDR_MAC802_16(p);
        if (wimaxHdr) {
            wimaxHdr->phy_info.num_subchannels = nbOfSlotsPacket;
            wimaxHdr->phy_info.subchannel_offset = burstSubchannelOffset;
            wimaxHdr->phy_info.num_OFDMSymbol = nbOfSymbolsPacket;
            wimaxHdr->phy_info.OFDMSymbol_offset = burstSymbolOffset;
            wimaxHdr->phy_info.channel_index = 1; //broadcast packet
            wimaxHdr->phy_info.direction = 0;
        }
        ch->timestamp() = NOW; //add timestamp since it bypasses the queue
        b->enqueue(p);      //enqueue into burst

        debug_ext("The length of the queue of burst is [%d]\n",b->getQueueLength_packets());

        //update freeBurstSize
        freeBurstSize -= packetSize;

        // UGLY HACK There might be some space left in the last Slot

        // calculate new Symbol and Subchannel Offset
        burstSymbolOffset = burstSymbolOffset + ((burstSubchannelOffset + nbOfSlotsPacket) / (phy->getNumsubchannels(DL_))) * phy->getSlotLength(DL_);
        burstSubchannelOffset = ( burstSubchannelOffset + nbOfSlotsPacket) % (phy->getNumsubchannels(DL_));

    }

    // Broadcast Packets
    Connection *addBroadcastConnection = mac_->getCManager ()->get_connection (BROADCAST_CID,OUT_CONNECTION);
    if ( addBroadcastConnection->queueByteLength() > 0) {
        freeBurstSize -= transfer_packets1( addBroadcastConnection, b, ( burstNbSubchannels * slotCapacityBurst ) - freeBurstSize);
    }

    //b->setStarttime(offset);
    //Get other broadcast messages
    //Connection *c=mac_->getCManager ()->get_connection (b->getCid(),OUT_CONNECTION);
    //b_data += transfer_packets (c, b, b_data,subchannel_offset_wimaxhdr, offset);
    //b_data += transfer_packets1 (c, b, b_data);

    //Now get the other bursts
    debug_ext("BS scheduler is going to handle other (not DL/UL_MAP, DCD/UCD, Broadcast) number of burst is [%d]\n",map->getDlSubframe()->getPdu ()->getNbBurst());
    for (int index = 1 ; index < map->getDlSubframe()->getPdu ()->getNbBurst(); index++) {
        //for (int index = 0 ; index < map->getDlSubframe()->getPdu ()->getNbBurst(); index++) {
        Burst *burst = map->getDlSubframe()->getPdu ()->getBurst (index);
        int b_data = 0;


        Connection *con=mac_->getCManager ()->get_connection (burst->getCid(),OUT_CONNECTION);
        debug_ext("BSScheduler is going to handle CID [%d]\n", burst->getCid());
#ifdef DEBUG_WIMAX
        assert (con);
#endif



        //Begin RPI
        if (con != NULL) {
            debug10 ("DL.2.2.Enable Frag :%d , Pack :%d, ARQ :%p\n",con->isFragEnable(),con->isPackingEnable(), con->getArqStatus ());
            debug10 ("DL.2.2.Before transfer_packets1 (other data) to burst_i :%d, CID :%d, b_data :%d\n", index, burst->getCid(), b_data);
            if ( con->isFragEnable() && con->isPackingEnable()
                    &&  (con->getArqStatus () != NULL) && (con->getArqStatus ()->isArqEnabled() == 1)) {
                debug2("BSSscheduler is goting to transfer packet with fragackarq.\n");
                b_data = transfer_packets_with_fragpackarq (con, burst, b_data); /*RPI*/
            } else {

                //b_data = transferPacketsDownlinkBurst(con, burst, b_data); // for OSCA algorithm
                b_data = transfer_packets1(con, burst, b_data);
            }
        }

        // UPDATE ALLOCATION MISSING !!!!!!!!!!!!!!!!

        debug_ext ("\nDL.2.2.After transfer_packets1 (other data) to burst_i :%d, CID :%d, b_data :%d\n", index, burst->getCid(), b_data);
        debug2("The length of the queue of burst is [%d]\n",burst->getQueueLength_packets());
    }//end loop ===> transfer bursts






#ifdef DEBUG_EXT
    // for testing purpose vr@tud 01-09
    //Print the map
    printf("\n==================BS %d Subframe============================\n", mac_->addr());

    mac_->getMap()->print_frame();

    printf("=====================================================\n");
#endif

    debug2("\n============================BSScheduler::schedule () End =============================\n");

}





/**
 * Add a downlink burst with the given information
 * @param burstid The burst number
 * @param c The connection to add
 * @param iuc The profile to use
 * @param dlduration current allocation status
 * @param the new allocation status
 */
void BSScheduler::addDlBurst (int burstid, int cid, int iuc, int ofdmsymboloffset, int numofdmsymbols, int subchanneloffset, int numsubchannels)
{
    DlBurst *db = (DlBurst*) mac_->getMap()->getDlSubframe()->getPdu ()->addBurst (burstid);
    db->setCid (cid);
    db->setIUC (iuc);
    db->setStarttime (ofdmsymboloffset);
    db->setDuration (numofdmsymbols);
    db->setSubchannelOffset (subchanneloffset);
    db->setnumSubchannels (numsubchannels);

    debug2("Data Burst:----symbol_offset[%d]\t symbol_num[%d]\t subchannel_offset[%d]\t subchannel_num[%d]\n",
           ofdmsymboloffset, numofdmsymbols,subchanneloffset,numsubchannels);

}


mac802_16_dl_map_frame * BSScheduler::buildDownlinkMap( VirtualAllocation * virtualAlloc, Connection *head, int totalDlSubchannels, int totalDlSymbols, int dlSymbolOffset, int dlSubchannelOffset, int freeDlSlots)
{
    // 1. Traffic Policing Downlink Data Connections and Allocation for management connections
    // 2. Scheduling for Data Connection
    // 3. Allocation for Data Connection
    // 4. Updata Traffic Policing


    // old code why is amc attached to the connections ?
    /*AMC mechanism. change the modulation coding scheme in each connections.*/
    if (mac_->amc_enable_ ==1) {
        for (int i=0; i<5; ++i) {
            Connection * con = head;
            DataDeliveryServiceType_t dataDeliveryServiceType;
            if (i==0)
                dataDeliveryServiceType = DL_UGS;
            else if (i==1)
                dataDeliveryServiceType = DL_ERTVR;
            else if (i==2)
                dataDeliveryServiceType = DL_RTVR;
            else if (i==3)
                dataDeliveryServiceType = DL_NRTVR;
            else
                dataDeliveryServiceType = DL_BE;

            while (con!=NULL) {
                if (con->get_category() == CONN_DATA && con->get_serviceflow()->getQosSet()->getDataDeliveryServiceType() == dataDeliveryServiceType) {
                    /*Xingting would like to change the modulation mode for this SS.*/
                    int ss_mac_id;
                    Ofdm_mod_rate  updated_mod_rate;
                    Ofdm_mod_rate con_mod = mac_->getMap()->getDlSubframe()->getProfile (con->getPeerNode()->getDIUC())->getEncoding();
                    Ofdm_mod_rate mod_rate = con_mod;

                    if (con!= NULL && con->getPeerNode() != NULL) {
                        ss_mac_id = con->getPeerNode()->getAddr();
                        debug2("ss_mac_id %d The condition is: mac_->get_change_modulation_flag(ss_mac_id) %d    mac_->amc_enable_ %d\n",
                               ss_mac_id, mac_->get_change_modulation_flag(ss_mac_id) , mac_->amc_enable_);
                        if (mac_->get_change_modulation_flag(ss_mac_id) == TRUE && mac_->amc_enable_ == 1) {
                            /*manually add a packet into the data connection since the MCS control information will be carried by the data burst. But this data burst is not big at all.*/
                            if (con->queueLength() <0) {
                                debug2("Need to manually add a data packet to carry the AMC mcs index info.\n");
                                Packet * pfb = mac_->getPacket ();
                                hdr_mac802_16 *wimaxHdrMap ;
                                wimaxHdrMap= HDR_MAC802_16(pfb);
                                wimaxHdrMap->header.cid = con->get_cid ();
                                wimaxHdrMap->header.ht = 1;
                                wimaxHdrMap->header.ec = 1;
                                con->enqueue( pfb);
                            }
                            int current_mcs_index =mac_->get_current_mcs_index(ss_mac_id);

                            bool increase_flag = mac_->get_increase_modulation(ss_mac_id);
                            debug2("DL BS flag 11, current_mcs_index is %d, increase flag is %d,  mod_rate is %d\n", current_mcs_index, increase_flag, mod_rate);

                            int updated_mcs_index = update_mcs_index(mod_rate,current_mcs_index,increase_flag);
                            bool change_modulation = check_modulation_change(mod_rate, current_mcs_index, increase_flag);

                            debug2("DL BS flag 22, updated mcs index is %d, change modulation is %d\n",updated_mcs_index,change_modulation);
                            if (change_modulation == TRUE) { // modulation and mcs index are all needed to change.
                                updated_mod_rate = change_rate(mod_rate,increase_flag);
                                con->getPeerNode()->setDIUC(getDIUCProfile(updated_mod_rate));
                                con->getPeerNode()->setCurrentMcsIndex(updated_mcs_index);
                                mac_->set_current_mcs_index(ss_mac_id, updated_mcs_index);
                                mac_->set_change_modulation_flag(ss_mac_id, false);
                                debug2("DL BS Xingting is gonna 1 change SS %d from  modulation from %d to %d\t MCS index from [%d] to [%d].\n",
                                       ss_mac_id, mod_rate, updated_mod_rate, current_mcs_index, updated_mcs_index);
                            } else if (change_modulation == FALSE) {
                                updated_mod_rate = mod_rate;
                                con->getPeerNode()->setDIUC(getDIUCProfile(mod_rate));
                                con->getPeerNode()->setCurrentMcsIndex(updated_mcs_index);
                                mac_->set_current_mcs_index(ss_mac_id,updated_mcs_index);
                                mac_->set_change_modulation_flag(ss_mac_id, false);
                                debug2("DL BS Xingting is gonna 2 change SS %d from  modulation from %d to %d\t MCS index from [%d] to [%d].\n",
                                       ss_mac_id, mod_rate, updated_mod_rate, current_mcs_index, updated_mcs_index);
                            }
                        }
                    }
                }
                con=con->next_entry();
            }
        }
    }



    Connection * currentCon;

    // get simulation time
    double currentTime = NOW;


    currentCon = head;
    while ( currentCon != NULL ) {
        // get type of this connection
        ConnectionType_t conType = currentCon->get_category();
        // variables used in switch block
        double allocationSize = 0.0;
        double maxLatency = 0.0;
        MrtrMstrPair_t mrtrMstrPair;

        switch( conType) {
        case CONN_BASIC:
        case CONN_PRIMARY:
        case CONN_SECONDARY:

            // data to send ?
            allocationSize = currentCon->queueByteLength();
            if ( allocationSize > 0 ) {
                // allocate resources for management data
                Ofdm_mod_rate burstProfile = mac_->getMap()->getDlSubframe()->getProfile(currentCon->getPeerNode()->getDIUC())->getEncoding();
                int slotCapacity = mac_->getPhy()->getSlotCapacity( burstProfile, DL_);

                // increase Downlink Map
                freeDlSlots -= virtualAlloc->increaseBroadcastBurst( DL_MAP_IE_SIZE);

                int nbOfSlots = int( ceil( allocationSize / slotCapacity));
                if (freeDlSlots < nbOfSlots) {
                    nbOfSlots = freeDlSlots;
                }
                freeDlSlots -= nbOfSlots;
                if ( nbOfSlots > 0) {
                    // search for allocation
                    if ( virtualAlloc->findConnectionEntry( currentCon) ) {
                        // connection found
                        // increase assigned slots
                        virtualAlloc->setCurrentNbOfSlots( virtualAlloc->getCurrentNbOfSlots() + nbOfSlots);
                        virtualAlloc->setCurrentNbOfBytes( virtualAlloc->getCurrentNbOfBytes() + allocationSize);
                    } else {
                        // connection has no assigned ressources
                        // add new entry
                        virtualAlloc->addAllocation( currentCon, 0, 0, slotCapacity, allocationSize, nbOfSlots);
                    }
                }
            }

            break;
        case CONN_DATA:

            // remove packets which exceeded their deadline
        	// QoS Parameter Max Latency in ms
            maxLatency = double( currentCon->get_serviceflow()->getQosSet()->getMaxLatency()) / 1e3;
            if ( maxLatency > 0) {
                double deadline = currentTime - maxLatency;

                Packet * oldestPacket = currentCon->queueLookup( 0);
                Packet * fragmentedPacket = NULL;

                // cause segmentation fault ?
                while (( oldestPacket != NULL ) && ( HDR_CMN( oldestPacket)->timestamp() < deadline )) {

                    // keep this packet if it is a part of a fragment
                    if (( currentCon->getFragmentationStatus() != FRAG_NOFRAG) && ( fragmentedPacket == NULL )) {
                        fragmentedPacket = currentCon->dequeue();
                    } else {
                        // drop packet
                        debug_ext("Con cid: %d deadline exceeded %f ms timestamp %f -> packet removed \n", currentCon->get_cid(), (deadline - HDR_CMN( oldestPacket)->timestamp()) * 1000, HDR_CMN( oldestPacket)->timestamp());
                        Packet::free( currentCon->dequeue());
                    }
                    // check next packet
                    oldestPacket = currentCon->queueLookup( 0);
                }
                if ( fragmentedPacket != NULL) {
                    // add fragmented packet as queuehead
                    currentCon->enqueue_head( fragmentedPacket);
                }
            }

            // Traffic Policing
            mrtrMstrPair = trafficShapingAlgorithm_->getDataSizes( currentCon, u_int32_t(currentCon->queuePayloadLength()));

            // Add to virtual allocation if data to send
            if (( mrtrMstrPair.first > 0 ) || (mrtrMstrPair.second > 0 )) {
                // this connection should not have an entry
                assert( !virtualAlloc->findConnectionEntry( currentCon));
                // add new virtual alloction
                Ofdm_mod_rate burstProfile = mac_->getMap()->getDlSubframe()->getProfile(currentCon->getPeerNode()->getDIUC())->getEncoding();
                int slotCapacity = mac_->getPhy()->getSlotCapacity( burstProfile, DL_);
                virtualAlloc->addAllocation( currentCon, mrtrMstrPair.first , mrtrMstrPair.second, slotCapacity);

                // increase size of Downlink Map TODO: Might be not necessary if connection is not scheduled
                freeDlSlots -= virtualAlloc->increaseBroadcastBurst( DL_MAP_IE_SIZE);
            }

         // udpate of traffic shaping now with get Data Sizese
         /*   else {
            	if ( currentCon->queueLength() > 0) {
            		// connection has data and is not scheduled therefore updateAllocation has to be called separately
            		trafficShapingAlgorithm_->updateAllocation( currentCon, 0, 0);
            	}
            } */


            break;
        default:
            // do nothing
            break;
        }
        // goto next connection
        currentCon = currentCon->next_entry();
    }

    // Call Scheduling Algorithm to allocate data connections
    dlSchedulingAlgorithm_->scheduleConnections( virtualAlloc, freeDlSlots);

 /*   // map virtual allocations to DlMap
    mac802_16_dl_map_frame * dlMap = new mac802_16_dl_map_frame;
    dlMap->type = MAC_DL_MAP;
    dlMap->bsid = mac_->addr(); // its called in the mac_ object

    // for testing purpose
    int entireUsedSlots = 0;

    OFDMAPhy *phy = mac_->getPhy();
    FrameMap *map = mac_->getMap();

    // add broadcast burst
    int dlMapIeIndex = 0;
    mac802_16_dlmap_ie * dlMapIe = NULL;
    dlMapIe = &(dlMap->ies[dlMapIeIndex]);

    dlMapIe->cid = BROADCAST_CID;
    dlMapIe->n_cid = 1;
    dlMapIe->diuc = map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC();
    dlMapIe->preamble = true;
    dlMapIe->start_time = dlSymbolOffset;
    dlMapIe->symbol_offset = dlSymbolOffset;
    dlMapIe->subchannel_offset = dlSubchannelOffset;
    dlMapIe->boosting = 0;
    dlMapIe->num_of_subchannels = virtualAlloc->getNbOfBroadcastSlots();
    dlMapIe->num_of_symbols = int( ceil( double( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) / totalDlSubchannels) * phy->getSlotLength(DL_));
    dlMapIe->repition_coding_indicator = 0;

    // update Symbol and Subchannel Offset
    dlSymbolOffset += int( floor( double( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) / totalDlSubchannels) * phy->getSlotLength(DL_));
    dlSubchannelOffset = ( dlSubchannelOffset + virtualAlloc->getNbOfBroadcastSlots()) % (totalDlSubchannels);

    // testing
    entireUsedSlots += virtualAlloc->getNbOfBroadcastSlots();

    // increase index
    dlMapIeIndex++;

    // get the first connection
    if ( virtualAlloc->firstConnectionEntry() )  {
        do {
            // get Connection
            Connection * currentCon = virtualAlloc->getConnection();
            int nbOfUsedSlots = virtualAlloc->getCurrentNbOfSlots();

            dlMapIe = &(dlMap->ies[dlMapIeIndex]);

            dlMapIe->cid = currentCon->get_cid();
            dlMapIe->n_cid = 1;
            dlMapIe->diuc = getDIUCProfile(mac_->getMap()->getDlSubframe()->getProfile(currentCon->getPeerNode()->getDIUC())->getEncoding());
            dlMapIe->preamble = false;
            dlMapIe->start_time = dlSymbolOffset;
            dlMapIe->symbol_offset = dlSymbolOffset;
            dlMapIe->subchannel_offset = dlSubchannelOffset;
            dlMapIe->boosting = 0;
            dlMapIe->num_of_subchannels = nbOfUsedSlots;
            dlMapIe->num_of_symbols = int( ceil( double( dlSubchannelOffset + nbOfUsedSlots) / totalDlSubchannels) * phy->getSlotLength(DL_));
            dlMapIe->repition_coding_indicator = 0;

            // update Symbol and Subchannel Offset
            dlSymbolOffset += int( floor( double( dlSubchannelOffset + nbOfUsedSlots) / totalDlSubchannels) * phy->getSlotLength(DL_));
            dlSubchannelOffset = ( dlSubchannelOffset + nbOfUsedSlots) % (totalDlSubchannels);

            // update traffic policing
            if ( currentCon->getType() == CONN_DATA) {
            	trafficShapingAlgorithm_->updateAllocation( currentCon, virtualAlloc->getCurrentMrtrPayload(), virtualAlloc->getCurrentMstrPayload());
            }

            // testing
            entireUsedSlots += nbOfUsedSlots;


            // increase index
            dlMapIeIndex++;
        } while (virtualAlloc->nextConnectionEntry());
        // go to the next connection if existing
    }
    // set the number of information elements
    dlMap->nb_ies = dlMapIeIndex;

    debug_ext("Number of Connection Scheduled %d , Number of Slots used %d \n", dlMapIeIndex, entireUsedSlots);
*/
    mac802_16_dl_map_frame * dlMap = new mac802_16_dl_map_frame;

    // call Burst Mapping Algorithm
    dlMap = dlBurstMappingAlgorithm_->mapDlBursts( dlMap, virtualAlloc, totalDlSubchannels, totalDlSymbols, dlSymbolOffset, dlSubchannelOffset);


    // Update Traffic Shaping
    if ( virtualAlloc->firstConnectionEntry()) {
    	do {

    		Connection * currentCon = virtualAlloc->getConnection();
    		if ( currentCon->getType() ==  CONN_DATA) {
    			trafficShapingAlgorithm_->updateAllocation( currentCon, virtualAlloc->getCurrentMrtrPayload(), virtualAlloc->getCurrentMstrPayload());
    		}
    	} while ( virtualAlloc->nextConnectionEntry());
    }

    // return DL-Map
    return dlMap;

}



/* returns the DIUC profile associated with a current modulation and coding scheme
 * Added by Ritun
 */
diuc_t BSScheduler::getDIUCProfile(Ofdm_mod_rate rate)
{
    Profile *p;

    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_1);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_1;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_2);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_2;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_3);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_3;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_4);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_4;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_5);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_5;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_6);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_6;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_7);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_7;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_8);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_8;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_9);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_9;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_10);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_10;
    p = getMac()->getMap()->getDlSubframe()->getProfile(DIUC_PROFILE_11);
    if (p->getEncoding() == rate)
        return DIUC_PROFILE_11;

    return DIUC_PROFILE_1;
}

/* Added by Ritun
 */
uiuc_t BSScheduler::getUIUCProfile(Ofdm_mod_rate rate)
{
    Profile *p;

    p = getMac()->getMap()->getUlSubframe()->getProfile(UIUC_PROFILE_1);
    if (p->getEncoding() == rate)
        return UIUC_PROFILE_1;
    p = getMac()->getMap()->getUlSubframe()->getProfile(UIUC_PROFILE_2);
    if (p->getEncoding() == rate)
        return UIUC_PROFILE_2;
    p = getMac()->getMap()->getUlSubframe()->getProfile(UIUC_PROFILE_3);
    if (p->getEncoding() == rate)
        return UIUC_PROFILE_3;
    p = getMac()->getMap()->getUlSubframe()->getProfile(UIUC_PROFILE_4);
    if (p->getEncoding() == rate)
        return UIUC_PROFILE_4;
    p = getMac()->getMap()->getUlSubframe()->getProfile(UIUC_PROFILE_5);
    if (p->getEncoding() == rate)
        return UIUC_PROFILE_5;
    p = getMac()->getMap()->getUlSubframe()->getProfile(UIUC_PROFILE_6);
    if (p->getEncoding() == rate)
        return UIUC_PROFILE_6;
    p = getMac()->getMap()->getUlSubframe()->getProfile(UIUC_PROFILE_7);
    if (p->getEncoding() == rate)
        return UIUC_PROFILE_7;
    p = getMac()->getMap()->getUlSubframe()->getProfile(UIUC_PROFILE_8);
    if (p->getEncoding() == rate)
        return UIUC_PROFILE_8;

    return UIUC_PROFILE_1;
}


mac802_16_ul_map_frame * BSScheduler::buildUplinkMap( Connection *head, int totalSubchannels, int totalSymbols, int ulSymbolOffset, int ulSubchannelOffset, int freeUlSlots)
{

    // 1. Traffic Policing for Uplink Connections
    // 2. Schedule Uplink Connections
    // 3. Allocate Uplink Burst
    // 4. Update Traffic Policing



    // why is amc attached to the connections ?
    int ss_mac_id;
    /*AMC mechanism. change the modulation coding scheme in each connections.*/
    if (mac_->amc_enable_ ==1) {

        UlGrantSchedulingType_t ulGrantSchedulingType;
        Connection * con;
        debug2("\n\nGoing to do UL AMC processing.\n\n");
        for (int i=0; i<5; ++i) {
            con = head;
            if (i==0)
                ulGrantSchedulingType = UL_UGS;
            else if (i==1)
                ulGrantSchedulingType = UL_ertPS;
            else if (i==2)
                ulGrantSchedulingType = UL_rtPS;
            else if (i==3)
                ulGrantSchedulingType = UL_nrtPS;
            else
                ulGrantSchedulingType = UL_BE;

            while (con!=NULL) {
                if (con->get_category() == CONN_DATA && con->get_serviceflow()->getQosSet()->getUlGrantSchedulingType() == ulGrantSchedulingType) {
                    /*Xingting would like to change the modulation mode for this SS.*/

                    Ofdm_mod_rate  updated_mod_rate;
                    Ofdm_mod_rate con_mod = mac_->getMap()->getUlSubframe()->getProfile (con->getPeerNode()->getUIUC())->getEncoding();
                    Ofdm_mod_rate mod_rate = con_mod;
                    debug2("UL 1.1 In BS: con_id %d, Modulation %d, UIUC %d\n", con->get_cid(), mod_rate, con->getPeerNode()->getUIUC());

                    if (con!= NULL && con->getPeerNode() != NULL) {
                        ss_mac_id = con->getPeerNode()->getAddr();
                        debug2("ss_mac_id %d The condition is: mac_->get_change_modulation_flag(ss_mac_id) %d    mac_->amc_enable_ %d\n",
                               ss_mac_id, mac_->get_change_ul_modulation_flag(ss_mac_id) , mac_->amc_enable_);

                        if (mac_->get_change_ul_modulation_flag(ss_mac_id) == TRUE && mac_->amc_enable_ == 1) {

                            int current_mcs_index =mac_->get_current_ul_mcs_index(ss_mac_id);

                            bool increase_flag = mac_->get_increase_ul_modulation(ss_mac_id);
                            debug2("UL flag 11, current_mcs_index is %d, increase flag is %d,  mod_rate is %d\n", current_mcs_index, increase_flag, mod_rate);

                            int updated_mcs_index = update_mcs_index(mod_rate,current_mcs_index,increase_flag);
                            bool change_modulation = check_modulation_change(mod_rate, current_mcs_index, increase_flag);

                            debug2("UL flag 22, updated mcs index is %d, change modulation is %d\n",updated_mcs_index,change_modulation);
                            if (change_modulation == TRUE) { // modulation and mcs index are all needed to change.
                                updated_mod_rate = change_rate(mod_rate,increase_flag);
                                con->getPeerNode()->setUIUC(getUIUCProfile(updated_mod_rate));
                                con->getPeerNode()->setCurrentULMcsIndex(updated_mcs_index);
                                mac_->set_current_ul_mcs_index(ss_mac_id, updated_mcs_index);
                                mac_->set_change_ul_modulation_flag(ss_mac_id, false);
                                mac_->set_increase_ul_modulation(ss_mac_id, TRUE);
                                debug2("UL BS Xingting is gonna 1 change SS %d from  modulation from %d to %d\t MCS index from [%d] to [%d].\n",
                                       ss_mac_id, mod_rate, updated_mod_rate, current_mcs_index, updated_mcs_index);
                                debug2("Important 1: set mac_->set_increase_ul_modulation(%d) to %d\n\n",ss_mac_id, mac_->get_increase_ul_modulation(ss_mac_id));
                            } else if (change_modulation == FALSE) {
                                updated_mod_rate = mod_rate;
                                con->getPeerNode()->setUIUC(getUIUCProfile(mod_rate));
                                con->getPeerNode()->setCurrentULMcsIndex(updated_mcs_index);
                                mac_->set_current_ul_mcs_index(ss_mac_id,updated_mcs_index);
                                mac_->set_change_ul_modulation_flag(ss_mac_id, false);
                                mac_->set_increase_ul_modulation(ss_mac_id, TRUE);
                                debug2("UL BS  Xingting is gonna 2 change SS %d from  modulation from %d to %d\t MCS index from [%d] to [%d].\n",
                                       ss_mac_id, mod_rate, updated_mod_rate, current_mcs_index, updated_mcs_index);
                                debug2("Important 2: set mac_->set_increase_ul_modulation(%d) to %d\n\n",ss_mac_id, mac_->get_increase_ul_modulation(ss_mac_id));
                            }
                        }
                    }
                }
                con=con->next_entry();
            }
        }
    }


    // create a virtualAllocation Container for data allocations
    VirtualAllocation * virtualAlloc = new VirtualAllocation;

    // create a virtualAllocation Container for cmda allocations
    // we need a second container as we use the connection object address as unique key
    VirtualAllocation * cdmaAlloc = new VirtualAllocation;

    // Go through the connection list
    Connection * currentCon;
    ConnectionType_t conType;

    // Matrix to detect collisions
    int cdmaTopCodeMatrix[ CDMA_SLOTS][ CODE_SIZE];
    for (int i = 0; i < CDMA_SLOTS; i++) {
        for (int j = 0; j < CODE_SIZE; j++) {
        	cdmaTopCodeMatrix[i][j] = 0;
        }
    }

    // ========================= CHECK FOR CDMA COLLISIONS ======================

    // Check for CDMA_Init_Ranging and cdma_bandwidth_ranging request collisions
    // before resource allocations


    currentCon = head;
    debug10 ("Check for Initial Ranging CDMA collisions \n");
    while ( currentCon != NULL ) {
    	// just Initial Ranging Connections
    	if ( currentCon->get_category() == CONN_INIT_RANGING ) {
            if ( currentCon->getCdma() == 2) {
                int beginFlag = 0;
                u_char beginCode = 0;
                u_char beginTop = 0;
                for ( int i = 0; i < MAX_SSID; i++) {
                    beginFlag = currentCon->getCdmaSsidFlag( i);
                    if ( beginFlag > 0 ) {
                        beginCode = currentCon->getCdmaSsidCode( i);
                        beginTop = currentCon->getCdmaSsidTop( i);
                        for ( int j = i + 1; j < MAX_SSID; j++ ) {
                            if ( currentCon->getCdmaSsidFlag( j) > 0) {
                                if ( (beginCode == currentCon->getCdmaSsidCode(j)) && (beginTop == currentCon->getCdmaSsidTop(j)) ) {
                                    // Collision detected
                                    debug10 ("=Collission CDMA_INIT_RNG_REQ (ssid i :%d and ssid j :%d), CDMA_flag i :%d and CDMA_flag j:%d, CDMA_code i :%d, CDMA_top i :%d\n", currentCon->getCdmaSsidSsid(i), currentCon->getCdmaSsidSsid(j), currentCon->getCdmaSsidFlag(i), currentCon->getCdmaSsidFlag(j), currentCon->getCdmaSsidCode(i), currentCon->getCdmaSsidTop(i));
                                    currentCon->setCdmaSsidFlag(j, 0);
                                    currentCon->setCdmaSsidFlag(i, 0);
                                }
                            }
                        }
                    }
                }
            }
            // set CDMA to zero for all initial ranging connections
            currentCon->setCdma( 0);
    	}
        // goto next connection
        currentCon = currentCon->next_entry();
    }

    // Allocate Resources for Ranging Request
    currentCon = head;
    debug10 ("Allocate Resources for Ranging Request  \n");
    while ( currentCon != NULL ) {
    	// just Initial Ranging Connections
    	if ( currentCon->get_category() == CONN_INIT_RANGING ) {
            for (int i = 0; i<MAX_SSID; i++) {
                int cdmaFlag = currentCon->getCdmaSsidFlag(i);
                if (cdmaFlag > 0) {
                    double allocationSize =  RNG_REQ_SIZE;
                    int slotCapacity = mac_->getPhy()->getSlotCapacity( OFDM_QPSK_1_2, UL_);
                    int nbOfSlots = int( ceil( allocationSize / slotCapacity));
                    // allocation must have at least RNG_REQ_SIZE
                    if ( freeUlSlots >= nbOfSlots) {
                    	debug10("=> Allocate init_ranging_msg opportunity for ssid :%d, code :%d, top :%d, size :%f\n", currentCon->getCdmaSsidSsid(i), currentCon->getCdmaSsidCode(i), currentCon->getCdmaSsidTop(i), allocationSize);
                    	//update free slots
                    	freeUlSlots -= nbOfSlots;

                        // add virtual allocation
                    	cdmaAlloc->addCdmaAllocation( currentCon, slotCapacity, nbOfSlots, currentCon->getCdmaSsidTop(i), currentCon->getCdmaSsidCode(i));


                    }
                    // reset CDMA request
                    currentCon->setCdmaSsidFlag( i, 0);
                    currentCon->setCdmaSsidCode( i, 0);
                    currentCon->setCdmaSsidTop( i, 0);

                }

            }
    	}
        // goto next connection
        currentCon = currentCon->next_entry();
    }

    // Build map for CDMA collisions
    currentCon = head;
    while ( currentCon != NULL) {
        // get type of connection
        conType = currentCon->get_category();
        switch ( conType) {

        case CONN_INIT_RANGING:
        	// place holder for check and handle cdma_init_ranging collisions
            break;
        case CONN_BASIC:
        case CONN_PRIMARY:
        case CONN_SECONDARY:
        case CONN_DATA:
            // for BASIC, PRIMARY, SECONDARY and DATA Connection

            // build up two dimensional array to find collisions in the next step
            if ( currentCon->getCdma() == 1 ) {
            	cdmaTopCodeMatrix[ int( currentCon->getCdmaTop())][ int( currentCon->getCdmaCode()) ]++;
            }
            // For debug
            if (currentCon->getCdma() > 0) {
                debug10 ("=Cid %d, CDMA_flag :%d, CDMA_code :%d, CDMA_top :%d, CDMA_code_top++ :%d\n", currentCon->get_cid(), currentCon->getCdma(), currentCon->getCdmaCode(), currentCon->getCdmaTop(), cdmaTopCodeMatrix[ int( currentCon->getCdmaTop())][ int( currentCon->getCdmaCode()) ]);
            }


           break;
        default:
            // Do nothing
            break;
        } // end switch

        currentCon = currentCon->next_entry();
    } // end while


    //========= Allocation  ==========

    // CDMA allocations

    currentCon = head;
    while (( currentCon != NULL ) && ( freeUlSlots > 0 )) {
        // get type of this connection
        conType = currentCon->get_category();

        switch( conType) {
        case CONN_INIT_RANGING:
            break;

        case CONN_BASIC:
        case CONN_PRIMARY:
        case CONN_SECONDARY:
        case CONN_DATA:

            //Check for cdma_bandwidth_ranging request collisions and allocate request
            if (currentCon->getCdma() == 1) {
                if ( cdmaTopCodeMatrix[ int( currentCon->getCdmaTop())][ int( currentCon->getCdmaCode()) ] > 1 ) {
                    // collision detected

                    debug_ext ("=Collission CDMA_BW_REQ, Cid :%d, CDMA_flag :%d, CDMA_code :%d, CDMA_top :%d, CDMA_code_top :%d\n", currentCon->get_cid(), currentCon->getCdma(), currentCon->getCdmaCode(), currentCon->getCdmaTop(), cdmaTopCodeMatrix[ int( currentCon->getCdmaTop())][ int( currentCon->getCdmaCode()) ]);

                    // reset request
                    currentCon->setCdma( 0);
                    currentCon->setCdmaCode( 0);
                    currentCon->setCdmaTop( 0);
                } else {
                    // allocate resources for CDMA requests
                    double allocationSize = GENERIC_HEADER_SIZE; 					//for explicit polling
                    int slotCapacity = mac_->getPhy()->getSlotCapacity( OFDM_QPSK_1_2, UL_);
                    int nbOfSlots = int( ceil( allocationSize / slotCapacity));
                    debug_ext ("\tUL.Check1.1.contype(?), Polling CDMA :%f, numslots :%d, freeslot :%d, CID :%d\n", allocationSize, nbOfSlots, freeUlSlots, currentCon->get_cid());

                    // Allocation of less slots than needed for an BW REQ makes no sense
                    if (freeUlSlots >= nbOfSlots) {

                    	freeUlSlots -= nbOfSlots;
                        // add virtual allocation
                        // Ofdm_mod_rate burstProfile = mac_->getMap()->getUlSubframe()->getProfile(currentCon->getPeerNode()->getUIUC())->getEncoding();
                        int slotCapacity = mac_->getPhy()->getSlotCapacity( OFDM_QPSK_1_2, UL_);
                        cdmaAlloc->addCdmaAllocation( currentCon, slotCapacity, nbOfSlots, currentCon->getCdmaTop(), currentCon->getCdmaCode());

                        // CDMA Request is fulfilled
                        currentCon->setCdma( 0);

                    } else {
                    	printf("Resources for BW Req are missing \n");
                    }

                }
            }


            break;

        default:
            break;

        } // end switch

        // goto next connection
        currentCon = currentCon->next_entry();
    } // end while


    // Data allocation for management connections
    currentCon = head;
    while ( currentCon != NULL) {
        // get type of connection
        conType = currentCon->get_category();

        // used variables
    	int allocationSize = 0;
    	int remainingBwReq = 0;
    	int slotCapacity = 0;
    	int nbOfSlots = 0;

        switch ( conType) {

        case CONN_BASIC:
        case CONN_PRIMARY:
        case CONN_SECONDARY:
            // for BASIC, PRIMARY and SECONDARY Connection
        	// all these management connections will be handled as MRTR requests

            // data to send ?
             if ( currentCon->getBw() > 0 ) {
                 // allocate ressources for management data
                 Ofdm_mod_rate burstProfile = mac_->getMap()->getUlSubframe()->getProfile(currentCon->getPeerNode()->getUIUC())->getEncoding();
                 slotCapacity = mac_->getPhy()->getSlotCapacity( burstProfile, UL_);

                 allocationSize = currentCon->getBw();
                 nbOfSlots = int( ceil( double(allocationSize) / slotCapacity));
                 if (freeUlSlots < nbOfSlots) {
                     nbOfSlots = freeUlSlots;
                 }
                 // ceil allocationSize to avoid scheduling issues
                 // may waste some bytes
                 allocationSize = nbOfSlots * slotCapacity;

				 // decrement bandwidth requested
                 remainingBwReq = currentCon->getBw() - allocationSize;
				 if (remainingBwReq < 0) {
					 remainingBwReq = 0;
				 }
                 currentCon->setBw( remainingBwReq);

				 // decrease number of free slots
                 freeUlSlots -= nbOfSlots;

    			 if ( nbOfSlots > 0) {

    				 // debug
    				 debug_ext("CID %d, Basic IN CID %d, Basic OUT CID %d, allocationSize %d , nbOfSlots %d \n", currentCon->get_cid(), currentCon->getPeerNode()->getBasic( IN_CONNECTION)->get_cid(), currentCon->getPeerNode()->getBasic( OUT_CONNECTION)->get_cid(), allocationSize, nbOfSlots);


    				 // search for allocation
    			//	 if ( virtualAlloc->findConnectionEntry( currentCon) ) {
    					 // connection found

    					 // remove cdma flags
    					 //virtualAlloc->resetCdma();

    					 // increase assigned slots
    					 //virtualAlloc->updateAllocation( virtualAlloc->getCurrentNbOfBytes() + allocationSize, virtualAlloc->getCurrentNbOfSlots() + nbOfSlots, 0, 0);

    					 // slots are directly assigned an are not handeled by scheduler

    					 // virtualAlloc->updateWantedMrtrMstr( virtualAlloc->getWantedMrtrSize() + allocationSize, virtualAlloc->getWantedMstrSize() + allocationSize);
    				// } else {
    					 // connection has no assigned resources
    					 // add new entry

    					 // resources are directly assigned an are not handeled by scheduler
    					 // virtualAlloc->addAllocation( currentCon->getPeerNode()->getBasic( IN_CONNECTION), 0, 0, slotCapacity, nbOfSlots , allocationSize);
    					 virtualAlloc->addAllocation( currentCon, 0, 0, slotCapacity, allocationSize, nbOfSlots);
    				// }



    			 }

             }

            break;
        default:
            // Do nothing
            break;
        } // end switch

        currentCon = currentCon->next_entry();
    } // end while


    /*
     * Resource allocation for data connections
     *
     * Each data connection is handled separately
     * 1. Calculate Demand
     * 2. Apply Traffic Shaping
     * 3. Scheduling
     * 4. Aggregate resources to one BASIC CID allocation
     */

    currentCon = head;
    MrtrMstrPair_t mrtrMstrPair;
    ServiceFlowQosSet * sfQosSet;


    // TODO: Avoid this ugly hack
    double overheadFactor = 1.00;


    while ( currentCon != NULL) {
        // get type of connection
        conType = currentCon->get_category();

        // consider that the queue size is unknown to the BS
        // check if this connection is active

        if (( conType == CONN_DATA ) && ( currentCon->getServiceFlow()->getServiceFlowState() == ACTIVE )) {


        	// consider that the queue size is unknown to the BS

			sfQosSet = currentCon->getServiceFlow()->getQosSet();

			// handle grants and polling intervals
			switch ( sfQosSet->getUlGrantSchedulingType() ) {

			case UL_UGS:
				if (( NOW - (( sfQosSet->getGrantInterval() - TIMEERRORTOLERANCE) * 1e-3)) >= currentCon->getLastAllocationTime()) {

					mrtrMstrPair = trafficShapingAlgorithm_->getDataSizes( currentCon, sfQosSet->getMaxTrafficBurst());
					if ( mrtrMstrPair.first > 0) {
//						if ( virtualAlloc->findConnectionEntry( currentCon) ) {
//
//							// Not POSSIBLE
//							assert( false);
//
//							// update wanted Sizes
//							virtualAlloc->updateWantedMrtrMstr( virtualAlloc->getWantedMrtrSize() + u_int32_t( mrtrMstrPair.first * overheadFactor), virtualAlloc->getWantedMstrSize() + u_int32_t( mrtrMstrPair.second * overheadFactor ));
//						} else {

							// create new entry
							Ofdm_mod_rate burstProfile = mac_->getMap()->getUlSubframe()->getProfile(currentCon->getPeerNode()->getUIUC())->getEncoding();
							int slotCapacity = mac_->getPhy()->getSlotCapacity( burstProfile, UL_);
							virtualAlloc->addAllocation( currentCon, u_int32_t( mrtrMstrPair.first * overheadFactor), u_int32_t( mrtrMstrPair.second * overheadFactor ), slotCapacity);
//						}
					}
				}
			break;
			case UL_ertPS:

				// TODO : BW Request needed for ertPS

				if (( NOW - (( sfQosSet->getGrantInterval() - TIMEERRORTOLERANCE )* 1e-3)) >= currentCon->getLastAllocationTime()) {

					mrtrMstrPair = trafficShapingAlgorithm_->getDataSizes( currentCon, sfQosSet->getMaxTrafficBurst());
					if ( mrtrMstrPair.second > 0) {
//						if ( virtualAlloc->findConnectionEntry( currentCon) ) {
//
//							// Not POSSIBLE
//							assert( false);
//
//							// update wanted Sizes
//							virtualAlloc->updateWantedMrtrMstr( virtualAlloc->getWantedMrtrSize() + MIN( u_int32_t( mrtrMstrPair.first * overheadFactor), u_int32_t( currentCon->getBw())), virtualAlloc->getWantedMstrSize() + MIN( u_int32_t( mrtrMstrPair.second * overheadFactor), u_int32_t( currentCon->getBw())));
//						} else {
//
//							// create new entry
							Ofdm_mod_rate burstProfile = mac_->getMap()->getUlSubframe()->getProfile(currentCon->getPeerNode()->getUIUC())->getEncoding();
							int slotCapacity = mac_->getPhy()->getSlotCapacity( burstProfile, UL_);
							//virtualAlloc->addAllocation( currentCon, MIN( u_int32_t( mrtrMstrPair.first * overheadFactor), u_int32_t( currentCon->getBw())), MIN( u_int32_t( mrtrMstrPair.second * overheadFactor), u_int32_t( currentCon->getBw())), mrtrMstrPair.second, slotCapacity);
							virtualAlloc->addAllocation( currentCon, u_int32_t( mrtrMstrPair.first * overheadFactor), u_int32_t( mrtrMstrPair.second * overheadFactor ), slotCapacity);
//						}
					}
				}
			break;
			case UL_rtPS:


				mrtrMstrPair = trafficShapingAlgorithm_->getDataSizes( currentCon, u_int32_t(currentCon->getBw()));

				// Unicast Polling according to Polling Interval
				if (( NOW - ((sfQosSet->getPollingInterval() - TIMEERRORTOLERANCE )* 1e-3)) >= currentCon->getLastAllocationTime()) {
					mrtrMstrPair.first += HDR_MAC802_16_SIZE;
					mrtrMstrPair.second += HDR_MAC802_16_SIZE;
				}

				if ( mrtrMstrPair.second > 0 ) {

//					if ( virtualAlloc->findConnectionEntry( currentCon) ) {
//
//						// Not POSSIBLE
//						assert( false);
//
//						// update wanted Sizes
//						virtualAlloc->updateWantedMrtrMstr( virtualAlloc->getWantedMrtrSize() + u_int32_t( mrtrMstrPair.first * overheadFactor), virtualAlloc->getWantedMstrSize() + u_int32_t( mrtrMstrPair.second * overheadFactor ));
//					} else {

						// create new entry
						Ofdm_mod_rate burstProfile = mac_->getMap()->getUlSubframe()->getProfile(currentCon->getPeerNode()->getUIUC())->getEncoding();
						int slotCapacity = mac_->getPhy()->getSlotCapacity( burstProfile, UL_);
						virtualAlloc->addAllocation( currentCon, mrtrMstrPair.first , mrtrMstrPair.second, slotCapacity);
//					}
				}



				break;
			case UL_nrtPS:

				mrtrMstrPair = trafficShapingAlgorithm_->getDataSizes( currentCon, u_int32_t(currentCon->getBw()));

				// TODO: Ugly

				// Unicast Polling according to Time Base
				if (( NOW - ((sfQosSet->getTimeBase() - TIMEERRORTOLERANCE) * 1e-3)) >= currentCon->getLastAllocationTime()) {
					mrtrMstrPair.first += HDR_MAC802_16_SIZE;
					mrtrMstrPair.second += HDR_MAC802_16_SIZE;
				}


				if ( mrtrMstrPair.second > 0 ) {

				//	if ( virtualAlloc->findConnectionEntry( currentCon) ) {


				//		if ( virtualAlloc->isCdmaAllocation()) {
							// remove allocation granted for cdma request
				//			virtualAlloc->resetCdma();

							// TODO: Scheduler will take the CDMA allocation into account

							// update wanted Sizes
//				//			virtualAlloc->updateWantedMrtrMstr( HDR_MAC802_16_SIZE + u_int32_t( mrtrMstrPair.first * overheadFactor), HDR_MAC802_16_SIZE + u_int32_t( mrtrMstrPair.second * overheadFactor ));
//							printf("Debug addAllocation CID %d, Req-BW %d, MRTR %d, MSTR %d \n", currentCon->get_cid(), currentCon->getBw(), virtualAlloc->getWantedMrtrSize() + u_int32_t( mrtrMstrPair.first * overheadFactor), virtualAlloc->getWantedMstrSize() + u_int32_t( mrtrMstrPair.second * overheadFactor ) );
//						} else {
							// Not POSSIBLE
//							assert( false);
//						}
//
			//		} else {

						// create new entry
						Ofdm_mod_rate burstProfile = mac_->getMap()->getUlSubframe()->getProfile(currentCon->getPeerNode()->getUIUC())->getEncoding();
						int slotCapacity = mac_->getPhy()->getSlotCapacity( burstProfile, UL_);
						virtualAlloc->addAllocation( currentCon, mrtrMstrPair.first , mrtrMstrPair.second, slotCapacity);
						printf("Debug addAllocation CID %d, Req-BW %d, MRTR %d, MSTR %d \n", currentCon->get_cid(), currentCon->getBw(), virtualAlloc->getWantedMrtrSize() + u_int32_t( mrtrMstrPair.first * overheadFactor), virtualAlloc->getWantedMstrSize() + u_int32_t( mrtrMstrPair.second * overheadFactor ) );

//					}
				}


			break;
			case UL_BE:
				mrtrMstrPair = trafficShapingAlgorithm_->getDataSizes( currentCon, u_int32_t(currentCon->getBw()));

				/*
    		    if (mac_->debugfile_.is_open())
    		     {
    		    	mac_->debugfile_ << "At "<< NOW << " Traffic Shaping CID "<< currentCon->get_cid() << "Req BW " <<  currentCon->getBw()
    		    			<< " MRTR " << mrtrMstrPair.first << " MSTR " << mrtrMstrPair.second << endl;
    		     }
    		     */

				if ( mrtrMstrPair.second > 0 ) {

//					if ( virtualAlloc->findConnectionEntry( currentCon) ) {
//
//
//						if ( virtualAlloc->isCdmaAllocation()) {
//							// remove allocation granted for cdma request
//							virtualAlloc->resetCdma();
//
//							// TODO: Scheduler will take the CDMA allocation into account
//
//							// update wanted Sizes
//							virtualAlloc->updateWantedMrtrMstr( HDR_MAC802_16_SIZE + u_int32_t( mrtrMstrPair.first * overheadFactor), HDR_MAC802_16_SIZE + u_int32_t( mrtrMstrPair.second * overheadFactor ));
//							printf("Debug addAllocation CID %d, Req-BW %d, MRTR %d, MSTR %d \n", currentCon->get_cid(), currentCon->getBw(), virtualAlloc->getWantedMrtrSize() + u_int32_t( mrtrMstrPair.first * overheadFactor), virtualAlloc->getWantedMstrSize() + u_int32_t( mrtrMstrPair.second * overheadFactor ) );
//						} else {
//							// Not POSSIBLE
//							assert( false);
//						}
//					} else {

						// create new entry
						Ofdm_mod_rate burstProfile = mac_->getMap()->getUlSubframe()->getProfile(currentCon->getPeerNode()->getUIUC())->getEncoding();
						int slotCapacity = mac_->getPhy()->getSlotCapacity( burstProfile, UL_);
						virtualAlloc->addAllocation( currentCon, mrtrMstrPair.first , mrtrMstrPair.second, slotCapacity);
						printf("Debug addAllocation CID %d, Req-BW %d, MRTR %d, MSTR %d \n", currentCon->get_cid(), currentCon->getBw(), mrtrMstrPair.first, mrtrMstrPair.second );
//					}
				}

			break;
			default:

				// all all data connection should have an Up Grant Scheduling Type
				assert(false);
				break;

			} // end switch

        } // end DATA_CONN and SF active
        currentCon = currentCon->next_entry();
    } // end while


    // ========= Run Scheduling Algorithm =================
    ulSchedulingAlgorithm_->scheduleConnections( virtualAlloc, freeUlSlots);


    // ========= Map Virtual Allocations to Uplink Map ====

    // Allocate CDMA Ranging / BW Req opportunities

    // All data and management resources for subscriber have to be allocated to its basic cid



    mac802_16_ul_map_frame * ulMap = new mac802_16_ul_map_frame;
    ulMap->type = MAC_UL_MAP;
    int ulMapIeIndex = 0;


    // allocate cdma allocations
    if ( cdmaAlloc->firstConnectionEntry() ) {
    	// loop over all allocations

    	do {

			// map CDMA Ranging or BW Req opportunity

			Connection * currentCon = cdmaAlloc->getConnection();

			ulMap->ies[ulMapIeIndex].cid = currentCon->get_cid();
			// default Profile is used now
			// UIUC should be 12 according to IEEE 802.16-2009 table 377 p. 824
			ulMap->ies[ulMapIeIndex].uiuc = getUIUCProfile(OFDM_QPSK_1_2);
			ulMap->ies[ulMapIeIndex].symbol_offset = ulSymbolOffset;
			ulMap->ies[ulMapIeIndex].subchannel_offset = ulSubchannelOffset;

			int nbOfSlots = cdmaAlloc->getCurrentNbOfSlots();
			int nbOfSymbols = int( ceil( double (ulSubchannelOffset + nbOfSlots ) / totalSubchannels) * 3 );

			ulMap->ies[ulMapIeIndex].num_of_symbols = nbOfSymbols;
			ulMap->ies[ulMapIeIndex].num_of_subchannels = nbOfSlots;

			// Update Offsets
			ulSymbolOffset += int( floor( ulSubchannelOffset + nbOfSlots) / totalSubchannels) * 3;
			ulSubchannelOffset = (ulSubchannelOffset + nbOfSlots) % (totalSubchannels);

			ulMap->ies[ulMapIeIndex].cdma_ie.subchannel = cdmaAlloc->getCurrentCdmaTop();
			ulMap->ies[ulMapIeIndex].cdma_ie.code = cdmaAlloc->getCurrentCdmaCode();

			printf("CDMA Allocation CID %d Slots %d Top %d Code %d\n",currentCon->get_cid(), nbOfSlots, cdmaAlloc->getCurrentCdmaTop(), cdmaAlloc->getCurrentCdmaCode());

			/*
			if (mac_->debugfile_.is_open())
			 {
				mac_->debugfile_ << "At "<< NOW << "CDMA Alloc CID "<< currentCon->get_cid() <<" CDMA Allocation with Bytes" <<   (cdmaAlloc->getCurrentNbOfSlots() * cdmaAlloc->getSlotCapacity()) << endl;
			 }
			*/

			ulMapIeIndex++;

    		// go to next connection
    	} while ( cdmaAlloc->nextConnectionEntry());

    }

	// debug
	printf("Number of Init Ranging and BW Req Opportunities %d \n", ulMapIeIndex);



    // aggregate data allocations and send feedback to traffic shaping

    // create new container to aggregate allocations
    VirtualAllocation * basicAlloc = new VirtualAllocation;

    // get first allocation
    if ( virtualAlloc->firstConnectionEntry() ) {
    	// loop over all allocations

    	do {

			// build up basicAlloc container to aggregate allocations

			Connection * currentCon = virtualAlloc->getConnection();
			printf("CID %d, Basic CID %d Slots %d\n", currentCon->get_cid(), currentCon->getPeerNode()->getBasic( OUT_CONNECTION)->get_cid(), virtualAlloc->getCurrentNbOfSlots());
			assert( currentCon->getPeerNode()->getBasic( IN_CONNECTION)->get_cid() == currentCon->getPeerNode()->getBasic( OUT_CONNECTION)->get_cid());


			/*
			if (mac_->debugfile_.is_open())
			 {
				mac_->debugfile_ << "At "<< NOW << "CID "<< currentCon->get_cid() <<" Basic CID "<< currentCon->getPeerNode()->getBasic( OUT_CONNECTION)->get_cid() <<
						" Bytes " <<  (virtualAlloc->getCurrentNbOfSlots() * virtualAlloc->getSlotCapacity()) << endl;
			 }
			*/


			int slotCapacity = virtualAlloc->getSlotCapacity();
			int nbOfBytes = virtualAlloc->getCurrentNbOfBytes();
			int nbOfSlots = virtualAlloc->getCurrentNbOfSlots();


			Connection * basicCon = currentCon->getPeerNode()->getBasic( OUT_CONNECTION);

			if ( basicAlloc->findConnectionEntry (basicCon)) {
				// entry exists
				nbOfBytes += basicAlloc->getCurrentNbOfBytes();
				nbOfSlots += basicAlloc->getCurrentNbOfSlots();


				basicAlloc->updateAllocation( nbOfBytes, nbOfSlots,  0, 0);
			} else {
				// create new entry
				basicAlloc->addAllocation( basicCon, 0, 0, slotCapacity, nbOfBytes, nbOfSlots);
			}

			if ( currentCon->getType() == CONN_DATA) {

				ServiceFlowQosSet * sfQosSet = currentCon->getServiceFlow()->getQosSet();

				int requestedBandwidth = currentCon->getBw();
				int receivedBandwidth = 0;

				if (( sfQosSet->getUlGrantSchedulingType() == UL_rtPS)
					&& (( NOW - (sfQosSet->getPollingInterval() * 1e-3)) >= currentCon->getLastAllocationTime())) {

					// Reduce feedback size if BW Req was send
					trafficShapingAlgorithm_->updateAllocation( currentCon, u_int32_t(virtualAlloc->getCurrentMrtrPayload() / overheadFactor - HDR_MAC802_16_SIZE),
							u_int32_t(virtualAlloc->getCurrentMstrPayload() / overheadFactor - HDR_MAC802_16_SIZE));

					receivedBandwidth = virtualAlloc->getCurrentNbOfBytes() - HDR_MAC802_16_SIZE;

				} else if (( sfQosSet->getUlGrantSchedulingType() == UL_nrtPS)
						&& (( NOW - (sfQosSet->getTimeBase() * 1e-3)) >= currentCon->getLastAllocationTime())) {

					// Reduce feedback size if BW Req was send
					trafficShapingAlgorithm_->updateAllocation( currentCon, u_int32_t(virtualAlloc->getCurrentMrtrPayload() / overheadFactor - HDR_MAC802_16_SIZE),
													u_int32_t(virtualAlloc->getCurrentMstrPayload() / overheadFactor - HDR_MAC802_16_SIZE));

					receivedBandwidth = virtualAlloc->getCurrentNbOfBytes() - HDR_MAC802_16_SIZE;
				} else {

					// Feedback to traffic shaping
					trafficShapingAlgorithm_->updateAllocation( currentCon, u_int32_t(virtualAlloc->getCurrentMrtrPayload() / overheadFactor), u_int32_t(virtualAlloc->getCurrentMstrPayload() / overheadFactor));

					receivedBandwidth = virtualAlloc->getCurrentNbOfBytes();
				}
				// update last allocation time
				if ( virtualAlloc->getCurrentNbOfBytes() > 0)  {
					currentCon->setLastAllocationTime( NOW);
				}

				// update requested bandwidth
				currentCon->setBw(requestedBandwidth - receivedBandwidth);
				// debug
				printf("Connection %d Requested BW %d Received BW %d \n", currentCon->get_cid(), requestedBandwidth, receivedBandwidth);

			}

    		// go to next connection
    	} while ( virtualAlloc->nextConnectionEntry());

    }




    // map data allocations

    // get first allocation
    if ( basicAlloc->firstConnectionEntry() ) {
    	// loop over all allocations

    	do {


			// map CDMA Ranging or BW Req opportunity

			Connection * basicCon = basicAlloc->getConnection();
			printf("CID %d \n",basicCon->get_cid());

			ulMap->ies[ulMapIeIndex].cid = basicCon->get_cid();
			// default Profile is used now
			// UIUC should be 12 according to IEEE 802.16-2009 table 377 p. 824
			ulMap->ies[ulMapIeIndex].uiuc = getUIUCProfile(mac_->getMap()->getUlSubframe()->getProfile(basicCon->getPeerNode()->getUIUC())->getEncoding());
			ulMap->ies[ulMapIeIndex].symbol_offset = ulSymbolOffset;
			ulMap->ies[ulMapIeIndex].subchannel_offset = ulSubchannelOffset;

			int nbOfSlots = basicAlloc->getCurrentNbOfSlots();
			int nbOfSymbols = int( ceil( double (ulSubchannelOffset + nbOfSlots ) / totalSubchannels) * 3 );

			ulMap->ies[ulMapIeIndex].num_of_symbols = nbOfSymbols;
			ulMap->ies[ulMapIeIndex].num_of_subchannels = nbOfSlots;

            // Update Offsets
            ulSymbolOffset += int( floor( ulSubchannelOffset + nbOfSlots) / totalSubchannels) * 3;
            ulSubchannelOffset = (ulSubchannelOffset + nbOfSlots) % (totalSubchannels);

			ulMap->ies[ulMapIeIndex].cdma_ie.subchannel = -1;
			ulMap->ies[ulMapIeIndex].cdma_ie.code = -1;

			ulMapIeIndex++;

    		// go to next connection
    	} while ( basicAlloc->nextConnectionEntry());

    }


	// debug
	printf("Number of Burst %d \n", ulMapIeIndex);

    // set the number of information elements
    ulMap->nb_ies = ulMapIeIndex;

    // delete virtual allocation container for data connections
    delete virtualAlloc;
    // delete virtual allocation container for cdma
    delete cdmaAlloc;
    // delete basic Cid container
    delete basicAlloc;

    return ulMap;
}




int BSScheduler::update_mcs_index(Ofdm_mod_rate current_rate, int current_mcs_index, bool increase)
{
    int lower_mcs_bound = -1;
    int upper_mcs_bound = -1;

    switch (current_rate) {
    case OFDM_BPSK_1_2:
        if (current_mcs_index!=1) {
            debug2("Mismatch modulation and MCS index in OFDMA BPSK 1/2.Set it to 1\n");
            //return 1;
            current_mcs_index = 1;
        }
        if (increase == TRUE) {
            current_mcs_index++;
            return current_mcs_index;
        } else if (increase == FALSE) {
            return 1;
        }

    case OFDM_QPSK_1_2:
        lower_mcs_bound = 1;
        upper_mcs_bound = 9;
        if (current_mcs_index<1) {
            debug2("Mismatch modulation and MCS index in OFDMA QPSK 1/2.\n");
            return 1;
        } else if (current_mcs_index> 9) {
            debug2("Mismatch modulation and MCS index in OFDMA QPSK 1/2.\n");
            return 9;
        }
        if (increase == TRUE) {
            current_mcs_index++;
            return current_mcs_index;
        } else if (increase == FALSE) {
            if (current_mcs_index==1)
                return 1;
            else {
                //current_mcs_index--;
                current_mcs_index = current_mcs_index -ROLL_DOWN_FACTOR_2;
                if (current_mcs_index <1)
                    return 1;
                else
                    return current_mcs_index;
            }
        }
        break;

    case OFDM_QPSK_3_4:
        lower_mcs_bound = 10;
        upper_mcs_bound = 15;

        if (current_mcs_index<10) {
            debug2("Mismatch modulation and MCS index in OFDMA QPSK 3/4.\n");
            return 10;
        } else if (current_mcs_index> 15) {
            debug2("Mismatch modulation and MCS index in OFDMA QPSK 3/4.\n");
            return 15;
        }
        if (increase == TRUE) {
            current_mcs_index++;
            return current_mcs_index;
        } else if (increase == FALSE) {
            current_mcs_index = current_mcs_index -ROLL_DOWN_FACTOR_1;
            return current_mcs_index;
        }

        break;

    case OFDM_16QAM_1_2:
        lower_mcs_bound = 16;
        upper_mcs_bound = 20;
        if (current_mcs_index<16) {
            debug2("Mismatch modulation and MCS index in OFDMA 16QAM 1/2.\n");
            return 16;
        } else if (current_mcs_index> 20) {
            debug2("Mismatch modulation and MCS index in OFDMA 16QAM 1/2.\n");
            return 20;
        }

        if (increase == TRUE) {
            current_mcs_index++;
            return current_mcs_index;
        } else if (increase == FALSE) {
            //current_mcs_index--;
            current_mcs_index = current_mcs_index -ROLL_DOWN_FACTOR_1;
            return current_mcs_index;
        }

        break;

    case OFDM_16QAM_3_4:
        lower_mcs_bound = 21;
        upper_mcs_bound = 23;
        if (current_mcs_index<21 && current_mcs_index> 23) {
            debug2("Mismatch modulation and MCS index in OFDMA 16QAM 3/4.\n");
            return 21;
        } else if (increase == TRUE) {
            if (current_mcs_index == 23) {
                return 27;
            } else {
                current_mcs_index++;
            }
            return current_mcs_index;
        } else if (increase == FALSE) {
            //current_mcs_index--;
            current_mcs_index = current_mcs_index -ROLL_DOWN_FACTOR_1;
            return current_mcs_index;
        }
        break;

    case OFDM_64QAM_2_3:
        lower_mcs_bound = 27;
        upper_mcs_bound = 28;

        if (current_mcs_index!=27 && current_mcs_index!= 28) {
            debug2("Mismatch modulation and MCS index in OFDMA 64 QAM 2/3.\n");
            return 27;
        }

        if (increase == TRUE && current_mcs_index == 28) {
            return 29;
        } else if (increase == FALSE && current_mcs_index == 27) {
            return 23;
        } else if (increase == TRUE && current_mcs_index == 27) {
            ++current_mcs_index;
            return current_mcs_index;
        } else if (increase == FALSE && current_mcs_index == 28) {
            --current_mcs_index;
            return current_mcs_index;
        }
        break;

    case OFDM_64QAM_3_4:
        lower_mcs_bound = 29;
        upper_mcs_bound = 30;

        if (current_mcs_index!=29 &&  current_mcs_index!=30) {
            debug2("Mismatch modulation and MCS index in OFDMA 64 QAM 3/4.\n");
            return 29;
        }

        if (increase == TRUE && current_mcs_index >= 30) {
            return 30;
        } else if (increase == FALSE && current_mcs_index <= 29) {
            return 28;
        } else if (increase == TRUE && current_mcs_index == 29) {
            current_mcs_index++;
            return current_mcs_index;
        } else if (increase == FALSE && current_mcs_index == 30) {
            current_mcs_index--;
            return current_mcs_index;
        }
        break;
    default:
        debug2("Input parameters are not reasonable. current_mod %d, current_mcs_index %d, increase %d\n",
               current_rate, current_mcs_index, increase);
        return FALSE;
        break;
    }
    return FALSE;
}

bool BSScheduler::check_modulation_change(Ofdm_mod_rate current_rate, int current_mcs_index, bool increase)
{
    int lower_mcs_bound = -1;
    int upper_mcs_bound = -1;

    switch (current_rate) {
    case OFDM_BPSK_1_2:
        if (increase == TRUE)
            return TRUE;
        else
            return FALSE;

    case OFDM_QPSK_1_2:
        lower_mcs_bound = 1;
        upper_mcs_bound = 9;
        if ((increase == TRUE && ((++current_mcs_index)> 9))
                || (increase == FALSE && ((current_mcs_index-ROLL_DOWN_FACTOR_2) < 1))) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;

    case OFDM_QPSK_3_4:
        lower_mcs_bound = 10;
        upper_mcs_bound = 15;
        if ((increase == TRUE && ((++current_mcs_index)> 15))
                || (increase == FALSE && ((current_mcs_index-ROLL_DOWN_FACTOR_1) < 10))) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;

    case OFDM_16QAM_1_2:
        lower_mcs_bound = 16;
        upper_mcs_bound = 20;
        if ((increase == TRUE && ((++current_mcs_index) > 20))
                || (increase == FALSE && ((current_mcs_index - ROLL_DOWN_FACTOR_1 ) < 16))) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;

    case OFDM_16QAM_3_4:
        lower_mcs_bound = 21;
        upper_mcs_bound = 23;
        if ((increase == TRUE && ((++current_mcs_index) > 23))
                || (increase == FALSE && ((current_mcs_index - ROLL_DOWN_FACTOR_1 ) < 21))) {
            return TRUE;
        } else {
            return FALSE;
        }

        break;

    case OFDM_64QAM_2_3:
        lower_mcs_bound = 27;
        upper_mcs_bound = 28;
        if ((increase == TRUE && current_mcs_index == 28) || (increase == FALSE && current_mcs_index == 27)) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;

    case OFDM_64QAM_3_4:
        lower_mcs_bound = 29;
        upper_mcs_bound = 30;
        if (increase == FALSE && current_mcs_index == 29) {
            return TRUE;
        } else {
            return FALSE;
        }
        break;
    default:
        debug2("Input parameters are not reasonable. current_mod %d, current_mcs_index %d, increase %d\n",
               current_rate, current_mcs_index, increase);
        return FALSE;
        break;
    }
}


Ofdm_mod_rate BSScheduler::change_rate(Ofdm_mod_rate rate, bool increase_modulation)
{
    Ofdm_mod_rate after_rate;
    switch (rate) {
    case OFDM_BPSK_1_2:
        after_rate = (increase_modulation)? OFDM_QPSK_1_2 : OFDM_BPSK_1_2;
        break;

    case OFDM_QPSK_1_2:
        after_rate = (increase_modulation)?OFDM_QPSK_3_4 : OFDM_BPSK_1_2;
        break;

    case OFDM_QPSK_3_4:
        after_rate = (increase_modulation)?OFDM_16QAM_1_2 : OFDM_QPSK_1_2;
        break;

    case OFDM_16QAM_1_2:
        after_rate = (increase_modulation)?OFDM_16QAM_3_4 : OFDM_QPSK_3_4;
        break;

    case OFDM_16QAM_3_4:
        after_rate = (increase_modulation)?OFDM_64QAM_2_3 : OFDM_16QAM_1_2;
        break;

    case OFDM_64QAM_2_3:
        after_rate = (increase_modulation)?OFDM_64QAM_3_4 : OFDM_16QAM_3_4;
        break;

    case OFDM_64QAM_3_4:
        after_rate = (increase_modulation)?OFDM_64QAM_3_4 : OFDM_64QAM_2_3;
        break;

    default:
        after_rate = OFDM_QPSK_3_4;
        break;
    }
    return after_rate;
}

/**
 * Returns the statistic for the downlink scheduling
 */
frameUsageStat_t BSScheduler::getDownlinkStatistic()
{
	return dlSchedulingAlgorithm_->getUsageStatistic();
}

/*
 * Returns the statistic for the uplink scheduling
 */
frameUsageStat_t BSScheduler::getUplinkStatistic()
{
	return ulSchedulingAlgorithm_->getUsageStatistic();
}

