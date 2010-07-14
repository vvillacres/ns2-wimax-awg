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

#define FRAME_SIZE 0.005
#define CODE_SIZE 256
#define CDMA_6SUB 6
#define MAX_CONN 2048
#define ROLL_DOWN_FACTOR_1 2
#define ROLL_DOWN_FACTOR_2 3

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
    default_mod_ = OFDM_BPSK_1_2;
    //bind ("dlratio_", &dlratio_);
    bind("Repetition_code_", &Repetition_code_);
    bind("init_contention_size_", &init_contention_size_);
    bind("bw_req_contention_size_", &bw_req_contention_size_);

//  contention_size_ = MIN_CONTENTION_SIZE;

    nextDL_ = -1;
    nextUL_ = -1;
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
            if (strcmp(argv[2], "OFDM_BPSK_1_2") == 0)
                default_mod_ = OFDM_BPSK_1_2;
            else if (strcmp(argv[2], "OFDM_QPSK_1_2") == 0)
                default_mod_ = OFDM_QPSK_1_2;
            else if (strcmp(argv[2], "OFDM_QPSK_3_4") == 0)
                default_mod_ = OFDM_QPSK_3_4;
            else if (strcmp(argv[2], "OFDM_16QAM_1_2") == 0)
                default_mod_ = OFDM_16QAM_1_2;
            else if (strcmp(argv[2], "OFDM_16QAM_3_4") == 0)
                default_mod_ = OFDM_16QAM_3_4;
            else if (strcmp(argv[2], "OFDM_64QAM_2_3") == 0)
                default_mod_ = OFDM_64QAM_2_3;
            else if (strcmp(argv[2], "OFDM_64QAM_3_4") == 0)
                default_mod_ = OFDM_64QAM_3_4;
            else
                return TCL_ERROR;
            return TCL_OK;
        } else if (strcmp(argv[1], "set-contention-size") == 0) {
            contention_size_ = atoi (argv[2]);
#ifdef DEBUG_WIMAX
            assert (contention_size_>=0);
#endif
            return TCL_OK;
        }
        /*
            else if (strcmp(argv[1], "set-init-contention-size") == 0) {
              init_contention_size_ = atoi (argv[2]);
        #ifdef DEBUG_WIMAX
              assert (init_contention_size_>=0);
        #endif
              return TCL_OK;
            }
            else if (strcmp(argv[1], "set-bw-req-contention-size") == 0) {
              bw_req_contention_size_ = atoi (argv[2]);
        #ifdef DEBUG_WIMAX
              assert (bw_req_contention_size_>=0);
        #endif
              return TCL_OK;
            }
        */



    }
    return TCL_ERROR;
}

/**
 * Initializes the scheduler
 */
