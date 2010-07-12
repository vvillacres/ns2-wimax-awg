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

#ifndef MAC802_16BS_H
#define MAC802_16BS_H

#include "mac802_16.h"
#include "scheduling/wimaxctrlagent.h"
#include "scheduling/scanningstation.h"

//Some configuration elements to enable/disable features under development
#define BWREQ_PATCH

/** Information about a new client */
struct new_client_t {
    int cid; //primary cid of new client
    new_client_t *next;
};

class T17Element;
LIST_HEAD (t17element, T17Element);
/** Object to handle timer t17 */
class T17Element
{
public:
    T17Element (Mac802_16 *mac, int index) {
        index_ = index;
        timer = new WimaxT17Timer (mac, index);
        timer->start (mac->macmib_.t17_timeout);
    }

    ~T17Element () {
        delete (timer);
    }

    int index () {
        return index_;
    }
    int paused () {
        return timer->paused();
    }
    int busy () {
        return timer->busy();
    }
    void cancel () {
        timer->stop();
    }

    // Chain element to the list
    inline void insert_entry(struct t17element *head) {
        LIST_INSERT_HEAD(head, this, link);
    }

    // Return next element in the chained list
    T17Element* next_entry(void) const {
        return link.le_next;
    }

    // Remove the entry from the list
    inline void remove_entry() {
        LIST_REMOVE(this, link);
    }
protected:

    /*
     * Pointer to next in the list
     */
    LIST_ENTRY(T17Element) link;
    //LIST_ENTRY(T17Element); //for magic draw

private:
    int index_;
    WimaxT17Timer *timer;
};

class FastRangingInfo;
LIST_HEAD (fastRangingInfo, FastRangingInfo);
/** Store information about a fast ranging request to send */
class FastRangingInfo
{
public:
    FastRangingInfo (int frame, int macAddr) {
        frame_ = frame;
        macAddr_ = macAddr;
    }

    int frame () {
        return frame_;
    }
    int macAddr () {
        return macAddr_;
    }

    // Chain element to the list
    inline void insert_entry(struct fastRangingInfo *head) {
        LIST_INSERT_HEAD(head, this, link);
    }

    // Return next element in the chained list
    FastRangingInfo* next_entry(void) const {
        return link.le_next;
    }

    // Remove the entry from the list
    inline void remove_entry() {
        LIST_REMOVE(this, link);
    }
protected:

    /*
     * Pointer to next in the list
     */
    LIST_ENTRY(FastRangingInfo) link;
    //LIST_ENTRY(FastRangingInfo); //for magic draw

private:
    int frame_;
    int macAddr_;
};


/**
 * Class implementing IEEE 802_16 State machine at the BS
 */
class Mac802_16BS : public Mac802_16
{
    friend class WimaxCtrlAgent;
    friend class BSScheduler;
public:

    Mac802_16BS();

    /**
     * Interface with the TCL script
     * @param argc The number of parameter
     * @param argv The list of parameters
     */
    int command(int argc, const char*const* argv);

    /**
     * Process packets going out
     * @param p The packet to transmit
     */
    void sendDown(Packet *p);

    /**
     * Process packets going out
     * @param p The packet to transmit
     */
    void transmit(Packet *p);

    /**
     * Process incoming packets
     * @param p The received packet
     */
    void sendUp(Packet *p);

    /**
     *sam intpower_
     */
    //double intpower_[1024];

    /**
     * Process the packet after receiving last bit
     */
    //void receive();

    // fille the frame with power values
    void addPowerinfo(hdr_mac802_16 *wimaxHdr,double power, bool collision );

    //chk for collision
    bool IsCollision (const hdr_mac802_16 *wimaxHdr,double power_subchannel);


    /**
     * Process the packet after receiving last bit
     *@param p - the packet to be received  RPI
     */
    void receive(Packet *p);

#ifdef USE_802_21 //Switch to activate when using 802.21 modules (external package)
    /*
     * Configure/Request configuration
     * The upper layer sends a config object with the required
     * new values for the parameters (or PARAMETER_UNKNOWN_VALUE).
     * The MAC tries to set the values and return the new setting.
     * For examples if a MAC does not support a parameter it will
     * return  PARAMETER_UNKNOWN_VALUE
     * @param config The configuration object
     */
    void link_configure (link_parameter_config_t* config);

#endif

protected:

    /**
     * init the timers and state
     */
    void init ();

    void ffb_process() {};


    /**
     * Initialize default connection
     */
    void init_default_connections ();

    /**
     * Update the given timer and check if thresholds are crossed
     * @param watch the stat watch to update
     * @param value the stat value
     */
    void update_watch (StatWatch *watch, double value);

