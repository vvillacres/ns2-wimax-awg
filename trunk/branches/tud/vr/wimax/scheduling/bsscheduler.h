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

#ifndef BSSCHEDULER_H
#define BSSCHEDULER_H

#include "wimaxscheduler.h"
#include "scanningstation.h"
#include "trafficshapinginterface.h"
#include "virtualallocation.h"
#include "schedulingalgointerface.h"




#define NUM_REALIZATIONS1 2000
#define OTHER_NUM_DL_BURST 2 //including 1st DL burst and END_OF_MAP burst.
#define OTHER_NUM_UL_BURST 3 //including initial ranging, bw req and END_OF_MAP burst

class Mac802_16BS;
class WimaxCtrlAgent;
class TrafficPolicingInterface;
class SchedulingAlgoInterface;
class VirtualAllocation;

/**
 * Class BSScheduler
 * Implement the packet scheduler on the BS side
 */
class BSScheduler : public WimaxScheduler
{



    //friend class SendTimer;
public:
    /*
     * Create a scheduler
     */
    BSScheduler ();

    /*
     * Delete a scheduler
     */
    ~BSScheduler ();

    /*
     * Interface with the TCL script
     * @param argc The number of parameter
     * @param argv The list of parameters
     */
    int command(int argc, const char*const* argv);

    /**
     * Initializes the scheduler
     */
    virtual void init ();

    /**
     * This function is used to schedule bursts/packets
     */
    virtual void schedule ();

    Ofdm_mod_rate change_rate(Ofdm_mod_rate rate, bool increase_modulation);
    bool check_modulation_change(Ofdm_mod_rate current_rate, int current_mcs_index, bool increase);
    int update_mcs_index(Ofdm_mod_rate current_rate, int current_mcs_index, bool increase);

    /**
     * Returns the statistic for the downlink scheduling
     */
    frameUsageStat_t getDownlinkStatistic();

    /*
     * Returns the statistic for the uplink scheduling
     */
    frameUsageStat_t getUplinkStatistic();

protected:


    /**
     * Compute and return the bandwidth request opportunity size
     * @return The bandwidth request opportunity size
     */
    int getBWopportunity ();

    /**
     * Compute and return the initial ranging opportunity size
     * @return The initial ranging opportunity size
     */
    int getInitRangingopportunity ();

    /**
     * Add a downlink burst with the given information
     * @param burstid The burst number
     * @param c The connection to add
     * @param iuc The profile to use
     * @param dlduration current allocation status
     * @param the new allocation status
     */
    int addDlBurst (int burstid, Connection * , int iuc, int dlduration, int maxdlduration);



//added rpi for OFDMA -------
    /**
     * Add a downlink burst with the given information
     * @param burstid The burst number
     * @param c The connection to add
     * @param iuc The profile to use
     * @param ofdmsymboloffset
     * @param numofdmsymbol
     * @param subchanneloffset
     * @param numsubchannels
     */
    void addDlBurst (int burstid, int cid, int iuc, int ofdmsymboloffset, int numofdmsymbols, int subchanneloffset, int numsubchannels);

    /* returns the DIUC profile associated with a current modulation and coding scheme */
    diuc_t getDIUCProfile(Ofdm_mod_rate rate);

    uiuc_t getUIUCProfile(Ofdm_mod_rate rate);


    mac802_16_dl_map_frame * buildDownlinkMap( VirtualAllocation * virtualAlloc, Connection *head, int totalDlSubchannels, int totalDlSymbols, int dlSymbolOffset, int dlSubchannelOffset, int freeDlSlots);


    mac802_16_ul_map_frame * buildUplinkMap( Connection *head, int totalUlSubchannels, int totalUlSymbols, int ulSymbolOffset, int ulSubchannelOffset, int freeUlSlots);


private:

    /**
     * Return the MAC casted to BSScheduler
     * @return The MAC casted to BSScheduler
     */
    Mac802_16BS* getMac();

    /**
     * The ratio for downlink subframe
     */
//  double dlratio_;   rpi removed , moved to wimax scheudler

    /**
     * The address of the next node for DL allocation
     */
    int nextDL_;

    /**
     * The address of the next node for UL allocation
     */
    int nextUL_;


    /**
     * Default modulation
     */
    Ofdm_mod_rate default_mod_;

    /**
     * Number of transmission opportunity for initial ranging
     * and bw request (i.e contention slots)
     */
    int init_contention_size_;
    int bw_req_contention_size_;

    /**
     * Repetition code for dl-broadcast
     */
    int repetition_code_;


    int channel_gain[NUM_REALIZATIONS1] ;

    //sam

    /**
     * pointer to the current traffic policing algorithm
     */
    TrafficShapingInterface* trafficShapingAlgorithm_;

    /**
     * pointer to the current scheduling algorithm for the downlink direction
     */
    SchedulingAlgoInterface* dlSchedulingAlgorithm_;

    /**
     * pinter to the current scheduling algorithm for the uplink direction
     */
    SchedulingAlgoInterface* ulSchedulingAlgorithm_;



};

#endif //BSSCHEDULER_H