void BSScheduler::init ()
{
    WimaxScheduler::init();

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

// This function is used to increase #DL_MAP_IEs (DL_MAP size)
int BSScheduler::increase_dl_map_ie(int num_of_entries, int totalslots, int num_ie)
{

    OFDMAPhy *phy = mac_->getPhy();
    int slots_sofar = 0;
    float dl_ie_size = DL_MAP_IE_SIZE;
//padding to byte based
    float dl_map_increase_byte = virtual_alloc[0].byte + num_ie*dl_ie_size;
    int dl_map_increase_slots = virtual_alloc[0].rep*(int)ceil(ceil(dl_map_increase_byte)/(double)phy->getSlotCapacity(virtual_alloc[0].mod,DL_));

    debug10 ("Check_DLMAP.From array DL MAP byte :%f, slots :%d, IE_SIZE :%f, slotcapacity :%d, #ie :%d\n", virtual_alloc[0].byte, virtual_alloc[0].numslots, dl_ie_size, phy->getSlotCapacity(virtual_alloc[0].mod,DL_), num_ie);
    debug10 ("\tNew DL MAP byte :%f, slots :%d\n", dl_map_increase_byte, dl_map_increase_slots);

    for (int i = 1; i<num_of_entries; i++) {
        debug2 ("\t index :%d, numslots :%d, accum :%d\n", i, virtual_alloc[i].numslots, slots_sofar);
        slots_sofar = slots_sofar + virtual_alloc[i].numslots;
    }

    int slots_temp = slots_sofar;
    slots_sofar = slots_sofar + dl_map_increase_slots;
    if ( slots_sofar <= totalslots) {
        debug10 ("Check_DLMAP. Old DL map byte :%f, slots :%d, New DL map byte :%f, slots :%d, #ie :%d\n", virtual_alloc[0].byte, virtual_alloc[0].numslots, dl_map_increase_byte, dl_map_increase_slots, num_ie);

        virtual_alloc[0].byte = dl_map_increase_byte;
        virtual_alloc[0].numslots = dl_map_increase_slots;
        return slots_sofar;
    } else {
        return -1;
    }

}


// This function is used to check if the allocation is feasible (enough slots for both DL_MAP_IE and its own data allocation
int BSScheduler::overallocation_withdlmap(int num_of_entries, int totalslots, int ownslots)
{

    OFDMAPhy *phy = mac_->getPhy();
    int slots_sofar = 0;
    float dl_ie_size = DL_MAP_IE_SIZE;
//padding to byte based
    float dl_map_increase_byte = virtual_alloc[0].byte + dl_ie_size;
    int dl_map_increase_slots = virtual_alloc[0].rep*(int)ceil((ceil)(dl_map_increase_byte)/(double)phy->getSlotCapacity(virtual_alloc[0].mod,DL_));

    debug10 ("Check_DLMAP.From array DL MAP byte :%f, slots :%d, IE_SIZE :%f, slotcapacity :%d\n", virtual_alloc[0].byte, virtual_alloc[0].numslots, dl_ie_size, phy->getSlotCapacity(virtual_alloc[0].mod,DL_));
    debug10 ("\tNew DL MAP byte :%f, slots :%d\n", dl_map_increase_byte, dl_map_increase_slots);

    for (int i = 1; i<num_of_entries; i++) {
        debug2 ("\t index :%d, numslots :%d, accum :%d\n", i, virtual_alloc[i].numslots, slots_sofar);
        slots_sofar = slots_sofar + virtual_alloc[i].numslots;
    }

    int slots_temp = slots_sofar;
    slots_sofar = slots_sofar + ownslots + dl_map_increase_slots;
    if ( slots_sofar <= totalslots) {
        debug10 ("Check_DLMAP. Old DL map byte :%f, slots :%d, New DL map byte :%f, slots :%d, available burst slot size :%d\n", virtual_alloc[0].byte, virtual_alloc[0].numslots, dl_map_increase_byte, dl_map_increase_slots, ownslots);

        virtual_alloc[0].byte = dl_map_increase_byte;
        virtual_alloc[0].numslots = dl_map_increase_slots;
        return ownslots;
    } else {
        if ( (slots_temp + dl_map_increase_slots) <= totalslots ) {
            debug10 ("Check_DLMAP. Old DL map byte :%f, slots :%d, New DL map byte :%f, slots :%d, available slot burst size :%d\n", virtual_alloc[0].byte, virtual_alloc[0].numslots, dl_map_increase_byte, dl_map_increase_slots, totalslots - slots_temp - dl_map_increase_slots);
            virtual_alloc[0].byte = dl_map_increase_byte;
            virtual_alloc[0].numslots = dl_map_increase_slots;
            return (totalslots - slots_temp - dl_map_increase_slots);
        } else {
            return -1;
        }
    }
}


// This function is used to calculate maximum number of connection with the increase of DL_MAP_IE
int BSScheduler::max_conn_withdlmap(int num_of_entries, int totalslots)
{

    OFDMAPhy *phy = mac_->getPhy();
    int slots_sofar = 0;
    float dl_ie_size = DL_MAP_IE_SIZE;
    int max_conn = 0;
    float dl_map_increase_byte = 0;
    int dl_map_increase_slots = 0;
    float virtual_increase_byte = virtual_alloc[0].byte;
    int virtual_increase_slots = virtual_alloc[0].numslots;

    for (int i = 0; i<num_of_entries; i++) {
//	debug2 ("\t index :%d, numslots :%d, accum :%d\n", i, virtual_alloc[i].numslots, slots_sofar);
        slots_sofar = slots_sofar + virtual_alloc[i].numslots;
    }
    int slots_data = slots_sofar - virtual_alloc[0].numslots;

    debug10 ("Check_DLMAP. Max supported conn total_dl_slots :%d, sofar slots :%d, dlmap slots :%d, dlmap byte :%f, available slots :%d\n", totalslots, slots_sofar, virtual_alloc[0].numslots, virtual_alloc[0].byte, totalslots-slots_sofar);

    while (1) {
        virtual_increase_byte += dl_ie_size;
        int virtual_increase_byte_ceil = (int)ceil(virtual_increase_byte);
        virtual_increase_slots = virtual_alloc[0].rep*ceil((double)(virtual_increase_byte_ceil)/(double)phy->getSlotCapacity(virtual_alloc[0].mod,DL_));
        if ( (virtual_increase_slots + slots_data) >= totalslots ) {
            break;
        } else {
            max_conn++;
        }
    }

    return max_conn;
}


// This function is used to calculate number of available slots after #DL_MAP_IEs are taken into account.
int BSScheduler::freeslots_withdlmap_given_conn(int num_of_entries, int totalslots, int newconn)
{

    OFDMAPhy *phy = mac_->getPhy();
    int slots_sofar = 0;
    float dl_ie_size = DL_MAP_IE_SIZE;
    int max_conn = 0;
    float dl_map_increase_byte = 0;
    int dl_map_increase_slots = 0;
    float virtual_increase_byte = virtual_alloc[0].byte;
    int virtual_increase_slots = virtual_alloc[0].numslots;
    int total_slots_withdlmap_nodata = 0;

    for (int i = 0; i<num_of_entries; i++) {
//	debug2 ("\t index :%d, numslots :%d, accum :%d\n", i, virtual_alloc[i].numslots, slots_sofar);
        slots_sofar = slots_sofar + virtual_alloc[i].numslots;
    }
    int slots_data = slots_sofar - virtual_alloc[0].numslots;

    for (int i=0; i<newconn; i++) {
        virtual_increase_byte += dl_ie_size;
    }
    int virtual_increase_byte_ceil = (int)ceil(virtual_increase_byte);
    virtual_increase_slots = virtual_alloc[0].rep*ceil((double)(virtual_increase_byte_ceil)/(double)phy->getSlotCapacity(virtual_alloc[0].mod,DL_));

    debug10 ("Check_DLMAP. #Free slots newconn :%d, sofar slots :%d, dlmap slots :%d, dlmap byte :%f, newdlmap slots :%d, newdlmap byte :%d\n", newconn, slots_sofar, virtual_alloc[0].numslots, virtual_alloc[0].byte, virtual_increase_slots, virtual_increase_byte_ceil);

    return totalslots - slots_data - virtual_increase_slots;
}


// This function is used check #available slots excluing DL_MAP_IE
// return #request slots if feasible else return #available slots
int BSScheduler::overallocation_withoutdlmap(int num_of_entries, int totalslots, int ownslots)
{

    int slots_sofar = 0;

    for (int i = 0; i<num_of_entries; i++) {
        slots_sofar = slots_sofar + virtual_alloc[i].numslots;
    }

    int slots_temp = slots_sofar;
    slots_sofar = slots_sofar + ownslots;
    if ( slots_sofar <= totalslots) {
        return ownslots;
    } else {
        return (totalslots-slots_temp);
    }
}


// This function is a traditional bubble sort.
// Note that "sort_field" can be ascending/descending order
void BSScheduler::bubble_sort (int arrayLength, con_data_alloc array[], int sort_field)
{
    int i, j, flag = 1;
    con_data_alloc temp;

    for (int k = 0; k < arrayLength; k++) {
//        debug10 ("-Before i :%d, cid :%d, counter :%d, req_slots :%d, mod :%d\n", k, array[k].cid, array[k].counter, array[k].req_slots, (int)array[k].mod_rate);
    }

    if (sort_field == 0) {
        for (i = 1; (i <= arrayLength) && flag; i++) {
            flag = 0;
            for (j = 0; j < (arrayLength -1); j++) {
                if (array[j+1].req_slots < array[j].req_slots) {
                    temp.cid = array[j].cid;
                    temp.direction = array[j].direction;
                    temp.mod_rate = array[j].mod_rate;
                    temp.req_slots = array[j].req_slots;
                    temp.grant_slots = array[j].grant_slots;
                    temp.weight = array[j].weight;
                    temp.counter = array[j].counter;

                    array[j].cid = array[j+1].cid;
                    array[j].direction = array[j+1].direction;
                    array[j].mod_rate = array[j+1].mod_rate;
                    array[j].req_slots = array[j+1].req_slots;
                    array[j].grant_slots = array[j+1].grant_slots;
                    array[j].weight = array[j+1].weight;
                    array[j].counter = array[j+1].counter;

                    array[j+1].cid = temp.cid;
                    array[j+1].direction = temp.direction;
                    array[j+1].mod_rate = temp.mod_rate;
                    array[j+1].req_slots = temp.req_slots;
                    array[j+1].grant_slots = temp.grant_slots;
                    array[j+1].weight = temp.weight;
                    array[j+1].counter = temp.counter;
                    flag = 1;
                }//end if
            }//end 2nd for
        }//end 1st for
    }//end sort_field == 0

    if (sort_field == 1) {
        for (i = 1; (i <= arrayLength) && flag; i++) {
            flag = 0;
            for (j = 0; j < (arrayLength -1); j++) {
                if (array[j+1].counter > array[j].counter) {
                    temp.cid = array[j].cid;
                    temp.direction = array[j].direction;
                    temp.mod_rate = array[j].mod_rate;
                    temp.req_slots = array[j].req_slots;
                    temp.grant_slots = array[j].grant_slots;
                    temp.weight = array[j].weight;
                    temp.counter = array[j].counter;

                    array[j].cid = array[j+1].cid;
                    array[j].direction = array[j+1].direction;
                    array[j].mod_rate = array[j+1].mod_rate;
                    array[j].req_slots = array[j+1].req_slots;
                    array[j].grant_slots = array[j+1].grant_slots;
                    array[j].weight = array[j+1].weight;
                    array[j].counter = array[j+1].counter;

                    array[j+1].cid = temp.cid;
                    array[j+1].direction = temp.direction;
                    array[j+1].mod_rate = temp.mod_rate;
                    array[j+1].req_slots = temp.req_slots;
                    array[j+1].grant_slots = temp.grant_slots;
                    array[j+1].weight = temp.weight;
                    array[j+1].counter = temp.counter;
                    flag = 1;
                }//end if
            }//end 2nd for
        }//end 1st for
    }//end sort_field == 1

    for (int k = 0; k < arrayLength; k++) {
//        debug10 ("-After i :%d, cid :%d, counter :%d, req_slots :%d, mod :%d\n", k, array[k].cid, array[k].counter, array[k].req_slots, (int)array[k].mod_rate);
    }

    return;   //arrays are passed to functions by address; nothing is returned
}


// This function is used to check if the virutall allocation exists
int BSScheduler::doesvirtual_allocexist(int num_of_entries, int cid)
{
    for (int i = 0; i<num_of_entries-1; i++) {
        if (virtual_alloc[i].cid == cid) {
            return cid;
        }
    }
    return -1;
}


// This function is used to add the virtual burst regardless of DL_MAP_IE
int BSScheduler::addslots_withoutdlmap(int num_of_entries, int byte, int slots, int cid)
{
    for (int i = 0; i<num_of_entries-1; i++) {
        if (virtual_alloc[i].cid == cid) {
            virtual_alloc[i].byte += byte;
            virtual_alloc[i].numslots += slots;
            return i;
        }
    }
    return -1;
}


// This function is used caculate #slots used so far
int BSScheduler::check_overallocation(int num_of_entries)
{

    int slots_sofar = 0;
    for (int i = 0; i<num_of_entries; i++) {
        slots_sofar = slots_sofar + virtual_alloc[i].numslots;
    }
    return slots_sofar;
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

    Packet *p;
    struct hdr_cmn *ch;
    double txtime; //tx time for some data (in second)
    int txtime_s;  //number of symbols used to transmit the data
    DlBurst *db;
    PeerNode *peer;
    int Channel_num=0;

    // We will try to Fill in the ARQ Feedback Information now...
    Packet * ph = 0;
    PeerNode * peernode;
    Connection * basic ;
    Connection * OutData;
    Packet * pfb = 0;
    hdr_mac802_16 *wimaxHdrMap ;
    u_int16_t   temp_num_of_acks = 0;
    bool out_datacnx_exists = false;
    for (Connection *n= mac_->getCManager ()->get_in_connection (); n; n=n->next_entry()) {
        if (n->getArqStatus () != NULL && n->getArqStatus ()->isArqEnabled() == 1) {
            if (!(n->getArqStatus ()->arq_feedback_queue_) || (n->getArqStatus ()->arq_feedback_queue_->length() == 0)) {
                continue;
            } else {
                peernode = n->getPeerNode ();
                if (peernode->getOutData () != NULL && peernode->getOutData ()->queueLength () != 0 && getMac()->arqfb_in_dl_data_) {
                    out_datacnx_exists = true;
                }

                if (out_datacnx_exists == false) {
                    //debug2("ARQ BS: Feedback in Basic Cid \n");
                    basic = peernode->getBasic (OUT_CONNECTION);
                    pfb = mac_->getPacket ();
                    wimaxHdrMap= HDR_MAC802_16(pfb);
                    wimaxHdrMap->header.cid = basic->get_cid ();
                    wimaxHdrMap->num_of_acks = 0;
                } else {
                    debug2("ARQ BS : Feedback in data Cid \n");
                    OutData = peernode->getOutData ();
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
                } else
                    OutData->enqueue_head (pfb);
            }
        }
    }

    peer = mac_->getPeerNode_head();
    if (peer) {
        for (int i=0; i<mac_->getNbPeerNodes() ; i++) {
            //peer->setchannel(++Channel_num);
            peer->setchannel((peer->getchannel())+1);

            if (peer->getchannel()>999)
                peer->setchannel(2);

            peer = peer->next_entry();
        }
    }

    if (Channel_num>999)
        Channel_num = 2;//GetInitialChannel();

    /// rpi random channel allocation

    OFDMAPhy *phy = mac_->getPhy();
    FrameMap *map = mac_->getMap();
    int nbPS = (int) floor((mac_->getFrameDuration()/phy->getPS()));
#ifdef DEBUG_WIMAX
    assert (nbPS*phy->getPS() <= mac_->getFrameDuration()); //check for rounding errors
#endif

    int nbPS_left = nbPS - mac_->phymib_.rtg - mac_->phymib_.ttg;
    int nbSymbols = (int) floor((phy->getPS()*nbPS_left)/phy->getSymbolTime());  // max num of OFDM symbols available per frame.
    assert (nbSymbols*phy->getSymbolTime()+(mac_->phymib_.rtg + mac_->phymib_.ttg)*phy->getPS() <= mac_->getFrameDuration());
    int dlduration = DL_PREAMBLE;                             //number of symbols currently used for downlink
    int maxdlduration = (int) (nbSymbols / (1.0/dlratio_)); //number of symbols for downlink

/////////////////////
// In next version, proper use of DL:UL symbols will be calculated.
//  int nbSymbols_wo_preamble = nbSymbols - DL_PREAMBLE;
//  int nbSymbols_wo_preamble = nbSymbols;
//
//  int virtual_maxdld = (int) (nbSymbols_preamble / (1.0/dlratio_)); //number of symbols for downlink
//  int virtual_maxuld = nbSymbols_wo_preamble - virtual_maxdld;
//  int counter_maxdld = virtual_maxdld;
//
//  for (int i=counter_maxdld; i<nbSymbols_wo_preamble; i++) {
//     if ( ((virtual_maxdld % 2) == 0) && ((virtual_maxuld % 3) == 0) ) {
//	break;
//     } else {
//
//     }
//  }
/////////////////////

    int subchannel_offset = 0;
    int nbdlbursts = 0;
    int maxulduration = nbSymbols - maxdlduration;                //number of symbols for uplink
    int ulduration = 0;           		                //number of symbols currently used for uplink
    int nbulpdus = 0;
    int total_dl_subchannel_pusc = 30;
    int total_ul_subchannel_pusc = 35;
    int total_dl_slots_pusc = floor((double)(maxdlduration-DL_PREAMBLE)/2)*total_dl_subchannel_pusc;
    int total_ul_slots_pusc = floor((double)maxulduration/3)*total_ul_subchannel_pusc;
    int numie;
    int number_ul_ie = 0;
    int number_dl_ie = 0;

    debug10 ("\n---------------------------------------------------\n");
    //| | | |       => symbols
    //0 1 2 3       => dlduration
    debug10 ("Start BS Scheduling: TotalSymbols :%d, MAXDL :%d, MAXUL :%d, Preamble :%d, dlduration after preamble :%d\n", nbSymbols, maxdlduration, maxulduration, DL_PREAMBLE, dlduration);

#ifdef DEBUG_WIMAX
    assert ((nbSymbols*phy->getSymbolPS()+mac_->phymib_.rtg + mac_->phymib_.ttg)*phy->getPS() <= mac_->getFrameDuration());
#endif
#ifdef DEBUG_WIMAX
    assert (maxdlduration*phy->getSymbolTime()+mac_->phymib_.rtg*phy->getPS()+maxulduration*phy->getSymbolTime()+mac_->phymib_.ttg*phy->getPS() <= mac_->getFrameDuration());
#endif

    mac_->setMaxDlduration (maxdlduration);
    mac_->setMaxUlduration (maxulduration);

    //============================UL allocation=================================
    //1 and 2 - Clear Ul_MAP and Allocate inital ranging and bw-req regions
    int ul_subframe_subchannel_offset = 0;
    int total_UL_num_subchannel = phy->getNumsubchannels(UL_);

    mac_->getMap()->getUlSubframe()->setStarttime (maxdlduration*phy->getSymbolPS()+mac_->phymib_.rtg);

    while (mac_->getMap()->getUlSubframe()->getNbPdu()>0) {
        PhyPdu *pdu = mac_->getMap()->getUlSubframe()->getPhyPdu(0);
        pdu->removeAllBursts();
        mac_->getMap()->getUlSubframe()->removePhyPdu(pdu);
        delete (pdu);
    }

    //Set contention slots for cdma initial ranging
    UlBurst *ub = (UlBurst*)mac_->getMap()->getUlSubframe()->addPhyPdu (nbulpdus++,0)->addBurst (0);
    ub->setIUC (UIUC_INITIAL_RANGING);
    int int_rng_num_sub = (int)(init_contention_size_*CDMA_6SUB);		//init_contention_size is set to 5 in this version
    int int_rng_num_sym = 2;
    ub->setDuration (int_rng_num_sym);
    ub->setStarttime (ulduration);
    ub->setSubchannelOffset (0);
    ub->setnumSubchannels(int_rng_num_sub);
    ul_subframe_subchannel_offset = (0);
    debug10 ("UL.Initial Ranging, contention_size :%d, 1opportunity = 6sub*2symbols, ulduration :%d, initial_rng_duration :%d, updated ulduration :%d\n", init_contention_size_, ulduration, int_rng_num_sym, ulduration + int_rng_num_sym);

    ulduration += int_rng_num_sym;

    //Set contention slots for cdma bandwidth request
    ub = (UlBurst*)mac_->getMap()->getUlSubframe()->addPhyPdu (nbulpdus++,0)->addBurst (0);
    ub->setIUC (UIUC_REQ_REGION_FULL);
    int bw_rng_num_sub = (int)(bw_req_contention_size_*CDMA_6SUB);	//bw_req_contention_size is set to 5 in this version
    int bw_rng_num_sym = 1;
    ub->setDuration (bw_rng_num_sym);
    ub->setStarttime (ulduration);
    ub->setSubchannelOffset (0);
    ub->setnumSubchannels(bw_rng_num_sub);
    ul_subframe_subchannel_offset = (0);
    debug10 ("UL.Bw-req, contention_size :%d, 1opportunity = 6sub*1symbols, ulduration :%d, bw-req_duration :%d, updated ulduration :%d\n", bw_req_contention_size_, ulduration, bw_rng_num_sym, ulduration + bw_rng_num_sym);
    ulduration +=bw_rng_num_sym;


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
            Connection *n= mac_->getCManager ()->get_in_connection ();

            for (int i=0; i<MAX_SYNC_CQICH_ALLOC_INFO; i++) {
                //debug2("has_sent is %d   ext_uiuc %d \n", mac_->cqich_alloc_info_buffer[i].has_sent, mac_->cqich_alloc_info_buffer[i].ext_uiuc);
                if ((mac_->cqich_alloc_info_buffer[i].need_cqich_slot == 1)
                        &&  (sync_cqich_slot_allocated < MAX_SYNC_CQICH_SLOT)) { // has sent, so start allocating CQICH slot
                    debug2("generate a UL Burst for CQICH slot.\n");
                    ub = (UlBurst*) mac_->getMap()->getUlSubframe()->addPhyPdu (nbulpdus++,0)->addBurst (0);
                    ub->setIUC (UIUC_PROFILE_2);  // Enhanced CQICH profile.
                    ub->setDuration(1);
                    ub->setStarttime (ulduration);
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
            ulduration+=cqich_symbol_num;
        }
    }

    mac_->setStartUlduration (ulduration);

    peer = mac_->getPeerNode_head();

    if (ulduration > maxulduration ) {
        debug2 (" not enough UL symbols to allocate \n " );
    }

    // This cdma_flag is used in case there is no peer yet however the allocation for cdma_transmission oppportunity needed to be allocated by the BS-UL scheduler
    int cdma_flag = 0;
    Connection *con1;
    con1 = mac_->getCManager()->get_in_connection();
    while (con1!=NULL) {
        if (con1->get_category() == CONN_INIT_RANGING) {
            if (con1->getCDMA()>0) {
                cdma_flag = 2;
            }
        }
        con1 = con1->next_entry();
    }

    //Call ul_stage2 to allocate the uplink resource
    if ( (peer)  || (cdma_flag>0) ) {
        if (maxulduration > ulduration) {
            debug2 ("UL.Before going to ul_stage2 (data allocation) Frame duration :%f, PSduration :%e, Symboltime :%e, nbPS :%d, rtg :%d, ttg :%d, nbPSleft :%d, nbSymbols :%d\n", mac_->getFrameDuration(), phy->getPS(), phy->getSymbolTime(), nbPS, mac_->phymib_.rtg, mac_->phymib_.ttg, nbPS_left, nbSymbols);
            debug2 ("\tmaxdlduration :%d, maxulduration :%d, numsubchannels :%d, ulduration :%d\n", maxdlduration, (maxulduration), phy->getNumsubchannels(/*phy->getPermutationscheme (),*/ UL_), ulduration);

            /*UL_stage2()*/
            mac802_16_ul_map_frame * ulmap = ul_stage2 (mac_->getCManager()->get_in_connection (),phy->getNumsubchannels( UL_), (maxulduration-ulduration), ulduration, VERTICAL_STRIPPING );


            // From UL_MAP_IE, we map the allocation into UL_BURST
            debug2("\nafter ul_stage2().ie_num is %d \n\n", ulmap->nb_ies);
            number_ul_ie = (int)ulmap->nb_ies;
            for (numie = 0 ; numie < (int) ulmap->nb_ies ; numie++) {
                mac802_16_ulmap_ie ulmap_ie = ulmap->ies[numie];
                ub = (UlBurst*) mac_->getMap()->getUlSubframe()->addPhyPdu (nbulpdus++,0)->addBurst (0);


                debug2 ("In side ulmap adding to burst,  UL_MAP_IE.UIUC = %d, num_UL_MAP_IE = %d\n", ulmap_ie.uiuc, ulmap->nb_ies);

                ub->setCid (ulmap_ie.cid);
                ub->setIUC (ulmap_ie.uiuc);
                ub->setStarttime (ulmap_ie.symbol_offset);
                ub->setDuration (ulmap_ie.num_of_symbols);
                ub->setSubchannelOffset (ulmap_ie.subchannel_offset);
                ub->setnumSubchannels (ulmap_ie.num_of_subchannels);
                ub->setB_CDMA_TOP (ulmap_ie.cdma_ie.subchannel);
                ub->setB_CDMA_CODE (ulmap_ie.cdma_ie.code);


                /*By using the UL-MAP IE to bring the CQICH Allocation IE information down to the corresponding SS.*/
                debug2("mac_ get cqich alloc ie ind %d\n", mac_->get_cqich_alloc_ie_ind());
                //printf("amc_enable flag is %d\n", amc_enable_);
                /*Now we add the CQICH allocation IE to the corresponding UL-MAP IE.*/
                if (getMac()->get_cqich_alloc_ie_ind() == true && mac_->amc_enable_ ==1) {
                    debug2("1.1 BSscheduler is gonna sent CQICH Allocation IE via UL-MAP.\n");
                    for (int i=0; i<=(int) ulmap->nb_ies; ++i) {
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
                debug2("   Addburst cdma code :%d, cdma top :%d\n", ub->getB_CDMA_CODE(), ub->getB_CDMA_TOP());
            }
            free (ulmap);
        }
    }

    //End of map
    //Note that in OFDMA, there is no end of UL map, it'll be removed in next version; however, this is a virtual end of map say there is no transmitted packet/message
    ub = (UlBurst*)mac_->getMap()->getUlSubframe()->addPhyPdu (nbulpdus,0)->addBurst (0);
    ub->setIUC (UIUC_END_OF_MAP);
    ub->setStarttime (maxulduration);
    ub->setSubchannelOffset (0); //Richard: changed 1->0
    ub->setnumSubchannels (phy->getNumsubchannels(UL_));
    debug10 ("UL.EndofMAP: (addBurst_%d) maxuluration :%d, lastbursts :%d, nbulpdus :%d\n", nbdlbursts, maxulduration, nbdlbursts, nbulpdus);





    //============================DL allocation=================================
    debug10 ("DL.Scheduler: FrameDuration :%5.4f, PSduration :%e, symboltime :%e, nbPS :%d, rtg :%d, ttg :%d, nbPSleft :%d, nbSymbols :%d, dlratio_ :%5.2f\n", mac_->getFrameDuration(), phy->getPS(), phy->getSymbolTime(), nbPS, mac_->phymib_.rtg, mac_->phymib_.ttg, nbPS_left, nbSymbols, dlratio_);

    map->getDlSubframe()->getPdu()->removeAllBursts();
    bzero(virtual_alloc, MAX_MAP_IE*sizeof(virtual_burst));
    index_burst = 0;

//1. Virtual DL_MAP
//Note that, we virtually allocate all burst into "virtual_alloc[]", then we will physically allocate/map those allocation into burst/physical later. The reason is because we have variable part of DL_MAPs
//"index_burst" is the #total_burst including each DL_MAP, UL_MAP, DCD, UCD, other broadcast message and each CID allocation
//In this version, the allocation is per CID/burst for downlink
    int fixed_byte_dl_map = DL_MAP_HEADER_SIZE;
    int rep_fixed_dl = Repetition_code_;
    Ofdm_mod_rate fixed_dl_map_mod = map->getDlSubframe()->getProfile (map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC())->getEncoding();
    int fixed_dl_map_slots = (int) rep_fixed_dl*ceil((double)fixed_byte_dl_map/(double)phy->getSlotCapacity(fixed_dl_map_mod, DL_));
    virtual_alloc[index_burst].alloc_type = 0;
    virtual_alloc[index_burst].cid = BROADCAST_CID;
    virtual_alloc[index_burst].n_cid = 0;
    virtual_alloc[index_burst].iuc = map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC();
    virtual_alloc[index_burst].preamble = true;
    virtual_alloc[index_burst].numslots = fixed_dl_map_slots;
    virtual_alloc[index_burst].mod = fixed_dl_map_mod;
    virtual_alloc[index_burst].byte = fixed_byte_dl_map;
    virtual_alloc[index_burst].rep =  Repetition_code_;
    virtual_alloc[index_burst].dl_ul = 0;
    virtual_alloc[index_burst].ie_type = 0;
    index_burst++;

//2. Virtual UL_MAP:
    int fixed_byte_ul_map = UL_MAP_HEADER_SIZE + UL_MAP_IE_SIZE*number_ul_ie;
    int rep_fixed_ul = Repetition_code_;
    Ofdm_mod_rate fixed_ul_map_mod = map->getDlSubframe()->getProfile (map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC())->getEncoding();
    int fixed_ul_map_slots = (int) rep_fixed_ul*ceil((double)fixed_byte_ul_map/(double)phy->getSlotCapacity(fixed_ul_map_mod, DL_));
    virtual_alloc[index_burst].alloc_type = 1;
    virtual_alloc[index_burst].cid = BROADCAST_CID;
    virtual_alloc[index_burst].n_cid = 0;
    virtual_alloc[index_burst].iuc = map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC();
    virtual_alloc[index_burst].preamble = true;
    virtual_alloc[index_burst].numslots = fixed_ul_map_slots;
    virtual_alloc[index_burst].mod = fixed_ul_map_mod;
    virtual_alloc[index_burst].byte = fixed_byte_ul_map;
    virtual_alloc[index_burst].rep =  Repetition_code_;
    virtual_alloc[index_burst].dl_ul = 0;
    virtual_alloc[index_burst].ie_type = 0;
    int add_ie_to_dlmap = increase_dl_map_ie(index_burst, total_dl_slots_pusc, 1);
    if (add_ie_to_dlmap <0 ) {
        debug10 ("Panic: not enough space for UL_MAP\n");
        exit(1);
    }
    index_burst++;

//3. Virtual DCD
    if (getMac()->sendDCD || map->getDlSubframe()->getCCC()!= getMac()->dlccc_) {
        p = map->getDCD();
        ch = HDR_CMN(p);
        int byte_dcd = ch->size();
        int rep_dcd = 1;
        Ofdm_mod_rate dcd_mod = map->getDlSubframe()->getProfile (map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC())->getEncoding();
        int dcd_slots = (int) rep_dcd*ceil((double)byte_dcd/(double)phy->getSlotCapacity(dcd_mod, DL_));
        virtual_alloc[index_burst].alloc_type = 2;
        virtual_alloc[index_burst].cid = BROADCAST_CID;
        virtual_alloc[index_burst].n_cid = 1;
        virtual_alloc[index_burst].iuc = map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC();
        virtual_alloc[index_burst].preamble = false;
        virtual_alloc[index_burst].numslots = dcd_slots;
        virtual_alloc[index_burst].mod = dcd_mod;
        virtual_alloc[index_burst].byte = byte_dcd;
        virtual_alloc[index_burst].rep =  rep_dcd;
        virtual_alloc[index_burst].dl_ul = 0;
        virtual_alloc[index_burst].ie_type = 0;
        int add_ie_to_dlmap = increase_dl_map_ie(index_burst, total_dl_slots_pusc, 1);
        if (add_ie_to_dlmap <0 ) {
            debug10 ("Panic: not enough space for DCD\n");
            exit(1);
        } else {
            debug10 ("DCD size :%d bytes, %d slots\n", ch->size(), dcd_slots);
        }
        index_burst++;
    }

//4. Virtual UCD
    if (getMac()->sendUCD || map->getUlSubframe()->getCCC()!= getMac()->ulccc_) {
        p = map->getUCD();
        ch = HDR_CMN(p);
        int byte_ucd = ch->size();
        int rep_ucd = 1;
        Ofdm_mod_rate ucd_mod = map->getDlSubframe()->getProfile (map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC())->getEncoding();
        int ucd_slots = (int) rep_ucd*ceil((double)byte_ucd/(double)phy->getSlotCapacity(ucd_mod, DL_));
        virtual_alloc[index_burst].alloc_type = 3;
        virtual_alloc[index_burst].cid = BROADCAST_CID;
        virtual_alloc[index_burst].n_cid = 1;
        virtual_alloc[index_burst].iuc = map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC();
        virtual_alloc[index_burst].preamble = false;
        virtual_alloc[index_burst].numslots = ucd_slots;
        virtual_alloc[index_burst].mod = ucd_mod;
        virtual_alloc[index_burst].byte = byte_ucd;
        virtual_alloc[index_burst].rep =  rep_ucd;
        virtual_alloc[index_burst].dl_ul = 0;
        virtual_alloc[index_burst].ie_type = 0;
        int add_ie_to_dlmap = increase_dl_map_ie(index_burst, total_dl_slots_pusc, 1);
        if (add_ie_to_dlmap <0 ) {
            debug10 ("Panic: not enough space for UCD\n");
            exit(1);
        } else {
            debug10 ("UCD size :%d bytes, %d slots\n", ch->size(), ucd_slots);
        }
        index_burst++;
    }

    int check_dl_slots = check_overallocation(index_burst);

    debug10 ("DL.broadcast messages (slots): (v)Fixed_DL_MAP :%d, (v)UL_MAP :%d, DCD :%d, UCD :%d, MaxDL :%d\n", virtual_alloc[0].numslots, virtual_alloc[1].numslots, virtual_alloc[2].numslots, virtual_alloc[3].numslots, total_dl_slots_pusc);
    debug10 ("\t(bytes): (v)Fixed_DL_MAP :%f, (v)UL_MAP :%f, DCD :%f, UCD :%f\n", virtual_alloc[0].byte, virtual_alloc[1].byte, virtual_alloc[2].byte, virtual_alloc[3].byte);

    if (check_dl_slots > total_dl_slots_pusc) {
        debug10("Panic : not enough dl_slots\n");
        exit(1);
    }

//5. Virtual Other broadcast
    if (mac_->getCManager()->get_connection (BROADCAST_CID, OUT_CONNECTION)->queueByteLength()>0) {
        Ofdm_mod_rate bc_mod = map->getDlSubframe()->getProfile (map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC())->getEncoding();

        int slot_br = 0;
        Connection *c_tmp;
        c_tmp = mac_->getCManager()->get_connection (BROADCAST_CID, OUT_CONNECTION);
        int real_bytes = 0;
        int i_packet = 0;
        Packet *np;
        debug10 ("Retrive connection :%d, qlen :%d\n", c_tmp->get_cid(), c_tmp->queueLength());
        for (int j_p = 0; j_p<c_tmp->queueLength(); j_p++) {
            if ( (np = c_tmp->queueLookup(i_packet)) != NULL ) {
                int p_size = hdr_cmn::access(np)->size();
                debug10 ("\t Other Broadcast CON CID :%d, packet-id :%d, q->byte :%d, q->len :%d, packet_size :%d, frag_no :%d, frag_byte :%d, frag_stat :%d, real_bytes :%d\n", c_tmp->get_cid(), i_packet, c_tmp->queueByteLength(), c_tmp->queueLength(), p_size, c_tmp->getFragmentNumber(), c_tmp->getFragmentBytes(), (int)c_tmp->getFragmentationStatus(), real_bytes );
                i_packet++;
                int num_of_slots = (int) ceil((double)p_size/(double)phy->getSlotCapacity(bc_mod,DL_));
                real_bytes = real_bytes + (int) ceil((double)num_of_slots*(double)(phy->getSlotCapacity(bc_mod,UL_)));
            }
        }

        int bc_slots = (int) ceil((double)real_bytes/(double)phy->getSlotCapacity(bc_mod,DL_));
        virtual_alloc[index_burst].alloc_type = 4;
        virtual_alloc[index_burst].cid = BROADCAST_CID;
        virtual_alloc[index_burst].n_cid = 1;
        virtual_alloc[index_burst].iuc = map->getDlSubframe()->getProfile (DIUC_PROFILE_2)->getIUC();
        virtual_alloc[index_burst].preamble = false;
        virtual_alloc[index_burst].numslots = bc_slots;
        virtual_alloc[index_burst].byte = real_bytes;
        virtual_alloc[index_burst].rep =  1;
        virtual_alloc[index_burst].dl_ul = 0;
        virtual_alloc[index_burst].ie_type = 0;
        int add_ie_to_dlmap = increase_dl_map_ie(index_burst, total_dl_slots_pusc, 1);
        if (add_ie_to_dlmap <0 ) {
            debug10 ("Panic: not enough space for other Broadcast message\n");
            exit(1);
        }

        debug10 ("DL.other_broadcast messages (slots): %d, (bytes): %f\n", virtual_alloc[index_burst].numslots, virtual_alloc[index_burst].byte);
        index_burst++;
    }
//End of Virtual allocation for all broadcast message

    int index_burst_before_data = index_burst;

    int check_dl_duration = ceil((double)check_overallocation(index_burst)/(double)total_dl_subchannel_pusc);
    dlduration = check_dl_duration;
    int num_of_slots = 0;
    int num_of_subchannel = 0;
    int num_of_symbol = 0;
    int total_DL_num_subchannel = phy->getNumsubchannels(DL_);

    // from here start ofdma DL -- call the scheduler API- here DLMAP    ------------------------------------
    peer = mac_->getPeerNode_head();

    subchannel_offset = 0;
    mac802_16_dl_map_frame *dl_map;

    // Call dl_stage2 to allocate the downlink resource
    if (peer) {
        if (maxdlduration > dlduration) {
            debug2 ("DL.Before going to dl-stage2 (data allocation), Frame Duration :%5.4f, PSduration :%e, symboltime :%e, nbPS :%d, rtg :%d, ttg :%d, PSleft :%d, nbSymbols :%d\n", mac_->getFrameDuration(), phy->getPS(), phy->getSymbolTime(), nbPS, mac_->phymib_.rtg, mac_->phymib_.ttg, nbPS_left, nbSymbols);
            debug10 ("\tStartdld :%d, Enddld :%d, Dldsize :%d, Uldsize :%d, Numsubchannels (35 or 30) :%d \n", dlduration, maxdlduration, (maxdlduration-dlduration), maxulduration, phy->getNumsubchannels(/*phy->getPermutationscheme (), */DL_));

            dl_map =  dl_stage2(mac_->getCManager()->get_out_connection (), subchannel_offset, phy->getNumsubchannels(DL_),(maxdlduration-dlduration), dlduration, VERTICAL_STRIPPING, total_dl_slots_pusc);
            number_dl_ie = (int) dl_map->nb_ies;

        }
    }

    // Add all virtual allocation (brodcast and data portion) to physical DL burst
    // Note that #subchannels = #slots in this version
    int virtual_symbol_offset = DL_PREAMBLE;
    int virtual_subchannel_offset = 0;
    int virtual_accum_slots = 0;
    for (int i=0; i<index_burst; i++) {
        db = (DlBurst*) map->getDlSubframe()->getPdu ()->addBurst (nbdlbursts++);
        db->setCid (virtual_alloc[i].cid);
        db->setIUC (virtual_alloc[i].iuc);
        db->setPreamble(virtual_alloc[i].preamble);
        db->setStarttime(virtual_symbol_offset);
        db->setSubchannelOffset(virtual_subchannel_offset);

        int num_of_subchannel = virtual_alloc[i].numslots;
        int num_of_symbol = (int) ceil((double)(virtual_subchannel_offset + num_of_subchannel)/total_dl_subchannel_pusc)*2;

        db->setnumSubchannels(num_of_subchannel);
        db->setDuration(num_of_symbol);

        virtual_accum_slots += virtual_alloc[i].numslots;

        debug10("DL.add bursts: index :%d, cid :%d, iuc :%d, subchannel_offset :%d, symbol_offset :%d, num_slots (num_subchannel) :%d, num_symbol :%d, accum_slots :%d\n", i, virtual_alloc[i].cid, virtual_alloc[i].iuc, virtual_subchannel_offset, virtual_symbol_offset, virtual_alloc[i].numslots, num_of_symbol, virtual_accum_slots);

        virtual_subchannel_offset = (virtual_subchannel_offset + num_of_subchannel)%(total_dl_subchannel_pusc);
        virtual_symbol_offset += num_of_symbol - 2;
    }

#ifdef DEBUG_WIMAX
    assert (dlduration <= maxdlduration);
#endif

    // Now transfert the packets to the physical bursts starting with broadcast messages
    // In this version, we can directly map burst information from virtual burst into physical burst.
    // 2D rectangular allocation will be considered in next version
    int b_data = 0;
    int max_data = 0;
    hdr_mac802_16 *wimaxHdr;
    double txtime2 = 0;
    int offset = 0;
    int subchannel_offset_wimaxhdr = 0;
    int num_burst_before_data = 0;
    Burst *b;
    int i_burst = 0;

    b = map->getDlSubframe()->getPdu ()->getBurst (i_burst);
    i_burst++;
    b_data  = 0;
    txtime2 = 0;
    offset  = 0;
    subchannel_offset_wimaxhdr = 0;
    max_data = phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding())-b_data;

    p = map->getDL_MAP();
    num_burst_before_data++;
    ch = HDR_CMN(p);
    offset = b->getStarttime( );
#ifdef SAM_DEBUG
    debug2(" offset of DLMAP = %d \n", offset);
#endif
    debug10 ("DL/UL.start transfer packets into burst => symbol offset of DLMAP_start at :%d, subchannel offset at :%d\n", offset, subchannel_offset_wimaxhdr);

    txtime = phy->getTrxTime (ch->size(), map->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
    txtime2 = phy->getTrxSymbolTime (ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
    ch->txtime() = txtime2;
    txtime_s = (int)round (txtime2/phy->getSymbolTime ()); //in units of symbol
    ulduration = 0;

    Ofdm_mod_rate dlul_map_mod = map->getDlSubframe()->getProfile (b->getIUC())->getEncoding();
    num_of_slots = (int) ceil(ch->size()/phy->getSlotCapacity(dlul_map_mod,DL_)); //get the slots	needs.

    num_of_subchannel = num_of_slots;
    num_of_symbol = (int) ceil((double)(subchannel_offset_wimaxhdr + num_of_subchannel)/total_dl_subchannel_pusc)*2;

#ifdef DEBUG_WIMAX
    assert (b_data+ch->size() <= max_data);
#endif
    wimaxHdr = HDR_MAC802_16(p);
    if (wimaxHdr) {
        wimaxHdr->phy_info.num_subchannels = b->getnumSubchannels ();
        wimaxHdr->phy_info.subchannel_offset = b->getSubchannelOffset ();
        wimaxHdr->phy_info.num_OFDMSymbol = b->getDuration();
        wimaxHdr->phy_info.OFDMSymbol_offset = b->getStarttime();
        wimaxHdr->phy_info.channel_index = 1; //broadcast packet
        wimaxHdr->phy_info.direction = 0;
    }
    ch->timestamp() = NOW; //add timestamp since it bypasses the queue
    b->enqueue(p);      //enqueue into burst
    b_data += num_of_slots*phy->getSlotCapacity(dlul_map_mod,DL_);


    subchannel_offset_wimaxhdr = (subchannel_offset_wimaxhdr + num_of_subchannel)%(total_dl_subchannel_pusc);

    debug2("DL/UL.wimaxHdr:DL-MAP --- Mod[%d]\t size[%d]\t symbol offset[%d]\t symbol num[%d]\t subchannel offset[%d]\t subchannel num[%d]\n", dlul_map_mod, ch->size(), wimaxHdr->phy_info.OFDMSymbol_offset, wimaxHdr->phy_info.num_OFDMSymbol, wimaxHdr->phy_info.OFDMSymbol_offset, wimaxHdr->phy_info.num_subchannels);

    offset += num_of_symbol-2;

#ifdef SAM_DEBUG
    debug2(" transferring management messages into the burst");
#endif

    b = map->getDlSubframe()->getPdu ()->getBurst (i_burst);
    i_burst++;
    b_data  = 0;
    txtime2 = 0;
    offset = b->getStarttime( );
    subchannel_offset_wimaxhdr = 0;
    max_data = phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding())-b_data;


    debug2("After the UL burst set, now ready to enter getUL_MAP().\n");
    p = map->getUL_MAP();
    num_burst_before_data++;
    ch = HDR_CMN(p);
    txtime = phy->getTrxTime (ch->size(), map->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
    txtime2 = phy->getTrxSymbolTime (ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
    txtime_s = (int)round (txtime2/phy->getSymbolTime ()); //in units of symbol
    ch->txtime() = txtime2;
#ifdef DEBUG_WIMAX
    assert (b_data+ch->size() <= max_data);
#endif

    dlul_map_mod = map->getDlSubframe()->getProfile (b->getIUC())->getEncoding();
    num_of_slots = (int) ceil(ch->size()/phy->getSlotCapacity(dlul_map_mod,DL_)); //get the slots	needs.
    num_of_subchannel = num_of_slots;
    num_of_symbol = (int) ceil((double)(subchannel_offset_wimaxhdr + num_of_subchannel)/total_dl_subchannel_pusc)*2;

    wimaxHdr = HDR_MAC802_16(p);
    if (wimaxHdr) {
        wimaxHdr->phy_info.subchannel_offset = b->getSubchannelOffset();
        wimaxHdr->phy_info.OFDMSymbol_offset = b->getStarttime();
        wimaxHdr->phy_info.num_subchannels = b->getnumSubchannels();
        wimaxHdr->phy_info.num_OFDMSymbol = b->getDuration();


        debug10 ("DL/UL.wimaxHdr:UL-MAP --- Mod[%d]\t size[%d]\t symbol offset[%d]\t symbol num[%d]\t subchannel offset[%d]\t subchannel num[%d]\n", dlul_map_mod, ch->size(), wimaxHdr->phy_info.OFDMSymbol_offset, wimaxHdr->phy_info.num_OFDMSymbol, wimaxHdr->phy_info.OFDMSymbol_offset, wimaxHdr->phy_info.num_subchannels);

        wimaxHdr->phy_info.channel_index = 1; //broadcast packet
        if (mac_->getNodeType()==STA_BS)
            wimaxHdr->phy_info.direction = 0;
        else
            wimaxHdr->phy_info.direction = 1;
    }
    ch->timestamp() = NOW; //add timestamp since it bypasses the queue
    b->enqueue(p);      //enqueue into burst
    b_data += num_of_slots*phy->getSlotCapacity(dlul_map_mod,DL_);
    offset += num_of_symbol-2;
    subchannel_offset_wimaxhdr = (subchannel_offset_wimaxhdr + num_of_subchannel)%(total_dl_subchannel_pusc);

    if (getMac()->sendDCD || map->getDlSubframe()->getCCC()!= getMac()->dlccc_) {
        b = map->getDlSubframe()->getPdu ()->getBurst (i_burst);
        i_burst++;
        b_data  = 0;
        txtime2 = 0;
        offset = b->getStarttime( );
        subchannel_offset_wimaxhdr = 0;
        max_data = phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding())-b_data;

        p = map->getDCD();
        num_burst_before_data++;
        ch = HDR_CMN(p);
        txtime = phy->getTrxTime (ch->size(), map->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
        txtime2 = phy->getTrxSymbolTime (ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
        ch->txtime() = txtime2;
        txtime_s = (int)round (txtime2/phy->getSymbolTime ()); //in units of symbol
#ifdef DEBUG_WIMAX
        assert (b_data+ch->size() <= max_data);
#endif
        dlul_map_mod = map->getDlSubframe()->getProfile (b->getIUC())->getEncoding();
        num_of_slots = (int) ceil(ch->size()/phy->getSlotCapacity(dlul_map_mod,DL_)); //get the slots	needs.
        num_of_subchannel = num_of_slots;
        num_of_symbol = (int) ceil((double)(subchannel_offset_wimaxhdr + num_of_subchannel)/total_DL_num_subchannel)*2;
        wimaxHdr = HDR_MAC802_16(p);
        if (wimaxHdr) {
            wimaxHdr->phy_info.subchannel_offset = b->getSubchannelOffset();
            wimaxHdr->phy_info.OFDMSymbol_offset = b->getStarttime();
            wimaxHdr->phy_info.num_subchannels = b->getnumSubchannels();
            wimaxHdr->phy_info.num_OFDMSymbol = b->getDuration();
            wimaxHdr->phy_info.channel_index = 1;

            debug10 ("DL/UL.wimaxHdr:DCD --- Mod[%d]\t size[%d]\t symbol offset[%d]\t symbol num[%d]\t subchannel offset[%d]\t subchannel num[%d]\n", dlul_map_mod, ch->size(), wimaxHdr->phy_info.OFDMSymbol_offset, wimaxHdr->phy_info.num_OFDMSymbol, wimaxHdr->phy_info.OFDMSymbol_offset, wimaxHdr->phy_info.num_subchannels);

            if (mac_->getNodeType()==STA_BS)
                wimaxHdr->phy_info.direction = 0;
            else
                wimaxHdr->phy_info.direction = 1;
        }
        ch->timestamp() = NOW; //add timestamp since it bypasses the queue
        b->enqueue(p);      //enqueue into burst
        b_data += num_of_slots*phy->getSlotCapacity(dlul_map_mod,DL_);
        offset += num_of_symbol-2;
        subchannel_offset_wimaxhdr = (subchannel_offset_wimaxhdr + num_of_subchannel)%(total_DL_num_subchannel);
    }


    if (getMac()->sendUCD || map->getUlSubframe()->getCCC()!= getMac()->ulccc_) {
        b = map->getDlSubframe()->getPdu ()->getBurst (i_burst);
        i_burst++;
        b_data  = 0;
        txtime2 = 0;
        offset = b->getStarttime( );
        subchannel_offset_wimaxhdr = 0;
        max_data = phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding())-b_data;

        p = map->getUCD();
        num_burst_before_data++;
        ch = HDR_CMN(p);
        txtime = phy->getTrxTime (ch->size(), map->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
        txtime2 = phy->getTrxSymbolTime (ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
        ch->txtime() = txtime2;
        txtime_s = (int)round (txtime2/phy->getSymbolTime ()); //in units of symbol
        //ch->txtime() = txtime;
        ch->txtime() = txtime2;
#ifdef DEBUG_WIMAX
        assert (b_data+ch->size() <= max_data);
#endif

        dlul_map_mod = map->getDlSubframe()->getProfile (b->getIUC())->getEncoding();
        num_of_slots = (int) ceil(ch->size()/phy->getSlotCapacity(dlul_map_mod,DL_)); //get the slots	needs.
        num_of_subchannel = num_of_slots;
        num_of_symbol = (int) ceil((double)(subchannel_offset_wimaxhdr + num_of_subchannel)/total_DL_num_subchannel)*2;

        wimaxHdr = HDR_MAC802_16(p);
        if (wimaxHdr) {
            wimaxHdr->phy_info.subchannel_offset = b->getSubchannelOffset();
            wimaxHdr->phy_info.OFDMSymbol_offset = b->getStarttime();
            wimaxHdr->phy_info.num_subchannels = b->getnumSubchannels();
            wimaxHdr->phy_info.num_OFDMSymbol = b->getDuration();
            wimaxHdr->phy_info.channel_index = 1;

            debug10 ("DL/UL.wimaxHdr:UCD --- Mod[%d]\t size[%d]\t symbol offset[%d]\t symbol num[%d]\t subchannel offset[%d]\t subchannel num[%d]\n", dlul_map_mod, ch->size(), wimaxHdr->phy_info.OFDMSymbol_offset, wimaxHdr->phy_info.num_OFDMSymbol, wimaxHdr->phy_info.OFDMSymbol_offset, wimaxHdr->phy_info.num_subchannels);

            if (mac_->getNodeType()==STA_BS)
                wimaxHdr->phy_info.direction = 0;
            else
                wimaxHdr->phy_info.direction = 1;
        }
        ch->timestamp() = NOW; //add timestamp since it bypasses the queue
        b->enqueue(p);      //enqueue into burst
        b_data += num_of_slots*phy->getSlotCapacity(dlul_map_mod,DL_);

        offset += num_of_symbol-2;
        subchannel_offset_wimaxhdr = (subchannel_offset_wimaxhdr + num_of_subchannel)%(total_DL_num_subchannel);

    }

    debug2("i_burst number is %d \t num_burst_before_data is %d  totall number of burst is %d\n",
           i_burst, num_burst_before_data,  map->getDlSubframe()->getPdu ()->getNbBurst());

    //Now get the other bursts
    debug2("BS scheduler is going to handle other bursts (not DL/UL_MAP, DCD/UCD), #bursts = %d\n", map->getDlSubframe()->getPdu ()->getNbBurst());
    for (int index = num_burst_before_data ; index < map->getDlSubframe()->getPdu ()->getNbBurst(); index++) {
        Burst *b = map->getDlSubframe()->getPdu ()->getBurst (index);
        int b_data = 0;

        Connection *c=mac_->getCManager ()->get_connection (b->getCid(),OUT_CONNECTION);
        debug2("DL/UL.other CID [%d]\n", b->getCid());
#ifdef DEBUG_WIMAX
        assert (c);
#endif

        //Begin RPI
        if (c!=NULL) {
            debug10 ("DL/UL.check CID :%d, flag: FRAG :%d, PACK :%d, ARQ: %p\n", b->getCid(), c->isFragEnable(), c->isPackingEnable(), c->getArqStatus ());
            debug10 ("DL/UL.before transfer_packets1 (other data) to burst_i :%d, CID :%d, b_data (bytes_counter) :%d\n", index, b->getCid(), b_data);

            if (c->isFragEnable() && c->isPackingEnable() &&  (c->getArqStatus () != NULL) && (c->getArqStatus ()->isArqEnabled() == 1)) {
                debug2("DL/UL.BSSscheduler is goting to transfer packet with fragackarq.\n");
                b_data = transfer_packets_with_fragpackarq (c, b, b_data); /*RPI*/
            } else {
                b_data = transfer_packets1(c, b, b_data);
            }
        }
        debug10 ("\nDL/UL.after transfer_packets1 (other data) to burst_i :%d, CID :%d, b_data (update_counter) :%d\n", index, b->getCid(), b_data);
        //debug10 ("Dl/UL.the length of the queue of burst is [%d]\n", b->getQueueLength_packets());
    }//end loop ===> transfer bursts

    //Print the map
    debug2 ("\n==================BS %d Subframe============================\n", mac_->addr());
    mac_->getMap()->print_frame();
    debug2 ("===========================================================\n");

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
/*       // rpi removed this dl burst ----------------------------------------------------------------------------
	 int BSScheduler::addDlBurst (int burstid, Connection *c, int iuc, int dlduration, int maxdlduration)
	 {
	 double txtime; //tx time for some data (in second)
	 int txtime_s;  //number of symbols used to transmit the data
	 OFDMPhy *phy = mac_->getPhy();

	 //add a burst for this node
	 DlBurst *db = (DlBurst*) mac_->getMap()->getDlSubframe()->getPdu ()->addBurst (burstid);
	 db->setCid (c->get_cid());
	 db->setIUC (iuc);
	 db->setStarttime (dlduration);

	 txtime = phy->getTrxSymbolTime (c->queueByteLength(), mac_->getMap()->getDlSubframe()->getProfile (db->getIUC())->getEncoding());
	 txtime += c->queueLength() * TX_GAP; //add small gaps between packets to send
	 txtime_s = (int) ceil(txtime/phy->getSymbolTime ()); //in units of symbol
	 if (txtime_s < maxdlduration-dlduration) {
	 db->setDuration (txtime_s);
	 dlduration += db->getDuration ()+1; //add 1 OFDM symbol between bursts
	 } else {
	 //fill up the rest
	 db->setDuration (maxdlduration-dlduration);
	 dlduration = maxdlduration;
	 }
	 return dlduration;
	 }

*/ // rpi removed this dl burst ----------------------------------------------------------------------------

// rpi added adddlburst for ofdma ------------------------------------------------------------------

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

int BSScheduler::doesMapExist(int cid, int *cid_list, int num_of_entries)
{
    for (int i = 0; i<num_of_entries; ++i) {
        if (cid_list[i]==cid)
            return i;
    }
    return -1;
}

//dl_stage2 (Virtual Allocation) for data allocation (all CIDs)
mac802_16_dl_map_frame * BSScheduler::dl_stage2(Connection *head, int input_subchannel_offset, int total_subchannels, int total_symbols, int symbol_start, int stripping, int total_dl_slots_pusc)
{
    struct mac802_16_dl_map_frame *dl_map;
    struct mac802_16_dlmap_ie *ies;
    Connection *con;
    int i, ie_index;
    int num_of_symbols, num_of_subchannels, num_of_slots;
    double allocationsize;
    int freeslots;
    int symbol_offset = 0;
    int subchannel_offset = input_subchannel_offset;
    int subchannel_start = 0;
    int num_of_data_connections = 0;
    int total_dl_free_slots = 0;
    ConnectionType_t contype;
    DataDeliveryServiceType_t dataDeliveryServiceType;
    Mac802_16 *  mac_ = getMac ();
    Ofdm_mod_rate mod_rate;

    int slots_per_con[MAX_MAP_IE];
    int cid_list[MAX_MAP_IE];
    int diuc_list[MAX_MAP_IE];


    int leftOTHER_slots=0;
    int needmore_con[3]={0,0,0};
    int needmore_c=0;
    int ori_symbols = total_symbols;
    int req_slots_tmp1 = 0;
    int return_cid_tmp = 0;

    dl_map = (struct mac802_16_dl_map_frame *) malloc(sizeof(struct mac802_16_dl_map_frame));
    dl_map->type = MAC_DL_MAP;
    dl_map->bsid = mac_->addr(); // its called in the mac_ object

    ies = dl_map->ies;
    ie_index = 0; //0 is for the ul-map
    int temp_index_burst = index_burst;

    if ((total_symbols%2)==1) total_symbols=total_symbols-1;
    freeslots = total_subchannels * floor(total_symbols/2);
    total_dl_free_slots = freeslots;

    debug10 ("DL2,****FRAME NUMBER**** :%d, Total_DL_Slots :%d, FreeSlots(TotalSub*TotalSym/2) :%d, TotalSub :%d, Ori_TotalSymbol :%d, TotalSymbol :%d, StartSymbol :%d\n", frame_number, total_dl_slots_pusc, total_dl_free_slots, total_subchannels, ori_symbols, total_symbols, symbol_start );

    frame_number++;

//1. (1st priority) Allocate management message (basic, primary, secondary)
    con=head;
    while (con!=NULL) {
        if (con->get_category()==CONN_DATA) ++num_of_data_connections;
        con = con->next_entry();
    }

    for (i=0; i<3; ++i) {
        con = head;
        if (i==0) 		contype = CONN_BASIC;
        else if (i==1) 	contype = CONN_PRIMARY;
        else 		contype = CONN_SECONDARY;
        while (con!=NULL) {
            if (con->get_category() == contype) {
                allocationsize = con->queueByteLength();

                int con_byte = con->queueByteLength();
                int rep = 1;
                if ( con_byte > 0 ) {
                    Ofdm_mod_rate con_mod = mac_->getMap()->getDlSubframe()->getProfile (con->getPeerNode()->getDIUC())->getEncoding();
                    int con_slots = (int) rep*ceil((double)con_byte/(double)mac_->getPhy()->getSlotCapacity(con_mod, DL_));
                    int virtual_alloc_exist = doesvirtual_allocexist(index_burst, con->get_cid());
                    int add_slots = 0;
                    if (virtual_alloc_exist > 0) {
                        int add_slots = overallocation_withoutdlmap (index_burst, total_dl_slots_pusc, con_slots);
                        int t_index = addslots_withoutdlmap(index_burst, con_byte, con_slots, con->get_cid());
                        debug10 ("DL Add more slots into existing burst contype(%d), index :%d, cid :%d, byte :%f, slots :%d\n", contype, t_index, con->get_cid(), virtual_alloc[t_index].byte, virtual_alloc[t_index].numslots);
                    } else {
                        int add_slots = overallocation_withdlmap (index_burst, total_dl_slots_pusc, con_slots);
                        if (add_slots > 0) {
                            virtual_alloc[index_burst].alloc_type = 5;
                            virtual_alloc[index_burst].cid = con->get_cid();
                            virtual_alloc[index_burst].n_cid = 1;
                            //virtual_alloc[index_burst].iuc = getDIUCProfile(con_mod);
                            virtual_alloc[index_burst].iuc = getDIUCProfile(OFDM_QPSK_1_2);
                            virtual_alloc[index_burst].numslots = add_slots;
                            virtual_alloc[index_burst].byte = (add_slots * mac_->getPhy()->getSlotCapacity(con_mod, DL_));
                            virtual_alloc[index_burst].rep =  rep;
                            virtual_alloc[index_burst].dl_ul = 0;
                            virtual_alloc[index_burst].ie_type = 0;
                            debug10 ("DL Add new burst contype(%d), index :%d, cid :%d, byte :%f, slots :%d\n", contype, index_burst, con->get_cid(), virtual_alloc[index_burst].byte, virtual_alloc[index_burst].numslots);
                            index_burst++;
                        }
                    }
                }//end if q > 0

                debug10 ("DL2.management msg, contype(%d), index :%d, CID :%d, Q-Bytes :%d, allocationsize :%d\n", contype, index_burst, con->get_cid(), con->queueByteLength(), con_byte);
            }
            con = con->next_entry();
        }//end con!=null
    }//end for loop


    /*AMC mechanism. change the modulation coding scheme in each connections.*/
    if (mac_->amc_enable_ ==1) {
        for (i=0; i<5; ++i) {
            con = head;
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
                    mod_rate = con_mod;

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


//2. Calculate #of active connections (ugs, ertps, rtps, nrtps, be)
    int conn_per_schetype[6]={0,0,0,0,0,0};
    int con_ugs_all  =0;
    int con_ertps_all=0;
    int con_rtps_all =0;
    int con_nrtps_all=0;
    int con_be_all   =0;

    for (i=0; i<5; ++i) {
        con = head;
        if (i==0)            dataDeliveryServiceType = DL_UGS;
        else if (i==1)       dataDeliveryServiceType = DL_ERTVR;
        else if (i==2)       dataDeliveryServiceType = DL_RTVR;
        else if (i==3)       dataDeliveryServiceType = DL_NRTVR;
        else                dataDeliveryServiceType = DL_BE;

        while (con!=NULL) {
            if (con->get_category() == CONN_DATA && con->get_serviceflow()->getQosSet()->getDataDeliveryServiceType() == dataDeliveryServiceType) {
                if (dataDeliveryServiceType == DL_UGS) {
                    if (con->queueByteLength() > 0) conn_per_schetype[0]++;
                    con_ugs_all++;
                } else if (dataDeliveryServiceType== DL_ERTVR) {
                    if (con->queueByteLength() > 0) conn_per_schetype[1]++;
                    con_ertps_all++;
                } else if (dataDeliveryServiceType==DL_RTVR) {
                    if (con->queueByteLength() > 0) conn_per_schetype[2]++;
                    con_rtps_all++;
                } else if (dataDeliveryServiceType==DL_NRTVR) {
                    if (con->queueByteLength() > 0) conn_per_schetype[3]++;
                    con_nrtps_all++;
                } else if (dataDeliveryServiceType==DL_BE) {
                    if (con->queueByteLength() > 0) conn_per_schetype[4]++;
                    con_be_all++;
                } else {
                    conn_per_schetype[5]++;
                }
            }
            con = con->next_entry();
        }
    }

    int active_conn_except_ugs = 0;
    for (int i=1; i<5; i++) active_conn_except_ugs =+ conn_per_schetype[i];

    debug10 ("DL2, Active UGS <%d>, ertPS <%d>, rtPS <%d>, nrtPS <%d>, BE <%d>\n", conn_per_schetype[0],conn_per_schetype[1], conn_per_schetype[2], conn_per_schetype[3], conn_per_schetype[4]);
    debug10 ("\tAll UGS <%d>, ertPS <%d>, rtPS <%d>, nrtPS <%d>, BE <%d>\n", con_ugs_all, con_ertps_all, con_rtps_all, con_nrtps_all, con_be_all);

//3. (2nd priority) Allocate UGS
    int next_ie_index = ie_index;
    //***************
    leftOTHER_slots = freeslots;
    //UGS Scheduling
    for (i=0; i<5; ++i) {
        con = head;
        if (i==0)            dataDeliveryServiceType = DL_UGS;

        while (con!=NULL) {
            if (con->get_category() == CONN_DATA && con->get_serviceflow()->getQosSet()->getDataDeliveryServiceType() == dataDeliveryServiceType) {
                req_slots_tmp1 = 0;
                int grant_slots = 0;
                num_of_slots = 0;
                mod_rate = mac_->getMap()->getDlSubframe()->getProfile(con->getPeerNode()->getDIUC())->getEncoding();

                if (dataDeliveryServiceType==DL_UGS) {
                    ServiceFlowQosSet * sfQosSet = con->get_serviceflow()->getQosSet();
                    if (con->queueByteLength() > 0) {
#ifdef UGS_AVG
                        allocationsize = (int) ceil(double(sfQosSet->getMinReservedTrafficRate()) * mac_->getFrameDuration() / 8.0);
#endif
                        /*
#ifndef UGS_AVG
                        int tmp_getpoll = con->getPOLL_interval();
                        if ( (tmp_getpoll%sfQosSet->getPeriod())== 0 ) {
                            allocationsize = ceil(sfQosSet->getDataSize());
                            con->setPOLL_interval(0);
                        } else {
                            allocationsize = 0;
                        }
                        tmp_getpoll++;
                        con->setPOLL_interval(tmp_getpoll);
#endif
*/
                    } else {
                        allocationsize = 0;
                    }

//ARQ aware scheduling
                    int arq_enable_f = 0;
                    if ((con->getArqStatus () != NULL) && (con->getArqStatus ()->isArqEnabled() == 1) ) {
                        arq_enable_f = 1;
                    }
                    int t_bytes = 0;
                    double ori_grant = allocationsize;
                    if ( (arq_enable_f == 1) && (allocationsize > 0) ) {
                        t_bytes = (int)allocationsize % (int)getMac()->arq_block_size_;
                        if (t_bytes > 0) {
                            allocationsize = allocationsize + (getMac()->arq_block_size_ - t_bytes);
                        }
                        debug10 ("\tARQ enable CID :%d, arq_block :%d, requested_size :%f, arq_block_boundary_size :%f, all_header_included :%f\n", con->get_cid(), getMac()->arq_block_size_, ori_grant, allocationsize, allocationsize + (double)14);
                        allocationsize = allocationsize + 14;	//optional => MAC+frag_sub+pack_sub+gm_sub+mesh_sub

                    }
//End ARQ aware scheduling

                    grant_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, DL_));;
                    if (con->queueByteLength() <=0) {
                        allocationsize = 0;
                        num_of_slots = 0;
                    } else {
                        num_of_slots = grant_slots;
                    }

                    Ofdm_mod_rate con_mod = mac_->getMap()->getDlSubframe()->getProfile (con->getPeerNode()->getDIUC())->getEncoding();
                    int con_slots = num_of_slots;
                    int con_byte = allocationsize;
                    int virtual_alloc_exist = doesvirtual_allocexist(index_burst, con->get_cid());
                    int add_slots = 0;
                    if (virtual_alloc_exist > 0) {
                        int add_slots = overallocation_withoutdlmap (index_burst, total_dl_slots_pusc, con_slots);
                        addslots_withoutdlmap(index_burst, con_byte, con_slots, con->get_cid());
                    } else {
                        int add_slots = overallocation_withdlmap (index_burst, total_dl_slots_pusc, con_slots);
                        if (add_slots > 0) {
                            virtual_alloc[index_burst].alloc_type = 5;
                            virtual_alloc[index_burst].cid = con->get_cid();
                            virtual_alloc[index_burst].n_cid = 1;
                            virtual_alloc[index_burst].iuc = getDIUCProfile(con_mod);
                            virtual_alloc[index_burst].numslots = add_slots;
                            virtual_alloc[index_burst].byte = (add_slots * mac_->getPhy()->getSlotCapacity(con_mod, DL_));
                            virtual_alloc[index_burst].rep =  1;
                            virtual_alloc[index_burst].dl_ul = 0;
                            virtual_alloc[index_burst].ie_type = 0;
                            index_burst++;
                        }
                    }

                    debug10 ("DL2.UGS, MRTR :%f, numslots :%d, CID :%d, DIUC :%d\n", double(sfQosSet->getMinReservedTrafficRate()), con_slots, con->get_cid(), con->getPeerNode()->getDIUC());
                    debug10 ("\tQ-bytes :%d, grant-bytes :%ld\n", con->queueByteLength(), (long int)( con_slots*mac_->getPhy()->getSlotCapacity(mod_rate, DL_) ));

                }//end UGS
            }
            con = con->next_entry();
        }//end con != NULL
    }//end UGS


//4. (3rd priority) Allocate to Others
//In this version, we validate the result only on best effort, we do not test rtPS, ertPS, and nrtPS yet.
//Fair allocation
//Find out how many connections can be supported given variable DL MAP
    int max_conn_dlmap = max_conn_withdlmap(index_burst, total_dl_slots_pusc);
    int active_shared_conn = active_conn_except_ugs;
    if (active_shared_conn > max_conn_dlmap) active_shared_conn = max_conn_dlmap;

    double weighted_isp = 1;
    int max_conn = MAX_CONN;
    int fair_share[max_conn];
    int dl_all_con=0;

    con_drr dl_con_data[MAX_CONN];
    con_data_alloc con_data[max_conn];
    con_data_alloc con_data_final[max_conn];

    bzero(fair_share, max_conn*sizeof(int));
    bzero(con_data, max_conn*sizeof(con_data_alloc));
    bzero(con_data_final, max_conn*sizeof(con_data_alloc));
    bzero(dl_con_data, max_conn*sizeof(con_drr));

    for (int i=0; i<MAX_CONN; i++) {
        dl_con_data[i].cid = -2;
        dl_con_data[i].quantum = 0;
        dl_con_data[i].counter = 0;
    }

//Find out #request_slots for each connection
    int j_con = 0;

    for (i=0; i<5; ++i) {
        con = head;
        if (i==0)        dataDeliveryServiceType = DL_UGS;
        else if (i==1)   dataDeliveryServiceType = DL_ERTVR;
        else if (i==2)   dataDeliveryServiceType = DL_RTVR;
        else if (i==3)   dataDeliveryServiceType = DL_NRTVR;
        else            dataDeliveryServiceType = DL_BE;

        if ( i==0 ) continue;

        while (con != NULL) {

            if (con->get_category() == CONN_DATA && con->get_serviceflow()->getQosSet()->getDataDeliveryServiceType() == dataDeliveryServiceType) {
                Ofdm_mod_rate mod_rate = mac_->getMap()->getDlSubframe()->getProfile(con->getPeerNode()->getDIUC())->getEncoding();

                int withfrag = 0;
                int length = 0;
                if (con->getArqStatus () != NULL && con->getArqStatus ()->isArqEnabled() == 1) {
                    if (con->getArqStatus()->arq_retrans_queue_->byteLength()>0 ||con->getArqStatus()->arq_trans_queue_->byteLength() >0 ) {
                        debug2("ARQ retrans queue length is %d\n\n",con->getArqStatus()->arq_retrans_queue_->byteLength() );
                        if (con->getArqStatus()->arq_retrans_queue_->byteLength()>0)
                            length =con->getArqStatus()->arq_retrans_queue_->byteLength();
                        else
                            length = con->getArqStatus()->arq_trans_queue_->byteLength();
                    } else {
                        length = con->queueByteLength();
                    }
                } else {
                    length = con->queueByteLength();
                }

                if  (con->getFragmentBytes()>0) {
                    withfrag = length - con->getFragmentBytes() + 4;

                } else {
                    withfrag = length+4;
                }

                debug2("ARQ retrans queue got allocation size %d\n\n", withfrag);
                //ARQ aware scheduling
                int arq_enable_f = 0;
                if ((con->getArqStatus () != NULL) && (con->getArqStatus ()->isArqEnabled() == 1) ) {
                    arq_enable_f = 1;
                }
                int t_bytes = 0;
                double ori_grant = withfrag;
                if ( (arq_enable_f == 1) && (allocationsize > 0) ) {
                    t_bytes = (int)withfrag % (int)getMac()->arq_block_size_;
                    if (t_bytes > 0) {
                        withfrag = withfrag + (getMac()->arq_block_size_ - t_bytes);
                    }
                    debug10 ("\t ARQ enable CID :%d, arq_block :%d, requested_size :%f, arq_block_boundary_size :%d, all_header_included :%d\n", con->get_cid(), getMac()->arq_block_size_, ori_grant, withfrag, withfrag + 14);
                    withfrag = withfrag + 14;   //optional => MAC+frag_sub+pack_sub+gm_sub+mesh_sub
                }
                //End ARQ

                int req_slots = ceil((double)withfrag/(double)mac_->getPhy()->getSlotCapacity(mod_rate, DL_));
                if (req_slots > 0) {
                    debug10 ("Con-Request byte :%d, slotcapa :%d, reqslot :%d\n", con->queueByteLength(), mac_->getPhy()->getSlotCapacity(mod_rate, DL_), req_slots );
                    Packet *np;
                    if ( (np = con->queueLookup(0)) != NULL ) {
                        int p_size = hdr_cmn::access(np)->size();
                        debug10 ("CON CID :%d, q->byte :%d, q->len :%d, packet_size :%d, frag_no :%d, frag_byte :%d, frag_stat :%d\n", con->get_cid(), con->queueByteLength(), con->queueLength(), p_size, con->getFragmentNumber(), con->getFragmentBytes(), (int)con->getFragmentationStatus() );
                    }

                    Ofdm_mod_rate t_mod_rate = mac_->getMap()->getDlSubframe()->getProfile(con->getPeerNode()->getDIUC())->getEncoding();
                    con_data[j_con].cid = con->get_cid();
                    con_data[j_con].direction = DL_;
                    con_data[j_con].mod_rate = t_mod_rate;
                    con_data[j_con].req_slots = req_slots;
                    con_data[j_con].req_bytes = withfrag;
                    con_data[j_con].grant_slots = 0;
                    con_data[j_con].weight = 0;
                    con_data[j_con].counter = 0;
                    j_con++;
                } else {
                }//end else
            }
            con = con->next_entry();
        }
    }


















    if (j_con == active_conn_except_ugs) debug10 ("DL2.other QoS flows OK :num_con :%d\n", j_con);
    else  debug10 ("Error Panic\n");

//Sorting and make sure #request slots is correct
    if (j_con>1)  {
        for (int i=0; i<j_con; i++) {
            debug10 ("DL2.other QoS flows, Req_slots Before i :%d, cid :%d, counter :%d, req_slots :%d, mod :%d\n", i, con_data[i].cid, con_data[i].counter, con_data[i].req_slots, (int)con_data[i].mod_rate);
        }
        bubble_sort(j_con, con_data, 0);
        for (int i=0; i<j_con; i++) {
            debug10 ("DL2.other QoS flows, Req_slots After i :%d, cid :%d, counter :%d, req_slots :%d, mod :%d\n", i, con_data[i].cid, con_data[i].counter, con_data[i].req_slots, (int)con_data[i].mod_rate);
        }
    }

    int t_freeslots = 0;
    int j_need = j_con;
    if (j_need > max_conn_dlmap) j_need = max_conn_dlmap;
    if (j_need > 0 ) t_freeslots = freeslots_withdlmap_given_conn(index_burst, total_dl_slots_pusc, j_need);

//Algorithm is here
//Note that in this version, we do slot-fair allocation or different MCSs will get the same throughput
//Next version, we will include weighted fair allocation
    int j_count = j_need;
    for (int i=0; i<j_need; i++) {
        if ( (t_freeslots <=0) ) break;
        fair_share[i] = floor(t_freeslots/j_count);
        if (fair_share[i] >= con_data[i].req_slots) {
            con_data[i].grant_slots = con_data[i].req_slots;
        } else {
            con_data[i].grant_slots = fair_share[i];
        }
        debug10 ("DL2.other QoS flows, Freeslots :%d, fairshare[%d] ;%d, grant_slots :%d\n", t_freeslots, i, fair_share[i], con_data[i].grant_slots);
        t_freeslots = t_freeslots - con_data[i].grant_slots;
        j_count--;
    }

    for (int i=0; i<j_need; i++) {
        debug10 ("\tAfter fair share ->Con :%d, req_slots :%d, grant_slots :%d\n", con_data[i].cid, con_data[i].req_slots, con_data[i].grant_slots);
    }

//Assign granted slots to each connection (virtual burst)
    for (int i=0; i<j_need; i++) {
        if (con_data[i].grant_slots >=0 ) {
            Ofdm_mod_rate con_mod = con_data[i].mod_rate;
            int con_slots = con_data[i].grant_slots;
            int con_byte = (con_slots * mac_->getPhy()->getSlotCapacity(con_mod, DL_));
            debug10 ("OTHERS, i ;%d, numslots :%d, bytes :%d, CID :%d\n", i, con_slots, con_byte, con_data[i].cid);

            int virtual_alloc_exist = doesvirtual_allocexist(index_burst, con_data[i].cid);
            int add_slots = 0;
            if (virtual_alloc_exist > 0) {
                int add_slots = overallocation_withoutdlmap (index_burst, total_dl_slots_pusc, con_slots);
                addslots_withoutdlmap(index_burst, con_byte, con_slots, con_data[i].cid);
            } else {
                int add_slots = overallocation_withdlmap (index_burst, total_dl_slots_pusc, con_slots);

                if (add_slots > 0) {
                    virtual_alloc[index_burst].alloc_type = 5;
                    virtual_alloc[index_burst].cid = con_data[i].cid;
                    virtual_alloc[index_burst].n_cid = 1;
                    virtual_alloc[index_burst].iuc = getDIUCProfile(con_mod);
                    virtual_alloc[index_burst].numslots = add_slots;
                    virtual_alloc[index_burst].byte = (add_slots * mac_->getPhy()->getSlotCapacity(con_mod, DL_));
                    virtual_alloc[index_burst].rep =  1;
                    virtual_alloc[index_burst].dl_ul = 0;
                    virtual_alloc[index_burst].ie_type = 0;
                    index_burst++;
                }
            }

            debug10 ("DL2.other QoS flows, index :%d, grant_numslots :%d, grant_bytes :%d, CID :%d\n", i, con_slots, (int)( con_slots*mac_->getPhy()->getSlotCapacity(con_data[i].mod_rate, DL_)), con_data[i].cid);
        }

    }

    int total_slots_sofar = check_overallocation(temp_index_burst);
    subchannel_offset = (total_slots_sofar%total_subchannels);
    symbol_offset = DL_PREAMBLE + floor(total_slots_sofar/total_subchannels)*2;
    debug10 ("DL2.end of dl_stage2, Sum allocated slots :%d, Suboffset :%d, Symoffset :%d\n", total_slots_sofar, subchannel_offset, symbol_offset);

    ie_index = index_burst - temp_index_burst;

    dl_map->nb_ies = ie_index;
    return dl_map;
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

//Modified by Chakchai
//Unlike dl_stage2, next version will be a wrap up version fo the UL scheduler with horizontal stripping
struct mac802_16_ul_map_frame * BSScheduler::ul_stage2(Connection *head, int total_subchannels, int total_symbols, int symbol_start, int stripping) {
    struct mac802_16_ul_map_frame *ul_map;
    struct mac802_16_ulmap_ie *ies;
    Connection *con;
    int i, ie_index, temp_index;
    int num_of_slots, num_of_symbols, num_of_subchannels;
    double allocationsize;
    int freeslots;
    int symbol_offset = 0;
    int subchannel_offset = 0;
    int subchannel_start = 0;
    ConnectionType_t contype;
    UlGrantSchedulingType_t ulGrantSchedulingType;
    int slots_per_con[MAX_MAP_IE];
    int cid_list[MAX_MAP_IE];
    int uiuc_list[MAX_MAP_IE];
    int cdma_flag_list[MAX_MAP_IE];
    u_char cdma_code_list[MAX_MAP_IE];
    u_char cdma_top_list[MAX_MAP_IE];

    Ofdm_mod_rate mod_rate;

    int leftOTHER_slots=0;
    int needmore_con[3]={0,0,0};
    int needmore_c=0;
    int ori_symbols = total_symbols;
    int req_slots_tmp1 = 0;
    int return_cid_tmp = 0;
    num_of_slots=0;
    int tmp_cdma_code_top[CODE_SIZE][CODE_SIZE];

    for (int i=0; i<CODE_SIZE; i++) {
        for (int j=0; j<CODE_SIZE; j++) {
            tmp_cdma_code_top[i][j] = 0;
        }
    }

    if ((total_symbols%3)==1) total_symbols=total_symbols-1;
    else if ((total_symbols%3)==2) total_symbols=total_symbols-2;

    freeslots = total_subchannels * total_symbols/3;
    leftOTHER_slots = freeslots;

    debug10 ("UL.Check1.0, ****FRAME NUMBER**** :%d, FreeSlots(TotalSub*TotalSym/3) :%d, TotalSub :%d, OriSymbol(maxul-ulduration) :%d, TotalSymbol :%d, StartSymbol :%d\n", frame_number, freeslots, total_subchannels, ori_symbols, total_symbols, symbol_start );

    ul_map = (struct mac802_16_ul_map_frame *) malloc(sizeof(struct mac802_16_ul_map_frame));
    bzero(ul_map, sizeof(struct mac802_16_ul_map_frame));
    ul_map->type = MAC_UL_MAP;

    ies = ul_map->ies;
    ie_index = 0;

    bzero(slots_per_con, MAX_MAP_IE*sizeof(int));
    bzero(cid_list, MAX_MAP_IE*sizeof(int));
    for (int i=0; i<MAX_MAP_IE; i++) {
        cdma_code_list[i] = 0;
        cdma_top_list[i] = 0;
        cdma_flag_list[i] = 0;
    }

//Check if cdma_init_ranging request
    con = head;
    while (con!=NULL) {
        if (con->get_category() == CONN_INIT_RANGING) {
            if (con->getCDMA() == 2) {
                int begin_code = 0;
                int begin_top = 0;
                int begin_flag = 0;
                for (int i = 0; i<MAX_SSID; i++) {
                    begin_flag = con->getCDMA_SSID_FLAG(i);
                    begin_code = con->getCDMA_SSID_CODE(i);
                    begin_top = con->getCDMA_SSID_TOP(i);
                    if (begin_flag == 0) continue;
                    for (int j = i+1; j<MAX_SSID; j++) {
                        if (con->getCDMA_SSID_FLAG(j) == 0) continue;
                        if ( (begin_code == con->getCDMA_SSID_CODE(j)) && (begin_top == con->getCDMA_SSID_TOP(j)) ) {
                            debug10 ("=Collission CDMA_INIT_RNG_REQ (ssid i :%d and ssid j :%d), CDMA_flag i :%d and CDMA_flag j:%d, CDMA_code i :%d, CDMA_top i :%d\n", con->getCDMA_SSID_SSID(i), con->getCDMA_SSID_SSID(j), con->getCDMA_SSID_FLAG(i), con->getCDMA_SSID_FLAG(j), con->getCDMA_SSID_CODE(i), con->getCDMA_SSID_TOP(i));
                            con->setCDMA_SSID_FLAG(j, 0);
                            con->setCDMA_SSID_FLAG(i, 0);
                        }
                    }
                }
            }

            con->setCDMA(0);
            break;
        }
        con = con->next_entry();
    }

    con = head;
    while (con!=NULL) {
        if (con->get_category() == CONN_INIT_RANGING) {
            for (int i = 0; i<MAX_SSID; i++) {
                int cdma_flag = con->getCDMA_SSID_FLAG(i);
                if (cdma_flag > 0) {

                    mod_rate = (Ofdm_mod_rate)1;
                    //mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getDIUC()-DIUC_PROFILE_1+UIUC_PROFILE_1)->getEncoding();
//            allocationsize =  RNG_REQ_SIZE+GENERIC_HEADER_SIZE;
                    allocationsize =  RNG_REQ_SIZE;
                    int num_of_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                    con->setCDMA_SSID_FLAG(i,0);
                    debug10("=> Allocate init_ranging_msg opportunity for ssid :%d, code :%d, top :%d, size :%f\n", con->getCDMA_SSID_SSID(i), con->getCDMA_SSID_CODE(i), con->getCDMA_SSID_TOP(i), allocationsize);


                    if (freeslots < num_of_slots) num_of_slots = freeslots;

                    freeslots -= num_of_slots;

                    if (num_of_slots> 0) {
                        int temp_index = ie_index++;
                        cid_list[temp_index] = con->get_cid();
                        slots_per_con[temp_index] = num_of_slots;
                        uiuc_list[temp_index] = getUIUCProfile(mod_rate);
                        cdma_code_list[temp_index] = con->getCDMA_SSID_CODE(i);
                        cdma_top_list[temp_index] = con->getCDMA_SSID_TOP(i);
                        cdma_flag_list[temp_index] = 1;
                    }
                    con->setCDMA_SSID_FLAG(i, 0);
                    con->setCDMA_SSID_CODE(i, 0);
                    con->setCDMA_SSID_TOP(i, 0);
                }
            }
        }
        con = con->next_entry();
    }


    int ss_mac_id;
    bool has_finished_one_round = false;
    /*AMC mechanism. change the modulation coding scheme in each connections.*/
    if (mac_->amc_enable_ ==1) {
        debug2("\n\nGoing to do UL AMC processing.\n\n");
        for (i=0; i<5; ++i) {
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
                    mod_rate = con_mod;
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

//Check if cdma_bandwidth_ranging request
    for (i=0; i<4; ++i) {
        con = head;
        if (i==0) 	  contype = CONN_BASIC;
        else if (i==1) contype = CONN_PRIMARY;
        else if (i==2) contype = CONN_SECONDARY;
        else contype = CONN_DATA;

        while (con!=NULL) {
            if (con->get_category() == contype) {
                if (con->getCDMA() == 1) {
                    tmp_cdma_code_top[(int)con->getCDMA_code()][(int)con->getCDMA_top()]++;
                }
                if (con->getCDMA()>0) {
                    debug10 ("=Contype :%d, CDMA_flag :%d, CDMA_code :%d, CDMA_top :%d, CDMA_code_top++ :%d\n", contype, con->getCDMA(), con->getCDMA_code(), con->getCDMA_top(), tmp_cdma_code_top[(int)con->getCDMA_code()][(int)con->getCDMA_top()]);
                }
            }
            con = con->next_entry();
        }

    }//end for

    for (i=0; i<4; ++i) {
        con = head;
        if (i==0) 	  contype = CONN_BASIC;
        else if (i==1) contype = CONN_PRIMARY;
        else if (i==2) contype = CONN_SECONDARY;
        else contype = CONN_DATA;

        while (con!=NULL) {
            if (con->get_category() == contype) {
//      	debug10 ("=Contype :%d, CDMA_flag :%d, CDMA_code :%d, CDMA_top :%d\n", contype, con->getCDMA(), con->getCDMA_code(), con->getCDMA_top());
                if (con->getCDMA() == 1) {
                    if ( tmp_cdma_code_top[(int)con->getCDMA_code()][(int)con->getCDMA_top()] >1 ) {
                        con->setCDMA(0);
                        debug10 ("=Collission CDMA_BW_REQ, Contype :%d, CDMA_flag :%d, CDMA_code :%d, CDMA_top :%d, CDMA_code_top :%d\n", contype, con->getCDMA(), con->getCDMA_code(), con->getCDMA_top(), tmp_cdma_code_top[(int)con->getCDMA_code()][(int)con->getCDMA_top()]);
                    }
                }
            }
            con = con->next_entry();
        }

    }//end for


    for (i=0; i<4; ++i) {
        con = head;
        if (i==0) 	  contype = CONN_BASIC;
        else if (i==1) contype = CONN_PRIMARY;
        else if (i==2) contype = CONN_SECONDARY;
        else contype = CONN_DATA;

        while (con!=NULL) {
            if (con->get_category() == contype) {

                mod_rate = (Ofdm_mod_rate)1;
                num_of_slots = 0;
                allocationsize = 0;

                if (con->getCDMA() == 1) {
                    allocationsize = GENERIC_HEADER_SIZE; 					//for explicit polling
                    int tmp_num_poll = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                    num_of_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                    debug10 ("\tUL.Check1.1.contype(%d), Polling CDMA :%f, numslots :%d, freeslot :%d, CID :%d\n", contype, allocationsize, num_of_slots, freeslots, con->get_cid());
                    con->setCDMA(0);
                }//end getCDMA

                if (freeslots < num_of_slots) num_of_slots = freeslots;

                freeslots -= num_of_slots;

                if (num_of_slots> 0) {
                    temp_index = doesMapExist(con->get_cid(), cid_list, ie_index); //conn, ie list, number of ies
                    if (temp_index < 0) {
                        temp_index = ie_index;
                        cid_list[temp_index] = con->get_cid();
                        slots_per_con[temp_index] = num_of_slots;
                        uiuc_list[temp_index] = getUIUCProfile(mod_rate);
                        cdma_code_list[temp_index] = con->getCDMA_code();
                        cdma_top_list[temp_index] = con->getCDMA_top();
                        cdma_flag_list[temp_index] = 1;
                        ++ie_index;
                    } else if (temp_index < MAX_MAP_IE)	 slots_per_con[temp_index] += num_of_slots;
                    else	 				 freeslots += num_of_slots;

                    debug10 ("UL.Check1.2, Polling CDMA (basic/pri/sec): CID(%d), Contype(B:5, P:6, S:7, D:8) :%d, Numslots :%d, Free :%d, allo :%e, StoreSlots[%d] :%d\n", cid_list[temp_index], contype, num_of_slots, freeslots, allocationsize, temp_index, slots_per_con[temp_index]);

                }
            }
            con = con->next_entry();
        }
    }


    //Assign to basic, primary, and secondary now
    for (i=0; i<3; ++i) {
        con = head;
        if (i==0) 	  contype = CONN_BASIC;
        else if (i==1) contype = CONN_PRIMARY;
        else	  contype = CONN_SECONDARY;

        while (con!=NULL) {
            if (con->get_category() == contype) {

                //mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getDIUC()-DIUC_PROFILE_1+UIUC_PROFILE_1)->getEncoding();
                mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getUIUC())->getEncoding();
                //if (mod_rate > OFDM_16QAM_3_4) {
                //  mod_rate = OFDM_16QAM_3_4;
                //}

                mod_rate = OFDM_QPSK_1_2; //Xingting added.
//Chakchai Modified 22 May 2008
                num_of_slots = 0;
                allocationsize = 0;
//

                if (con->getBw() > 0 ) {
                    allocationsize = con->getBw();
                    num_of_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));

                    debug10 ("\tBwreq Still>0 :%f, numslots :%d, freeslot :%d\n", allocationsize, num_of_slots, freeslots);
                } else {
                    if (con->getCDMA() == 1) {
                        allocationsize = GENERIC_HEADER_SIZE; 					//for explicit polling
                        int tmp_num_poll = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                        int still_setbw = (int) ceil(con->getBw()/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));

                        if (tmp_num_poll>still_setbw) {
                            num_of_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                            debug10 ("\tPolling CDMA :%f, numslots :%d, freeslot :%d\n", allocationsize, num_of_slots, freeslots);
                        } else {
                            debug10 ("\tStill set BW Not Polling\n");
                        }
                    }//end getCDMA
                    con->setCDMA(0);
                }

                if (num_of_slots>0) {
                    debug10 ("UL.Check1.1.contype(%d), allocationsize (con->getBW) :%f, #slots :%d\n", contype, allocationsize, num_of_slots);
                    debug10 ("\tfree slots :%d, left-over :%d\n", freeslots, freeslots-num_of_slots);
                }

                if ( (allocationsize>0) || (num_of_slots>0) ) debug2 ("=> Allocation size %f, num of slots %d\n", allocationsize, num_of_slots);

                if (freeslots < num_of_slots) num_of_slots = freeslots;

                freeslots -= num_of_slots;

#ifdef BWREQ_PATCH
                //decrement bandwidth requested by allocation
                debug2 ("\tCtrl CID= %d Req= %d Alloc= %d Left=%d\n",
                        con->get_cid(), con->getBw(),
                        num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_),
                        con->getBw()-(num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
                con->setBw(con->getBw()-(num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
#endif

                if (num_of_slots> 0) {
                    temp_index = doesMapExist(con->getPeerNode()->getBasic(IN_CONNECTION)->get_cid(), cid_list, ie_index); //conn, ie list, number of ies
//          temp_index = doesMapExist(con->get_cid(), cid_list, ie_index); //conn, ie list, number of ies

                    if (temp_index < 0) {
                        temp_index = ie_index;
                        cid_list[temp_index] = con->getPeerNode()->getBasic(IN_CONNECTION)->get_cid();
                        //     cid_list[temp_index] = con->get_cid();

                        slots_per_con[temp_index] = num_of_slots;
                        uiuc_list[temp_index] = getUIUCProfile(mod_rate);
                        ++ie_index;
                    } else if (temp_index < MAX_MAP_IE)	 slots_per_con[temp_index] += num_of_slots;
                    else	 				 freeslots += num_of_slots;

                    debug10 ("UL.Check1.2, Init Assign (basic/pri/sec): CID(%d), Contype(B:5, P:6, S:7, D:8) :%d, Numslots :%d, Free :%d, allo :%e, StoreSlots[%d] :%d\n", cid_list[temp_index], contype, num_of_slots, freeslots, allocationsize, temp_index, slots_per_con[temp_index]);

                }
            }
            con = con->next_entry();
        }
    }


    leftOTHER_slots = freeslots;
    int conn_per_schetype[6]={0,0,0,0,0,0};
    conn_per_schetype[0]=0;
    conn_per_schetype[1]=0;
    conn_per_schetype[2]=0;
    conn_per_schetype[3]=0;
    conn_per_schetype[4]=0;
    conn_per_schetype[5]=0;

    int con_ertps_all=0;
    int con_rtps_all=0;
    int con_nrtps_all=0;
    int con_be_all=0;

    for (i=0; i<5; ++i) {
        con = head;
        if (i==0)            ulGrantSchedulingType = UL_UGS;
        else if (i==1)       ulGrantSchedulingType = UL_ertPS;
        else if (i==2)       ulGrantSchedulingType = UL_rtPS;
        else if (i==3)       ulGrantSchedulingType = UL_nrtPS;
        else                ulGrantSchedulingType = UL_BE;

        while (con!=NULL) {
            if (con->get_category() == CONN_DATA && con->get_serviceflow()->getQosSet()->getUlGrantSchedulingType() == ulGrantSchedulingType) {
                //mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getDIUC()-DIUC_PROFILE_1+UIUC_PROFILE_1)->getEncoding();
                mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getUIUC())->getEncoding();

                if (ulGrantSchedulingType==UL_UGS) {
                    conn_per_schetype[0]++;
                } else if (ulGrantSchedulingType==UL_ertPS) {
                    if (con->getBw() > 0) conn_per_schetype[1]++;
                    con_ertps_all++;

                } else if (ulGrantSchedulingType==UL_rtPS) {
                    if (con->getBw() > 0) conn_per_schetype[2]++;
                    con_rtps_all++;
                } else if (ulGrantSchedulingType==UL_nrtPS) {
                    if (con->getBw() > 0) conn_per_schetype[3]++;
                    con_nrtps_all++;
                } else if (ulGrantSchedulingType==UL_BE) {
                    if (con->getBw() > 0) conn_per_schetype[4]++;
                    con_be_all++;
                } else {
                    conn_per_schetype[5]++;
                }
            }
            con = con->next_entry();
        }
    }

    debug10 ("UL, Active -UGS <%d>, -ertPS <%d>, -rtPS <%d>, -nrtPS <%d>, -BE <%d>, -OTHER <%d>\n", conn_per_schetype[0],conn_per_schetype[1], conn_per_schetype[2], conn_per_schetype[3], conn_per_schetype[4], conn_per_schetype[5] );
    debug10 ("    All     UGS <%d>,  ertPS <%d>,  rtPS <%d>,  nrtPS <%d>,  BE <%d>\n", conn_per_schetype[0], con_ertps_all, con_rtps_all, con_nrtps_all, con_be_all);



    for (i=0; i<5; ++i) {
        con = head;
        if (i==0)            ulGrantSchedulingType = UL_UGS;
        else if (i==1)       ulGrantSchedulingType = UL_ertPS;
        else if (i==2)       ulGrantSchedulingType = UL_rtPS;
        else if (i==3)       ulGrantSchedulingType = UL_nrtPS;
        else                ulGrantSchedulingType = UL_BE;


        while (con!=NULL) {
            if (con->get_category() == CONN_DATA && con->get_serviceflow()->getQosSet()->getUlGrantSchedulingType() == ulGrantSchedulingType) {
                //Richard: we must look up the rate again
                //mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getDIUC()-DIUC_PROFILE_1+UIUC_PROFILE_1)->getEncoding();
                mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getUIUC())->getEncoding();
                /*if (mod_rate > OFDM_16QAM_3_4) {
                  mod_rate = OFDM_16QAM_3_4;
                }*/

                req_slots_tmp1 = 0;
                return_cid_tmp = 0;
                int grant_slots = 0;
                num_of_slots = 0;

                return_cid_tmp = 0;
                temp_index = doesMapExist(con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), cid_list, ie_index);
                if (temp_index < 0) return_cid_tmp = -1;
                else return_cid_tmp = con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid();

                if (ulGrantSchedulingType==UL_UGS) {
                    ServiceFlowQosSet *sfQosSet = con->get_serviceflow()->getQosSet();


#ifdef UGS_AVG
                    allocationsize = (int) ceil( double(sfQosSet->getMinReservedTrafficRate()) / ( mac_->getFrameDuration() * 8 ) );
#endif

                    /* TODO Adapt to new QoS parameters vr@tud
#ifndef UGS_AVG
                    int tmp_getpoll = con->getPOLL_interval();
                    if ( (tmp_getpoll%sfQosSet->getGrantInterval())== 0 ) {
                        allocationsize = ceil(sfQosSet->getDataSize());
                        con->setPOLL_interval(0);
                    } else {
                        allocationsize = 0;
                    }
                    tmp_getpoll++;
                    con->setPOLL_interval(tmp_getpoll);
#endif
*/

                    int arq_enable_f = 0;
                    if ((con->getArqStatus () != NULL) && (con->getArqStatus ()->isArqEnabled() == 1) ) {
                        arq_enable_f = 1;
                    }
                    int t_bytes = 0;
                    double ori_grant = allocationsize;
                    if ( (arq_enable_f == 1) && (allocationsize > 0) ) {
                        t_bytes = (int)allocationsize % (int)getMac()->arq_block_size_;
                        if (t_bytes > 0) {
                            allocationsize = allocationsize + (getMac()->arq_block_size_ - t_bytes);
                        }
                        debug10 (" ARQ enable CID :%d, arq_block :%d, requested_size :%f, arq_block_boundary_size :%f, all_header_included :%f\n", con->get_cid(), getMac()->arq_block_size_, ori_grant, allocationsize, allocationsize + (double)14);
                        allocationsize = allocationsize + 14;   //optional => MAC+frag_sub+pack_sub+gm_sub+mesh_sub

                    }

                    grant_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));

                    num_of_slots = grant_slots;

                    if (leftOTHER_slots<=num_of_slots) {
                        debug10 ("Fatal Error: There is not enough resource for UGS\n");
                        exit(1);
                    } else leftOTHER_slots=leftOTHER_slots-num_of_slots;


                    debug10 ("UL.Check1.3.UGS, DataSize :%f, PRE-GRANT-SLOTS :%d, Peer-CID :%d, returnCID :%d, DIUC :%d\n", double(sfQosSet->getMinReservedTrafficRate()), sfQosSet->getGrantInterval(), grant_slots, con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), return_cid_tmp, con->getPeerNode()->getDIUC());
                    debug10 ("\tAllocatedSLots :%d, BeforeFree :%d, LeftforOTHER :%d, NumofUGS :%d\n", num_of_slots, freeslots, leftOTHER_slots, conn_per_schetype[0]);

                }

                if (ulGrantSchedulingType == UL_ertPS) {

                	ServiceFlowQosSet *sfQosSet = con->get_serviceflow()->getQosSet();
                    req_slots_tmp1 = (int) ceil((double)con->getBw()/(double)mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                    int issue_pol = 0;

                    /*
                    	  if (req_slots_tmp1>0) allocationsize = (int) ceil(sfqos->getDataSize()/sfqos->getPeriod());
                    	  else  allocationsize = GENERIC_HEADER_SIZE; 					//for explicit polling
                    */

                    int tmp_getpoll = con->getPOLL_interval();
                    if (req_slots_tmp1>0) {
                        allocationsize = ceil( double(sfQosSet->getMinReservedTrafficRate()) * double(sfQosSet->getGrantInterval()) / ( 1000 * 8 ));
                        debug10 ("\tPoll_ertPS: No polling (bw-req>0), lastpoll :%d, period :%d\n", tmp_getpoll, sfQosSet->getGrantInterval());
                        con->setPOLL_interval(0);
                    } else  {
                        if ( (tmp_getpoll% sfQosSet->getGrantInterval()) >=  ( mac_->getFrameDuration() * 1000)) {
                            debug10 ("\tPoll_ertPS(yes): Issues unicast poll, lastpoll :%d, period :%d\n", tmp_getpoll, sfQosSet->getGrantInterval());

                            allocationsize = GENERIC_HEADER_SIZE;                                        //for explicit polling
                            con->setPOLL_interval(0);
                            issue_pol = 1;
                        } else {
                            debug10 ("\tPoll_ertPS(no): Don't issues unicast poll, lastpoll :%d, period :%d\n", tmp_getpoll, sfQosSet->getGrantInterval());
                            allocationsize = 0;
                        }
                        debug10 ("\tPoll_ertPS: Current polling_counter :%d, update_polling :%d, period :%d\n", tmp_getpoll, tmp_getpoll+1, sfQosSet->getGrantInterval());
                        tmp_getpoll += int( mac_->getFrameDuration() * 1000);
                        con->setPOLL_interval(tmp_getpoll);
                    }

                    int arq_enable_f = 0;
                    if ((con->getArqStatus () != NULL) && (con->getArqStatus ()->isArqEnabled() == 1) && (issue_pol==0) ) {
                        arq_enable_f = 1;
                    }
                    int t_bytes = 0;
                    double ori_grant = allocationsize;
                    if ( (arq_enable_f == 1) && (allocationsize > 0) ) {
                        t_bytes = (int)allocationsize % (int)getMac()->arq_block_size_;
                        if (t_bytes > 0) {
                            allocationsize = allocationsize + (getMac()->arq_block_size_ - t_bytes);
                        }
                        debug10 (" ARQ enable CID :%d, arq_block :%d, requested_size :%f, arq_block_boundary_size :%f, all_header_included :%f\n", con->get_cid(), getMac()->arq_block_size_, ori_grant, allocationsize, allocationsize + (double)14);
                        allocationsize = allocationsize + 14;   //optional => MAC+frag_sub+pack_sub+gm_sub+mesh_sub

                    }

                    grant_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                    num_of_slots = grant_slots;

#ifdef BWREQ_PATCH
                    //decrement bandwidth requested by allocation
                    debug2 ("\tertPS: CID= %d Req= %d Alloc= %d Left=%d\n",
                            con->get_cid(), con->getBw(),
                            num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_),
                            con->getBw()-(num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
                    con->setBw(con->getBw()-(num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
#endif

                    if (leftOTHER_slots<=num_of_slots) {
                        debug10 ("Fatal Error: There is not enough resource for ertPS\n");
                        exit(1);
                    } else leftOTHER_slots=leftOTHER_slots-num_of_slots;


                    debug10 ("UL.Check1.3.ertPS, DataSize :%f, period :%d, PRE-GRANT-SLOTS :%d, Peer-CID :%d, returnCID :%d, DIUC :%d\n", sfQosSet->getMinReservedTrafficRate(), sfQosSet->getGrantInterval(), grant_slots, con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), return_cid_tmp, con->getPeerNode()->getDIUC());
                    debug10 ("\tAllocatedSLots :%d, BeforeFree :%d, LeftforOTHER :%d, NumofertPS :%d\n", num_of_slots, freeslots, leftOTHER_slots, conn_per_schetype[1]);

                }

                if (ulGrantSchedulingType == UL_rtPS) {
                	ServiceFlowQosSet *sfQosSet = con->get_serviceflow()->getQosSet();
                    req_slots_tmp1 = (int) ceil((double)con->getBw()/(double)mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                    int issue_pol = 0;

                    /*
                    	  if (req_slots_tmp1>0) allocationsize = (int) ceil((sfqos->getMinReservedRate()*FRAME_SIZE)/8);
                    	  else  allocationsize = GENERIC_HEADER_SIZE; 					//for explicit polling
                    */

                    int tmp_getpoll = con->getPOLL_interval();
                    if (req_slots_tmp1>0) {
                        allocationsize = ceil( double(sfQosSet->getMinReservedTrafficRate()) * double(sfQosSet->getPollingInterval()) / ( 1000 * 8 ));
                        debug10 ("\tPoll_rtPS: No polling (bw-req>0), lastpoll :%d, period :%d\n", tmp_getpoll, sfQosSet->getPollingInterval());
                        con->setPOLL_interval(0);
                    } else  {
                        if ( (tmp_getpoll % sfQosSet->getPollingInterval()) >=  ( mac_->getFrameDuration() * 1000) ) {
                            debug10 ("\tPoll_rtPS(yes): Issues unicast poll, lastpoll :%d, period :%d\n", tmp_getpoll, sfQosSet->getPollingInterval());

                            allocationsize = GENERIC_HEADER_SIZE;                                        //for explicit polling
                            con->setPOLL_interval(0);
                            issue_pol = 1;
                        } else {
                            debug10 ("\tPoll_rtPS(no): Don't issue unicast poll, lastpoll :%d, period :%d\n", tmp_getpoll, sfQosSet->getPollingInterval());
                            allocationsize = 0;
                        }
                        debug10 ("\tPoll_rtPS: Current polling_counter :%d, update_polling :%d, period :%d\n", tmp_getpoll, tmp_getpoll+1, sfQosSet->getPollingInterval());
                        tmp_getpoll += int( mac_->getFrameDuration() * 1000);
                        con->setPOLL_interval(tmp_getpoll);
                    }

                    int arq_enable_f = 0;
                    if ((con->getArqStatus () != NULL) && (con->getArqStatus ()->isArqEnabled() == 1) && (issue_pol==0) ) {
                        arq_enable_f = 1;
                    }
                    int t_bytes = 0;
                    double ori_grant = allocationsize;
                    if ( (arq_enable_f == 1) && (allocationsize > 0) ) {
                        t_bytes = (int)allocationsize % (int)getMac()->arq_block_size_;
                        if (t_bytes > 0) {
                            allocationsize = allocationsize + (getMac()->arq_block_size_ - t_bytes);
                        }
                        debug10 (" ARQ enable CID :%d, arq_block :%d, requested_size :%f, arq_block_boundary_size :%f, all_header_included :%f\n", con->get_cid(), getMac()->arq_block_size_, ori_grant, allocationsize, allocationsize + (double)14);
                        allocationsize = allocationsize + 14;   //optional => MAC+frag_sub+pack_sub+gm_sub+mesh_sub
                    }

                    grant_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                    num_of_slots = grant_slots;

#ifdef BWREQ_PATCH
                    //decrement bandwidth requested by allocation
                    debug2 ("\trtPS: CID= %d Req= %d Alloc= %d Left=%d\n",
                            con->get_cid(), con->getBw(),
                            num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_),
                            con->getBw()-(num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
                    con->setBw(con->getBw()-(num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
#endif
                    if (leftOTHER_slots<=num_of_slots) {
                        debug10 ("Fatal Error: There is not enough resource for rtPS\n");
                        exit(1);
                    } else leftOTHER_slots=leftOTHER_slots-num_of_slots;

                    if (grant_slots<req_slots_tmp1) needmore_con[0]++;

                    debug10 ("UL.Check1.3.rtPS, MinReservedRate :%d, PRE-GRANT-SLOTS :%d, Peer-CID :%d, returnCID :%d, DIUC :%d, bw_req_header :%d, getBw :%d\n", sfQosSet->getMinReservedTrafficRate(), grant_slots, con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), return_cid_tmp, con->getPeerNode()->getDIUC(), GENERIC_HEADER_SIZE, con->getBw());
                    debug10 ("\tAllocatedSLots :%d, BeforeFree :%d, LeftforOTHER :%d, NumofrtPS :%d\n", num_of_slots, freeslots, leftOTHER_slots, conn_per_schetype[2]);

                }


                if (ulGrantSchedulingType == UL_nrtPS) {
                	ServiceFlowQosSet *sfQosSet = con->get_serviceflow()->getQosSet();
                    int issue_pol = 0;
                    req_slots_tmp1 = (int) ceil((double)con->getBw()/(double)mac_->getPhy()->getSlotCapacity(mod_rate, UL_));

                    if (req_slots_tmp1>0) {
                        allocationsize = (int) ceil( double(sfQosSet->getMinReservedTrafficRate()) * mac_->getFrameDuration() / 8 );
                    } else  {
                        if (con->getCDMA() == 1) {
                            if (GENERIC_HEADER_SIZE > con->getBw()) {
                                allocationsize = GENERIC_HEADER_SIZE; 					//for explicit polling
                                issue_pol = 1;
                            }
                            con->setCDMA(0);
                        } else {
                            allocationsize = 0;
                        }
                    }

                    int arq_enable_f = 0;
                    if ((con->getArqStatus () != NULL) && (con->getArqStatus ()->isArqEnabled() == 1) && (issue_pol == 0)) {
                        arq_enable_f = 1;
                    }
                    int t_bytes = 0;
                    double ori_grant = allocationsize;
                    if ( (arq_enable_f == 1) && (allocationsize > 0) ) {
                        t_bytes = (int)allocationsize % (int)getMac()->arq_block_size_;
                        if (t_bytes > 0) {
                            allocationsize = allocationsize + (getMac()->arq_block_size_ - t_bytes);
                        }
                        debug10 (" ARQ enable CID :%d, arq_block :%d, requested_size :%f, arq_block_boundary_size :%f, all_header_included :%f\n", con->get_cid(), getMac()->arq_block_size_, ori_grant, allocationsize, allocationsize + (double)14);
                        allocationsize = allocationsize + 14;   //optional => MAC+frag_sub+pack_sub+gm_sub+mesh_sub

                    }

                    grant_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                    num_of_slots = grant_slots;

#ifdef BWREQ_PATCH
                    //decrement bandwidth requested by allocation
                    debug2 ("UL.Check1.3.nrtPS: CID= %d Req= %d Alloc= %d Left=%d\n",
                            con->get_cid(), con->getBw(),
                            num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_),
                            con->getBw()-(num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
                    con->setBw(con->getBw()-(num_of_slots*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
#endif

                    if (leftOTHER_slots<=num_of_slots) {
                        debug10 ("Fatal Error: There is not enough resource for nrtPS\n");
                        exit(1);
                    } else leftOTHER_slots=leftOTHER_slots-num_of_slots;

                    if (grant_slots<req_slots_tmp1) needmore_con[1]++;

                    debug10 ("UL.Check1.3.nrtPS, MinReservedRate :%d, PRE-GRANT-SLOTS :%d, Peer-CID :%d, returnCID :%d, DIUC :%d, getBw :%d\n", sfQosSet->getMinReservedTrafficRate(), grant_slots, con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), return_cid_tmp, con->getPeerNode()->getDIUC(), con->getBw());
                    debug10 ("\tAllocatedSLots :%d, BeforeFree :%d, LeftforOTHER :%d, NumofnrtPS :%d\n", num_of_slots, freeslots, leftOTHER_slots, conn_per_schetype[3]);

                }

                if (ulGrantSchedulingType==UL_BE) {
                    req_slots_tmp1 = (int) ceil((double)con->getBw()/(double)mac_->getPhy()->getSlotCapacity(mod_rate, UL_));

                    if (req_slots_tmp1>0) needmore_con[2]++;

                    //may effect QoS for other classes => may need more sophicicate scheduler
                    if (con!=NULL && con->getCDMA() == 1) {
                        if (GENERIC_HEADER_SIZE > con->getBw()) {
                            allocationsize = GENERIC_HEADER_SIZE; 					//for explicit polling
                            grant_slots = (int) ceil(allocationsize/mac_->getPhy()->getSlotCapacity(mod_rate, UL_));
                            num_of_slots = grant_slots;
                        }
                        con->setCDMA(0);

                        if (leftOTHER_slots<=num_of_slots) {
                            debug10 ("Fatal Error: There is not enough resource for BE polling\n");
                            // exit(1);
                            num_of_slots = leftOTHER_slots;
                        } else leftOTHER_slots=leftOTHER_slots-num_of_slots;

                        debug10 ("UL.Check1.3.BE polling, PRE-GRANT-SLOTS :%d, Peer-CID :%d, returnCID :%d, DIUC :%d, bw_req_header :%d, setCDMA :%d\n", grant_slots, con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), return_cid_tmp, con->getPeerNode()->getDIUC(), GENERIC_HEADER_SIZE, con->getCDMA());
                        debug10 ("\tAllocatedSLots :%d, BeforeFree :%d, LeftforOTHER :%d, NumofBE :%d\n", num_of_slots, freeslots, leftOTHER_slots, conn_per_schetype[4]);

                    }

                }//end BE

                freeslots = leftOTHER_slots;

                if (num_of_slots> 0) {
                    temp_index = doesMapExist(con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), cid_list, ie_index);
//          temp_index = doesMapExist(con->get_cid(), cid_list, ie_index); //conn, ie list, number of ies

                    if (temp_index < 0) {
                        temp_index = ie_index;
//	    cid_list[temp_index] = con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid();
                        cid_list[temp_index] = con->get_cid();

                        slots_per_con[temp_index] = num_of_slots;
                        uiuc_list[temp_index] = getUIUCProfile(mod_rate);
                        ++ie_index;

                        debug10 ("UL_Check1.3: MapNotExist New_CID :%d, return index :-1, cid_list[%d] :%d,  #entry (ie_index)++ :%d\n", con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), temp_index, cid_list[temp_index], ie_index);
                        debug10 ("\tNumSlots1.2 :%d\n", num_of_slots);

                    } else if (temp_index<MAX_MAP_IE) {
                        slots_per_con[temp_index] += num_of_slots;

                        debug10 ("UL.Check1.3: MapExist Peer_CID :%d, return index :%d, cid_list[%d] :%d,  #entry (ie_index) :%d\n", con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), temp_index, temp_index, cid_list[temp_index], ie_index);
                        debug10 ("\tNumSlots1.2 :%d\n", num_of_slots);

                    } else {
                        freeslots += num_of_slots; 			//return back the slots
                        leftOTHER_slots += num_of_slots; 		//return back the slots
                    }

                    debug10 ("UL.Check1.3, First Assign (ugs/ertps/rtps/nrtps/no be): CID(%d), Schetype :%d, Numslots :%d, Freeleft :%d, StoreSlots[%d] :%d, mod_rate :%d, UIUC :%d\n", cid_list[temp_index], ulGrantSchedulingType, num_of_slots, freeslots, temp_index, slots_per_con[temp_index], mod_rate, uiuc_list[temp_index]);
                }



            }
            con = con->next_entry();
        }//end con != NULL
    }//end for loop 5 Qos


    //Allocate left-over to rtPS, nrtPS, and BE fairly
    int share_next_slots = 0;
    for (int i=0; i<3; i++) needmore_c = needmore_c+needmore_con[i];
    if (needmore_c>0) share_next_slots = (int) floor(leftOTHER_slots/needmore_c);

    int first_assign = 0;

    while (needmore_c>0 && freeslots>0) {

        share_next_slots = (int) floor(freeslots/needmore_c);

        debug10 ("UL.Check1.4, (Check still need more here): Needmore Conn :%d, Free :%d, Sharenext :%d\n", needmore_c, freeslots, share_next_slots
                );

        for (i=0; i<5; ++i) {
            con = head;
            if (i==0)        ulGrantSchedulingType = UL_UGS;
            else if (i==1)   ulGrantSchedulingType = UL_ertPS;
            else if (i==2)   ulGrantSchedulingType = UL_rtPS;
            else if (i==3)   ulGrantSchedulingType = UL_nrtPS;
            else            ulGrantSchedulingType = UL_BE;

            if ( (i==0) || (i==1) ) continue;

            first_assign = 0;
            while (con!=NULL) {

                if (con->get_category() == CONN_DATA && con->get_serviceflow()->getQosSet()->getUlGrantSchedulingType() == ulGrantSchedulingType) {

                    //mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getDIUC()-DIUC_PROFILE_1+UIUC_PROFILE_1)->getEncoding();
                    mod_rate = mac_->getMap()->getUlSubframe()->getProfile(con->getPeerNode()->getUIUC())->getEncoding();
                    /*  if (mod_rate > OFDM_16QAM_3_4) {
                        mod_rate = OFDM_16QAM_3_4;
                      }*/

                    temp_index = doesMapExist(con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), cid_list, ie_index);
//          temp_index = doesMapExist(con->get_cid(), cid_list, ie_index); //conn, ie list, number of ies

                    if (temp_index < 0) return_cid_tmp = -1;
                    else return_cid_tmp = temp_index;

                    int req_slots = 0;
                    req_slots = (int) ceil((double)con->getBw()/(double)mac_->getPhy()->getSlotCapacity(mod_rate, UL_));

                    if ( (ulGrantSchedulingType==UL_rtPS) || (ulGrantSchedulingType==UL_nrtPS) ) {
                        if (return_cid_tmp == -1) {
                            debug10 ("UL.Check1.4.rtps/nrtps, No_CID(%d), n_Conn :%d, Free :%d\n", return_cid_tmp, needmore_c, freeslots);
                            con = con->next_entry();
                            continue;
                        }
                        if ( req_slots<=slots_per_con[return_cid_tmp] ) {
                            debug10 ("UL.Check1.4.rtPS/nrtPS, CID(%d), <=MinSlots: n_Conn :%d, REQ-SLOTS :%d, GRANT-1SLOTS :%d, Free :%d\n", cid_list[return_cid_tmp], needmore_c, req_slots, slots_per_con[return_cid_tmp], freeslots);
                            con = con->next_entry();
                            continue;
                        }
                    }

                    int t_num_of_slots = 0;

                    if ( (ulGrantSchedulingType==UL_rtPS) || (ulGrantSchedulingType==UL_nrtPS) ) {
                        first_assign = slots_per_con[return_cid_tmp];

                        if (req_slots <= (first_assign + share_next_slots) ) {
                            t_num_of_slots = req_slots;
                            slots_per_con[return_cid_tmp] = t_num_of_slots;
                            needmore_c--;
                            freeslots = freeslots - (req_slots-first_assign);
                            debug10 ("UL.Check1.4.rtps/nrtps, CID(%d), rtps/nrtpsNoNeedMore: n_Conn :%d, Pre-Grant-Slots :%d, Free :%d\n", cid_list[return_cid_tmp], needmore_c, t_num_of_slots, freeslots);
                        } else {
                            if (share_next_slots==0) {
                                if (freeslots>0) {
                                    t_num_of_slots = (first_assign + 1);
                                    freeslots = freeslots - 1;
                                } else t_num_of_slots = first_assign;
                            } else {
                                t_num_of_slots =  (first_assign + share_next_slots);
                                freeslots = freeslots - share_next_slots;
                            }
                            slots_per_con[return_cid_tmp] = t_num_of_slots;
                        }
                        debug10 ("UL.Check1.4.rtps/nrtps, CID(%d), rtps/nrtpsNeedMore: n_Conn :%d, Previous-Slots :%d, Grant-Slots :%d, Free :%d\n", cid_list[return_cid_tmp], needmore_c, first_assign, t_num_of_slots, freeslots);
                    } else { //BE
                        if (req_slots<1) {
                            con = con->next_entry();
                            continue;
                        }
                        temp_index = doesMapExist(con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), cid_list, ie_index);
//            temp_index = doesMapExist(con->get_cid(), cid_list, ie_index); //conn, ie list, number of ies

                        if (temp_index < 0) {
                            temp_index = ie_index;
                            cid_list[temp_index] = con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid();
//              cid_list[temp_index] = con->get_cid();

                            slots_per_con[temp_index] = 0;
                            uiuc_list[temp_index] = getUIUCProfile(mod_rate);
                            ++ie_index;
                            debug10 ("UL.Check1.4.BE, CID(%d), Initial_BE: n_Conn :%d, REQ-SLOTS :%d, Free :%d, getBW :%d\n", cid_list[temp_index], needmore_c, req_slots, freeslots, con->getBw());

                        } else {
                        }

                        first_assign = slots_per_con[temp_index];
                        int bw_assigned = 0; //amount we are assigning in the round
#ifdef BWREQ_PATCH
                        //req_slots is updated each round since we update remain Bw to allocate
                        if (req_slots <= share_next_slots ) {
                            bw_assigned = req_slots;
                            t_num_of_slots = req_slots;
                            slots_per_con[temp_index] += req_slots;
                            needmore_c--;
                            freeslots = freeslots - req_slots;
#else
                        if (req_slots <= (first_assign + share_next_slots) ) {
                            bw_assigned = req_slots;
                            t_num_of_slots = req_slots;
                            slots_per_con[temp_index] = t_num_of_slots;
                            needmore_c--;
                            freeslots = freeslots - (req_slots-first_assign);
#endif

                            debug10 ("UL.Check1.4.BE, Peer-CID(%d), returnCID(%d), BENoNeedMore: n_Conn :%d, Pre-Grant-Slots :%d, Free :%d\n", con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), cid_list[temp_index], needmore_c, t_num_of_slots, freeslots);
                        } else {
                            if (share_next_slots==0) {
                                //there is less slot than connections left..assign slot 1 by 1.
                                if (freeslots>0) {
                                    bw_assigned = 1;
                                    t_num_of_slots = (first_assign + 1); //assign one more slot
                                    freeslots = freeslots - 1;
                                } else t_num_of_slots = first_assign;
                            } else {
                                bw_assigned = share_next_slots;
                                t_num_of_slots =  (first_assign + share_next_slots); //assign its share of slots
                                freeslots = freeslots - share_next_slots;
                            }
                            slots_per_con[temp_index] = t_num_of_slots;

                            debug10 ("UL.Check1.4.BE, Peer-CID(%d), returnCID(%d), BENeedMore: n_Conn :%d, Pre-Grant-Slots :%d, Free :%d\n", con->getPeerNode()->getBasic(OUT_CONNECTION)->get_cid(), cid_list[temp_index], needmore_c, t_num_of_slots, freeslots);
                        }

#ifdef BWREQ_PATCH
                        //update Bw requested left after allocation
                        debug2 ("\tBE: CID= %d Req= %d Slot cap=%d slots= %d Alloc= %d Left=%d\n",
                                con->get_cid(), con->getBw(), mac_->getPhy()->getSlotCapacity(mod_rate, UL_), req_slots,
                                bw_assigned*mac_->getPhy()->getSlotCapacity(mod_rate, UL_),
                                con->getBw()-(bw_assigned*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
                        con->setBw(con->getBw()-(bw_assigned*mac_->getPhy()->getSlotCapacity(mod_rate, UL_)));
#endif

                    }	//else BE

                }//else CONNDATA
                con = con->next_entry();
            }//while
        }//for

    }//while

    if (stripping == HORIZONTAL_STRIPPING) {
        for (i=0; i<ie_index; ++i) {
            ies[i].cid = cid_list[i];
            num_of_slots = slots_per_con[i];
            ies[i].uiuc = uiuc_list[i];
            num_of_subchannels = num_of_slots;

            ies[i].subchannel_offset = subchannel_offset + subchannel_start;
            ies[i].symbol_offset = symbol_offset + symbol_start;
            ies[i].num_of_subchannels = num_of_subchannels;
            if (total_symbols>0) num_of_symbols = (int) ceil( (double)(symbol_offset + num_of_slots) / (total_symbols) );
            else num_of_symbols = 0;
            ies[i].num_of_symbols = num_of_symbols;

            if (cdma_flag_list[i]>0) {
                ies[i].cdma_ie.subchannel = cdma_top_list[i];
                ies[i].cdma_ie.code = cdma_code_list[i];
            } else {
                ies[i].cdma_ie.subchannel = 0;
                ies[i].cdma_ie.code = 0;
            }

            debug10 ("UL.Check1.5: ie_index(MAX=60) :%d, ies[%d].cid :%d, #Slots :%d, UIUC :%d\n", ie_index, i, ies[i].cid, num_of_slots, ies[i].uiuc);
            debug10 ("\tIE_Subchannel_offset (sub_offset<%d>+sub_start<%d>) :%d, #IE_Subchannel :%d\n", subchannel_offset, subchannel_start, ies[i].subchannel_offset, ies[i].num_of_subchannels);
            debug10 ("\tIE_Symbol_offset (sym_offset<%d>+sym_start<%d>) :%d, #Symbols [ceil((sub_offset<%d>+#Subchannel<%d>)/total_sub<%d>)*3] :%d\n", symbol_offset, symbol_start, ies[i].symbol_offset, subchannel_offset, num_of_subchannels, total_subchannels, ies[i].num_of_symbols);

            subchannel_offset += num_of_symbols;
            if (total_symbols>0) symbol_offset = (symbol_offset + num_of_slots) % (total_symbols);
            else symbol_offset = 0;

            /*
                  subchannel_offset += num_of_subchannels - 1;
                  symbol_offset = (symbol_offset + num_of_symbols)%(total_symbols);
            */
        }
    }

    if (stripping == VERTICAL_STRIPPING) {
        for (i=0; i<ie_index; ++i) {
            ies[i].cid = cid_list[i];
            num_of_slots = slots_per_con[i];
            ies[i].uiuc = uiuc_list[i];
            ies[i].subchannel_offset = subchannel_offset + subchannel_start;
            ies[i].symbol_offset = symbol_offset + symbol_start;
            num_of_subchannels = num_of_slots;
            num_of_symbols = (int) ceil((double)(subchannel_offset + num_of_subchannels)/total_subchannels)*3;
            ies[i].num_of_symbols = num_of_symbols;
            ies[i].num_of_subchannels = num_of_subchannels;

            if (cdma_flag_list[i]>0) {
                ies[i].cdma_ie.subchannel = cdma_top_list[i];
                ies[i].cdma_ie.code = cdma_code_list[i];
            } else {
                ies[i].cdma_ie.subchannel = 0;
                ies[i].cdma_ie.code = 0;
            }

            debug10 ("UL.Check1.5: ie_index(MAX=60) :%d, ies[%d].cid :%d, #Slots :%d, UIUC :%d\n", ie_index, i, ies[i].cid, num_of_slots, ies[i].uiuc);
            debug10 ("\tIE_Subchannel_offset (sub_offset<%d>+sub_start<%d>) :%d, #IE_Subchannel :%d\n", subchannel_offset, subchannel_start, ies[i].subchannel_offset, ies[i].num_of_subchannels);
            debug10 ("\tIE_Symbol_offset (sym_offset<%d>+sym_start<%d>) :%d, #Symbols [ceil((sub_offset<%d>+#Subchannel<%d>)/total_sub<%d>)*3] :%d\n", symbol_offset, symbol_start, ies[i].symbol_offset, subchannel_offset, num_of_subchannels, total_subchannels, ies[i].num_of_symbols);

            subchannel_offset = (subchannel_offset + num_of_subchannels)%(total_subchannels);
            symbol_offset += num_of_symbols - 3;

        }
    }

    ul_map->nb_ies = ie_index;
    return ul_map;
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