    /**
     * Update the given timer and check if thresholds are crossed
     * @param watch the stat watch to update
     * @param size the size of packet received
     */
    void update_throughput (ThroughputWatch *watch, double size);

#ifdef USE_802_21 //Switch to activate when using 802.21 modules (external package)
    /**
     * Poll the given stat variable to check status
     * @param type The link parameter type
     */
    void poll_stat (link_parameter_type_s type);
#endif

    /**
     * Called when a timer expires
     * @param The timer ID
     */
    virtual void expire (timer_id id);

    /**
     * Start a new DL subframe
     */
    virtual void start_dlsubframe ();

    /**
      * Process CQICH Allocation Request from SS.
      */
    void process_cqich_alloc_req(Packet *p);

    /*Indicator to show if there is a cqich alloc ie.*/
    bool bs_cqich_alloc_ie_indicator_;

    virtual void set_cqich_alloc_ie_ind(bool indicator) {
        printf("BS set CQICH_ALLOC_IE Indicator flag to %d\n",indicator);
        bs_cqich_alloc_ie_indicator_ = indicator;
    }
    virtual  bool get_cqich_alloc_ie_ind() {
        return bs_cqich_alloc_ie_indicator_;
    }


    /*xingting set get modulation configure*/
    virtual bool get_registered_flag(int mac_index)  {
        return ss_modulation_cng_record[mac_index].hadRegistered;
    }
    virtual void set_registered_flag(int mac_index, bool reg)   {
        ss_modulation_cng_record[mac_index].hadRegistered = reg;
    }
    virtual bool get_change_modulation_flag(int mac_index)   {
        return ss_modulation_cng_record[mac_index].changeModulation;
    }
    virtual void set_change_modulation_flag(int mac_index, bool change)  {
        ss_modulation_cng_record[mac_index].changeModulation = change;
    }
    virtual bool get_increase_modulation(int mac_index)  {
        return ss_modulation_cng_record[mac_index].increaseModulation;
    }
    virtual void set_increase_modulation(int mac_index, bool change)  {
        ss_modulation_cng_record[mac_index].increaseModulation = change;
    }
    virtual void set_current_mcs_index(int mac_index, int mcs_index) {
        ss_modulation_cng_record[mac_index].current_mcs_index = mcs_index;
    }
    virtual int get_current_mcs_index(int mac_index) {
        return ss_modulation_cng_record[mac_index].current_mcs_index;
    }
    virtual void set_current_bler(int mac_index, int bler) {
        ss_modulation_cng_record[mac_index].current_bler= bler;
    }
    //float get_current_bler(int mac_index) {return ss_modulation_cng_record[mac_index].current_bler;}


    virtual bool get_change_ul_modulation_flag(int mac_index)   {
        return ss_modulation_cng_record[mac_index].changeULModulation;
    }
    virtual void set_change_ul_modulation_flag(int mac_index, bool change)  {
        ss_modulation_cng_record[mac_index].changeULModulation = change;
    }
    virtual bool get_increase_ul_modulation(int mac_index)  {
        return ss_modulation_cng_record[mac_index].increaseULModulation;
    }
    virtual void set_increase_ul_modulation(int mac_index, bool change)  {
        ss_modulation_cng_record[mac_index].increaseULModulation = change;
    }
    virtual void set_current_ul_mcs_index(int mac_index, int mcs_index) {
        ss_modulation_cng_record[mac_index].current_ul_mcs_index = mcs_index;
    }
    virtual int get_current_ul_mcs_index(int mac_index) {
        return ss_modulation_cng_record[mac_index].current_ul_mcs_index;
    }
    virtual void set_current_ul_bler(int mac_index, int bler) {
        ss_modulation_cng_record[mac_index].current_ul_bler= bler;
    }
    virtual float get_current_ul_bler(int mac_index) {
        return ss_modulation_cng_record[mac_index].current_ul_bler;
    }

    int get_smooth_factor(int mac_index) {
        return ss_modulation_cng_record[mac_index].smooth_dl_factor;
    }
    void set_smooth_factor(int mac_index, int factor) {
        ss_modulation_cng_record[mac_index].smooth_dl_factor = factor;
    }

    void UL_AMC_algorithm(float BLER, int mac_index);
    void AMC_algorithm(Packet * p, struct mac_hdr_ffb_report * ffb_report);
    void process_ffb_report ( Packet * p);

    void set_channel_error(bool error_flag) {
        channel_error_rate_ = error_flag;
    }
    bool get_channel_error() {
        return channel_error_rate_;
    }

    void set_anti_pingpong_factor(int factor=3) {
        anti_pingpong_factor_ = factor;
    }
    int get_anti_pingpong_factor() {
        return anti_pingpong_factor_;
    }

    int get_ul_smooth_factor(int mac_index) {
        return ss_modulation_cng_record[mac_index].smooth_ul_factor;
    }
    void set_ul_smooth_factor(int mac_index, int factor) {
        ss_modulation_cng_record[mac_index].smooth_ul_factor = factor;
    }

    int cqich_slot_allocation_offset_;

    void cqich_id_offset_allocation(int ss_mac_id, int & alloc_offset, int & cqich_id);
    int lookup_ssid_via_cqich_id(int cqich_id);

    /**
     * Process cdma bandwidth request
     * @param p The request
     */
    void process_cdma_req (Packet *p);

    /**
     * Start a new UL subframe
     */
    virtual void start_ulsubframe ();

    /**
     * Finds out if the given station is currently scanning
     * @param nodeid The MS id
     */
    bool isPeerScanning (int nodeid);

    /**
     * Set the control agent
     * @param agent The control agent
     */
    void setCtrlAgent (WimaxCtrlAgent *agent);

    /** Add a new Fast Ranging allocation
     * @param time The time when to allocate data
     * @param macAddr The MN address
     */
    void addNewFastRanging (double time, int macAddr);

    /**
     * Send a scan response to the MN
     * @param rsp The response from the control
     * @cid The CID for the MN
     */
    void send_scan_response (mac802_16_mob_scn_rsp_frame *rsp, int cid);

    /**
     * Indicates if it is time to send a DCD message
     */
    bool sendDCD;

    /**
     * DL configuration change count
     */
    int dlccc_;

    /**
     * Indicates if it is time to send a UCD message
     */
    bool sendUCD;

    /**
     * UL configuration change count
     */
    int ulccc_;

private:

    /*store the status inform for each of SS about AMC.*/
    struct modulation_cng_record ss_modulation_cng_record[MAX_NUM_SS_IN_ONE_CELL];

    /*This variable is used to save the actual channel error status. Although the SINR might be very
       high, but the interference is also high and this packet might be corrupted. On the contary, the
       SINR might be low, the interference is low too and the packet might be successfully transmitted.*/
    bool channel_error_rate_;

    /*This variable is used to save anti_pingpong factor for AMC algorithm.*/
    int anti_pingpong_factor_;


    //Begin RPI
    /**
     * The function is used to process the MAC PDU when ARQ,Fragmentation and Packing are enabled
     * @param con The connection by which it arrived
     * @param p The packet to process
     */
    void process_mac_pdu_witharqfragpack (Connection *con, Packet *p);
    //End RPI

    /**
     * Process a MAC type packet
     * @param con The connection by which it arrived
     * @param p The packet to process
     */
    void process_mac_packet (Connection *con, Packet *p);

    /**
     * Process a RNG-REQ message
     * @param frame The ranging request information
     */
    void process_ranging_req (Packet *p);

    /**
     * Process bandwidth request
     * @param p The request
     */
    void process_bw_req (Packet *p);

    /**
     * Process bandwidth request
     * @param p The request
     */
    void process_reg_req (Packet *p);

    /**
     * Process handover request
     * @param req The request
     */
    void process_msho_req (Packet *req);

    /**
     * Process handover indication
     * @param p The indication
     */
    void process_ho_ind (Packet *p);

    /**
     * Send a neighbor advertisement message
     */
    void send_nbr_adv ();

    /**
     * Add a new timer 17 for client
     * @param index The client address
     */
    void addtimer17 (int index);

    /**
     * Cancel and remove the timer17 associated with the node
     * @param index The client address
     */
    void removetimer17 (int index);


    /**
     * Pointer to the head of the list of nodes that should
     * perform registration
     */
    struct t17element t17_head_;

    /**
     * Pointer to the head of the list of clients
     */
    struct new_client_t *cl_head_;

    /**
     * Pointer to the tail of the list of clients
     */
    struct new_client_t *cl_tail_;

    /**
     * The index of the last SS that had bandwidth allocation
     */
    int bw_node_index_;

    /**
     * The node that had the last bandwidth allocation
     */
    PeerNode *bw_peer_;

    /**
     * Timer for DCD
     */
    WimaxDCDTimer *dcdtimer_;

    /**
     * Timer for UCD
     */
    WimaxUCDTimer *ucdtimer_;

    /**
     * Timer for MOB-NBR_ADV messages
     */
    WimaxMobNbrAdvTimer *nbradvtimer_;

    /**
     * List of station in scanning
     */
    struct scanningStation scan_stations_;

    /**
     * The Wimax control for BS synchronization
     */
    WimaxCtrlAgent *ctrlagent_;

    /**
     * List of the upcoming Fast Ranging allocation
     */
    struct fastRangingInfo fast_ranging_head_;

};

#endif //MAC802_16BS_H

