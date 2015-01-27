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
 * @modified by Chakchai So-In
 * @modified by Xingting Guo
 */

#include "wimaxscheduler.h"
#define HDR_PIG 2
#define BW_REQ_TIMEOUT 2
#define INIT_REQ_TIMEOUT 2
#define UL_HORIZONTAL 1
//#define IF_PIG

/*
 * Create a scheduler
 * @param mac The Mac where it is located
 */
WimaxScheduler::WimaxScheduler ()
{
    bind ("dlratio_", &dlratio_);
    mac_ = NULL;
}

/*
 * Set the mac
 * @param mac The Mac where it is located
 */
void WimaxScheduler::setMac (Mac802_16 *mac)
{
    assert (mac!=NULL);

    mac_ = mac;
}

/**
 * Initialize the scheduler.
 */
void WimaxScheduler::init()
{
    // initializing dl and ul duration  --rpi

	/*
	OFDMAPhy *phy = mac_->getPhy();

    int nbPS = (int) floor((mac_->getFrameDuration()/phy->getPS()));
#ifdef DEBUG_WIMAX
    assert (nbPS*phy->getPS()<=mac_->getFrameDuration()); //check for rounding errors
#endif
    int nbPS_left = nbPS - mac_->phymib_.rtg - mac_->phymib_.ttg;
    int nbSymbols = (int) floor((phy->getPS()*nbPS_left)/phy->getSymbolTime());  // max num of OFDM symbols available per frame.
    assert (nbSymbols*phy->getSymbolTime()+(mac_->phymib_.rtg + mac_->phymib_.ttg)*phy->getPS() <= mac_->getFrameDuration());
    int maxdlduration = (int) (nbSymbols / (1.0/dlratio_)); //number of symbols for downlink
    int maxulduration = nbSymbols - maxdlduration;            //number of symbols for uplink

    mac_->setMaxDlduration (maxdlduration);
    mac_->setMaxUlduration (maxulduration);

    debug2(" in wimax scheduler maxdlduration = %d maxulduration =%d mac =%d \n", maxdlduration, maxulduration, mac_->addr());

    */
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
    debug2("Maximum Number of OFDM Symbols in DL Frame : %d Useful Number of Symbols in DL Frame : %d \n", maxNbDlSymbols, useNbDlSymbols);

    // Useful number of OFDMA symbols per uplink frame for PUSC  ofdm = 3 * slots
    int useNbUlSymbols = int( floor (maxNbUlSymbols / phy->getSlotLength(UL_)) * phy->getSlotLength(UL_));
    debug2("Maximum Number of OFDM Symbols in UL Frame : %d Useful Number of Symbols in UL Frame : %d \n", maxNbUlSymbols, useNbUlSymbols);

    mac_->setMaxDlduration (useNbDlSymbols - DL_PREAMBLE); // dl duration left after preamble for dl timer
    mac_->setMaxUlduration (useNbUlSymbols);

}

/**
 * This function is used to schedule bursts/packets
 */
void WimaxScheduler::schedule ()
{
    //defined by subclasses
}


/**
 * Transfer the packets from the given connection to the given burst
 * @param con The connection
 * @param b The burst
 * @param b_data Amount of data in the burst
 * @return the new burst occupation
 */
int WimaxScheduler::transfer_packets (Connection *c, Burst *b, int b_data, int subchannel_offset, int symbol_offset)
{
    Packet *p;
    hdr_cmn* ch;
    hdr_mac802_16 *wimaxHdr;
    double txtime, txtime2, txtime3;
    int txtime_s;
    bool pkt_transfered = false;
    OFDMAPhy *phy = mac_->getPhy();
    //int offset = b->getStarttime( );
    PeerNode *peer;

    //  debug2 (" entering transfer packets");// , bdata = %d  mac_ = %d \n ",b_data, mac_);
    debug10 ("\tEntered transfer_packets MacADDR :%d, with bdata  :%d\n ", mac_->addr(), b_data);
    peer = mac_->getPeerNode_head();

    p = c->get_queue()->head();

    int max_data;
    if (mac_->getNodeType()==STA_BS) {
        max_data = phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding())-b_data;

        debug10 ("\tBS_transfer_packets, CID :%d with bdata :%d, but MaxData (MaxSize-bdata) :%d, MaxSize :%d\n", c->get_cid(), b_data, max_data, phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding()) );
        debug10 ("\t                     Bduration :%d, DIUC :%d\n",b->getDuration(), b->getIUC());

    } else {
        max_data = phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding())-b_data;

        debug10 ("\tSS_transfer_packets, CID :%d with bdata :%d, but MaxData (MaxSize-bdata) :%d, MaxSize :%d\n", c->get_cid(), b_data, max_data, phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding()) );
        debug10 ("\t                     Bduration :%d, UIUC :%d\n",b->getDuration(), b->getIUC());
    }

    if (max_data < HDR_MAC802_16_SIZE ||
            (c->getFragmentationStatus()!=FRAG_NOFRAG && max_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE))
        return b_data; //not even space for header

    while (p) {
        ch = HDR_CMN(p);
        wimaxHdr = HDR_MAC802_16(p);

        if (mac_->getNodeType()==STA_BS) max_data = phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding())-b_data;
        else max_data = phy->getMaxPktSize (b->getDuration(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding())-b_data;

        debug2 ("\tIn Loop MacADDR :%d, MaxDATA :%d (burst duration :%d, b_data :%d)\n", mac_->addr(), max_data, b->getDuration(), b_data);
        if (max_data < HDR_MAC802_16_SIZE ||
                (c->getFragmentationStatus()!=FRAG_NOFRAG && max_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE))
            return b_data; //not even space for header

        if (c->getFragmentationStatus()!=FRAG_NOFRAG) {
            if (max_data >= ch->size()-c->getFragmentBytes()+HDR_MAC802_16_FRAGSUB_SIZE) {
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                //no need to fragment again
                wimaxHdr->frag_subheader.fc = FRAG_LAST;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                c->dequeue();  /*remove packet from queue */

                ch->size() = ch->size()-c->getFragmentBytes()+HDR_MAC802_16_FRAGSUB_SIZE; //new packet size
                //update fragmentation
                debug2 ("\tEnd of fragmentation (FRAG :%x), FSN :%d (max_data :%d, bytes to send :%d\n", FRAG_LAST, wimaxHdr->frag_subheader.fsn, max_data, ch->size());

                c->updateFragmentation (FRAG_NOFRAG, 0, 0);
            } else {
                //need to fragment the packet again
                p = p->copy(); //copy packet to send
                ch = HDR_CMN(p);
                wimaxHdr = HDR_MAC802_16(p);
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                wimaxHdr->frag_subheader.fc = FRAG_CONT;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                ch->size() = max_data; //new packet size
                //update fragmentation
                c->updateFragmentation (FRAG_CONT, (c->getFragmentNumber ()+1)%8, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE));
                debug2 ("\tContinue fragmentation (FRAG :%x), FSN :%d\n", FRAG_CONT, wimaxHdr->frag_subheader.fsn);
            }
        } else {
            if (max_data < ch->size() && c->isFragEnable()) {
                //need to fragment the packet for the first time
                p = p->copy(); //copy packet to send
                ch = HDR_CMN(p);
                wimaxHdr = HDR_MAC802_16(p);
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                wimaxHdr->frag_subheader.fc = FRAG_FIRST;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                ch->size() = max_data; //new packet size
                //update fragmentation
                c->updateFragmentation (FRAG_FIRST, 1, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE));
                debug2 ("\tFirst fragmentation (FRAG :%x), FSN :%d\n", FRAG_FIRST, c->getFragmentNumber());

            } else if (max_data < ch->size() && !c->isFragEnable()) {
                //the connection does not support fragmentation
                //can't move packets anymore
                return b_data;
            } else {
                //no fragmentation necessary
                c->dequeue();
            }
        }


        if (mac_->getNodeType()==STA_BS) {
            txtime = phy->getTrxTime (ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
            txtime2 = phy->getTrxSymbolTime (b_data+ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
            txtime3 = phy->getTrxSymbolTime (ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
        } else {
            txtime = phy->getTrxTime (ch->size(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding());
            txtime2 = phy->getTrxSymbolTime (b_data+ch->size(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding());
            txtime3 = phy->getTrxSymbolTime (ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding());
        }
        ch->txtime() = txtime3;
        txtime_s = (int)round (txtime3/phy->getSymbolTime ()); //in units of symbol
        //debug2 ("symbtime=%f\n", phy->getSymbolTime ());
        debug2 ("\tCheck packet to send size :%d txtime :%f(%d) duration :%d(%f)\n", ch->size(),txtime, txtime_s, b->getDuration(), b->getDuration()*phy->getSymbolTime ());
        assert ( txtime2 <= b->getDuration()*phy->getSymbolTime () );
        debug2 ("\tTransfert to burst (txtime :%f, txtime2 :%f, bduration :%f, txtime3 :%f)\n", txtime,txtime2,b->getDuration()*phy->getSymbolTime (),txtime3);


        int total_DL_num_subchannel = phy->getNumsubchannels(DL_);
        Ofdm_mod_rate dlul_map_mod = mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding();
        int num_of_slots = (int) ceil(ch->size()/phy->getSlotCapacity(dlul_map_mod,DL_)); //get the slots	needs.
        int num_of_subchannel = num_of_slots*2;
        int num_of_symbol = (int) ceil((double)(subchannel_offset + num_of_subchannel)/total_DL_num_subchannel);


        wimaxHdr = HDR_MAC802_16(p);
        if (wimaxHdr) {
            //wimaxHdr->phy_info.num_subchannels = b->getnumSubchannels();
            //wimaxHdr->phy_info.subchannel_offset = b->getSubchannelOffset ();
            //wimaxHdr->phy_info.num_OFDMSymbol = txtime_s;
            //wimaxHdr->phy_info.OFDMSymbol_offset = symbol_offset;//(int)round ((phy->getTrxSymbolTime (b_data, mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding()))/phy->getSymbolTime ());//offset;

            wimaxHdr->phy_info.num_subchannels = num_of_subchannel;
            wimaxHdr->phy_info.subchannel_offset = subchannel_offset;
            wimaxHdr->phy_info.num_OFDMSymbol = num_of_symbol;
            wimaxHdr->phy_info.OFDMSymbol_offset = symbol_offset;//(int)round ((phy->getTrxSymbolTime (b_data, mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding()))/phy->getSymbolTime ());//offset;


            // if(c->getPeerNode())
            //wimaxHdr->phy_info.channel_index = c->getPeerNode()->getchannel();
            wimaxHdr->phy_info.channel_index = 1;
            if (mac_->getNodeType()==STA_BS)
                wimaxHdr->phy_info.direction = 0;
            else
                wimaxHdr->phy_info.direction = 1;
        }

        symbol_offset += num_of_symbol-1;
        //symbol_offset+= txtime_s;

        //p = c->dequeue();   //dequeue connection queue
        debug10("enqueue into the burst for cid [%d].\n",c->get_cid());
        debug10("New packet in [%d] CID burst ||| sym_#[%d]\t sym_off[%d]\t subch_#[%d]\t subch_off[%d]\n",
                c->get_cid(), wimaxHdr->phy_info.num_OFDMSymbol, wimaxHdr->phy_info.OFDMSymbol_offset,wimaxHdr->phy_info.num_subchannels,
                wimaxHdr->phy_info.subchannel_offset);
        b->enqueue(p);         //enqueue into burst
        b_data += ch->size(); //increment amount of data enqueued
        if (!pkt_transfered && mac_->getNodeType()!=STA_BS) { //if we transfert at least one packet, remove bw request
            pkt_transfered = true;
            mac_->getMap()->getUlSubframe()->getBw_req()->removeRequest (c->get_cid());
        }
        p = c->get_queue()->head(); //get new head
    }
    return b_data;

}

/**
 * Transfer the packets from the given connection to the given burst
 * @param con The connection
 * @param b The burst
 * @param b_data Amount of data in the burst
 * @return the new burst occupation
 */
int WimaxScheduler::transfer_packets1 (Connection *c, Burst *b, int b_data)
{
    Packet *p;
    hdr_cmn* ch;
    hdr_mac802_16 *wimaxHdr;
    double txtime = 0.0;
    int txtime_s = 0;
    bool pkt_transfered = false;
    OFDMAPhy *phy = mac_->getPhy();
    int offset = b->getStarttime( );
    PeerNode *peer;
    int numsubchannels,subchannel_offset;
    int initial_offset = b->getStarttime( );
    int initial_subchannel_offset = b->getSubchannelOffset ();
    int bw_req_packet = GENERIC_HEADER_SIZE;
    int flag_bw_req = 0;
    int flag_pig = 0;
    int old_subchannel_offset = 0;

    int num_symbol_per_slot;

    if (mac_->amc_enable_ ==1 &&  c->getPeerNode() != NULL )
        debug2("CID %d the current MCS index is %d\n", c->get_cid(),c->getPeerNode()->getCurrentMcsIndex());

    subchannel_offset = initial_subchannel_offset;
    old_subchannel_offset = subchannel_offset;

    if (mac_->getNodeType()==STA_BS) {
        num_symbol_per_slot = phy->getSlotLength (DL_);
    } else {
        num_symbol_per_slot = phy->getSlotLength (UL_);
    }

    debug10 ("\nEntered transfer_packets1 for MacADDR :%d, for connection :%d with bdata  :%d, #subchannels :%d, #symbol :%d, IUC :%d, cqichSlotFlag %d\n",
             mac_->addr(), c->get_cid(), b_data, b->getnumSubchannels(), b->getDuration(), b->getIUC(), b->getCqichSlotFlag());

//Start sending registration message
    debug2 ("TOTO: check init ranging %d \n", c->getINIT_REQ_QUEUE(mac_->addr()));
    if ( (c->get_cid() == 0) && (c->getINIT_REQ_QUEUE(mac_->addr()) == 0) && (mac_->getNodeType()!=STA_BS) ) {

        Packet *p1= mac_->getPacket();
        hdr_cmn* ch = HDR_CMN(p1);
        hdr_mac802_16 *wimaxHdr;

        wimaxHdr = HDR_MAC802_16(p1);
        wimaxHdr->header.cid = INITIAL_RANGING_CID;

        int max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_) - b_data;

        p1->allocdata (sizeof (struct mac802_16_rng_req_frame));
        mac802_16_rng_req_frame *frame = (mac802_16_rng_req_frame*) p1->accessdata();
        frame->type = MAC_RNG_REQ;
        frame->ss_mac_address = mac_->addr();

        wimaxHdr->header.type_frag = 0;
        wimaxHdr->header.type_fbgm = 0;
        frame->req_dl_burst_profile = mac_->get_diuc() & 0xF; //we use lower bits only

        ch->size() = RNG_REQ_SIZE;

        debug10 ("\t   Sending INIT-RNG-MSG-REQ, cid :%d, ssid :%d, MaxSize :%d, msg-size :%d\n", c->get_cid(), mac_->addr(), max_data, ch->size());

        int assigned_subchannels;
        int symbol_offset;

        assigned_subchannels = phy->getNumSubchannels(b_data, mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);
        numsubchannels = phy->getNumSubchannels(ch->size(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);

        // unused Ofdm_mod_rate dlul_map_mod = mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding();
	// unused vr@tud int num_of_slots = (int) ceil((double)ch->size()/phy->getSlotCapacity(dlul_map_mod,UL_)); //get the slots needs.
        symbol_offset = b->getStarttime() + num_symbol_per_slot * ((b->getSubchannelOffset()+assigned_subchannels)/ phy->getNumsubchannels(UL_));
        txtime_s = (int)(ceil((double)(numsubchannels + subchannel_offset) / (double)phy->getNumsubchannels(UL_)))*num_symbol_per_slot;
        offset = b->getStarttime( )+ txtime_s-num_symbol_per_slot;
        subchannel_offset = (int) ceil((numsubchannels + subchannel_offset) % (phy->getNumsubchannels(UL_)));

        ch->txtime() = txtime_s*phy->getSymbolTime ();

        debug10 ("\t   Initial_Ranging_Msg, In station %d, Check packet to send: con id in BURST :%d, con id in connection :%d, size :%d, txtime :%f(%d), duration :%d(%f), numsubchnn :%d, subchnoff :%d\n",mac_->getNodeType(), c->get_cid(),b->getCid(), ch->size(),txtime, txtime_s, b->getDuration(), b->getDuration()*phy->getSymbolTime (),b->getnumSubchannels(),b->getSubchannelOffset ());

        wimaxHdr->phy_info.num_subchannels = numsubchannels;
        wimaxHdr->phy_info.subchannel_offset = initial_subchannel_offset;//subchannel_offset;
        wimaxHdr->phy_info.num_OFDMSymbol = txtime_s;
        wimaxHdr->phy_info.OFDMSymbol_offset = symbol_offset; //initial_offset;
        wimaxHdr->phy_info.channel_index = (mac_->getPeerNode_head())->getchannel();
        wimaxHdr->phy_info.direction = 1;

        debug10 ("\t   Initial_Ranging_Msg, In station %d, Packet phy header: numsymbols :%d, symboloffset :%d, subchanneloffset :%d, numsubchannel :%d, Channel_num :%d, direction :%d\n", mac_->getNodeType(), txtime_s,initial_offset, initial_subchannel_offset,numsubchannels,wimaxHdr->phy_info.channel_index,wimaxHdr->phy_info.direction);

        initial_offset = offset;
        initial_subchannel_offset = subchannel_offset;

        b->enqueue(p1);      //enqueue into burst
        b_data = b_data+ch->size();
        int t6time = (ceil)((double)(mac_->macmib_.t6_timeout)/(double)(mac_->macmib_.frame_duration));
        subchannel_offset = old_subchannel_offset;

        if ( mac_->getMap()->getUlSubframe()->getRanging()->getRequest_mac (mac_->addr()) != NULL ) {
            mac_->getMap()->getUlSubframe()->getRanging()->setTIMEOUT (mac_->addr(), t6time);
            debug10("\t   Reset CDMA_INIT_REQ timer to #frame :%d (%f sec) since send INIT-RNG-MSGQ out for cid :%d, ssid :%d\n", t6time, mac_->macmib_.t6_timeout, c->get_cid(), mac_->addr());
            c->setINIT_REQ_QUEUE(mac_->addr(), t6time);
        }

        if (mac_->getNodeType()!=STA_BS) {
            if ( mac_->getMap()->getUlSubframe()->getRanging()->getRequest_mac (mac_->addr()) != NULL ) {
                Packet *p_tmp = NULL;
                FrameMap *map1 = mac_->getMap();
                Burst *b1;

                if ((p_tmp = mac_->getMap()->getUlSubframe()->getRanging()->getPacket_P_mac(mac_->addr())) != NULL) {
                    int flag_transmit = mac_->getMap()->getUlSubframe()->getRanging()->getFLAGNOWTRANSMIT(mac_->addr());

                    debug10 ("Search flag_transmit :%d; remove cdma_bw_req if flag == 1, nbpdu :%d\n", flag_transmit, map1->getUlSubframe()->getNbPdu ());
                    if (flag_transmit == 1) {
                        int break_frame = 0;
                        for (int index = 0 ; index < map1->getUlSubframe()->getNbPdu (); index++) {
                            debug10 ("Search index :%d for CDMA enqueue :%d\n", index, mac_->addr());
                            b1 = map1->getUlSubframe()->getPhyPdu (index)->getBurst (0);

                            if (b1->getIUC()==UIUC_INITIAL_RANGING) {
                                int $index_p_burst = 0;
                                int q_len = map1->getUlSubframe()->getPhyPdu(index)->getBurst(0)->getQueueLength_packets();

                                while (q_len>0) {
                                    Packet *p_tmp_b = map1->getUlSubframe()->getPhyPdu(index)->getBurst(0)->lookup($index_p_burst);
                                    q_len--;
                                    $index_p_burst++;

                                    if (p_tmp_b) {
                                        // unused hdr_cmn* ch_tmp = HDR_CMN(p_tmp_b);
                                        hdr_mac802_16 *wimaxHdr_send;
                                        wimaxHdr_send = HDR_MAC802_16(p_tmp_b);
                                        cdma_req_header_t *header_s = (cdma_req_header_t *)&(HDR_MAC802_16(p_tmp_b)->header);
                                        if (mac_->addr() == header_s->br) {
                                            debug10 ("\tFound enqueued cdma_init_req in BURST (remove it) CID :%d, ADDR :%d, #symbol :%d, #symbol_offset :%d, #subchannel :%d, #subchannel_offset :%d, code :%d\n", header_s->cid, header_s->br, wimaxHdr_send->phy_info.num_OFDMSymbol, wimaxHdr_send->phy_info.OFDMSymbol_offset, wimaxHdr_send->phy_info.num_subchannels, wimaxHdr_send->phy_info.subchannel_offset, header_s->code);
                                            b1->remove(p_tmp_b);
                                            mac_->getMap()->getUlSubframe()->getRanging()->setFLAGNOWTRANSMIT(mac_->addr(), 0);
                                            break_frame = 1;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (break_frame == 1) break;
                        }
                    }
                }

            }

        }//end SS


        return b_data;
    }
//end sending registration message

    peer = mac_->getPeerNode_head();
    p = c->get_queue()->head();

    int max_data;

    if (mac_->getNodeType()==STA_BS) {
        max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_) - b_data;

        debug10 ("\tBS_transfer1.1, CID :%d with bdata :%d, but MaxData (MaxSize-bdata) :%d, MaxSize :%d, q->bytes :%d\n", c->get_cid(), b_data, max_data, phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_), c->queueByteLength() );
        debug10 ("\t   Bduration :%d, Biuc :%d, B#subchannel :%d CqichSlotFlag %d\n",b->getDuration(), b->getIUC(), b->getnumSubchannels(), b->getCqichSlotFlag());

    } else {
        if (mac_->getMap()->getUlSubframe()->getProfile (b->getIUC()) == NULL) {
            printf("Fatal ERROR: the IUC[%d] profile is NULL.\n",b->getIUC());
        }
        max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_) - b_data;

        debug10 ("\tSS_transfer1.1, CID :%d with bdata :%d, but MaxData (MaxSize-bdata) :%d, MaxSize :%d, bw-header-size :%d, q->bytes :%d\n", c->get_cid(), b_data, max_data, phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_), bw_req_packet, c->queueByteLength() );
        debug10 ("\t   Bduration :%d, Biuc :%d, B#subchannel :%d CqichSlotFlag %d\n",b->getDuration(), b->getIUC(), b->getnumSubchannels(), b->getCqichSlotFlag());

        if ( (max_data >= HDR_MAC802_16_SIZE) && (mac_->getMap()->getUlSubframe()->getBw_req()->getRequest (c->get_cid())!=NULL) && (c->queueByteLength() > 0) ) {
            // unused vr@tud int slot_br = 0;
            Connection *c_tmp;
            c_tmp = c;
            int i_packet = 0;
            int already_frag = 0;
            int realQueueSize = 0;

            Packet *np;
            debug10 ("Retrive connection :%d, qlen :%d\n", c_tmp->get_cid(), c_tmp->queueLength());
            for (int j_p = 0; j_p < c_tmp->queueLength(); j_p ++) {
                if ( (np = c_tmp->queueLookup(i_packet)) != NULL ) {
                    int p_size = hdr_cmn::access(np)->size();
                    i_packet++;

                    if ( (c_tmp->getFragmentBytes()>0) && (already_frag == 0) ) {
                        p_size = p_size - c_tmp->getFragmentBytes() + 4;
                        already_frag = 1;
                    }

                    realQueueSize += p_size;

                    Ofdm_mod_rate dlul_map_mod = mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding();

                    // several PDUs share one burst vr@tud
                    //int num_of_slots = (int) ceil((double)p_size/(double)phy->getSlotCapacity(dlul_map_mod,UL_));
                    //real_bytes = real_bytes + (int) ceil((double)num_of_slots*(double)(phy->getSlotCapacity(dlul_map_mod,UL_)));
                }
            }
            double slotCapacity =  phy->getSlotCapacity( mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);
            int nbOfSlots = int( ceil( double( realQueueSize + b_data)/ slotCapacity));
            realQueueSize = int( nbOfSlots * slotCapacity);

            if (c_tmp->getBW_REQ_QUEUE() == 0) {
                flag_bw_req = 0;
            }

            if ( (max_data + b_data) >= realQueueSize) {
                flag_bw_req = 0;
            } else {

#ifndef IF_PIG
                flag_bw_req = 6;
                debug10 ("\t   CheckBW-REQ (cdma_bw_req exist), generate BW-REQ, maxdata :%d, bw-req-size :%d, queue :%d\n",max_data, bw_req_packet, c->queueByteLength());
#endif

#ifdef IF_PIG
                if (max_data <= 10) {
                    flag_bw_req = 6;
                    debug10 ("\t   CheckBW-REQ (cdma_bw_req exist), generate BW-REQ, maxdata :%d, queue :%d\n",max_data, c->queueByteLength());
                } else {
                    debug10 ("\t   CheckBW-REQ (cdma_bw_req exist), generate Piggybacking, maxdata :%d, queue :%d\n",max_data, c->queueByteLength());
                    flag_pig = 2;
                    flag_bw_req = 0;
                }
#endif
            }
        } else {
            if (max_data >= c->queueByteLength()) {
                flag_bw_req = 0;
                flag_pig = 0;
            } else {
#ifdef IF_PIG
                if ( ( max_data < c->queueByteLength() ) && (max_data > 10) && (c->queueByteLength() > 0)) {
                    debug10 ("\t   CheckBW-REQ (cdma_bw_req exist), generate Piggybacking, maxdata :%d, queue :%d\n",max_data, c->queueByteLength());
                    flag_pig = 2;
                }
#endif
            }
        }//end if sending bw-req
    }

//Start sending BW-REQ packet
    if (flag_bw_req > 0) {
        Packet *p1= mac_->getPacket();
        hdr_cmn* ch = HDR_CMN(p1);
        hdr_mac802_16 *wimaxHdr;

        wimaxHdr = HDR_MAC802_16(p1);

        bw_req_header_t *header = (bw_req_header_t *)&(HDR_MAC802_16(p1)->header);
        header->ht=1;
        header->ec=1;
        header->type = 0x1; //aggregate
        //    header->br = c->queueByteLength();
        header->cid = c->get_cid();
        int realQueueSize = 0;

        int slot_br = 0;
        // int max_tmp_data_t1 = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_) - b_data;
        Ofdm_mod_rate dlul_map_mod = mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding();
        int num_slot_t1 =ceil((double)GENERIC_HEADER_SIZE/(double)phy->getSlotCapacity(dlul_map_mod,UL_));
        int max_tmp_data = phy->getMaxPktSize (b->getnumSubchannels()-num_slot_t1, mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_) - b_data;

        Connection *c_tmp;
        c_tmp = c;

        int i_packet = 0;
        int already_frag = 0;

        Packet *np_tmp;
        debug10 ("Retrive connection :%d, qlen :%d\n", c_tmp->get_cid(), c_tmp->queueLength());
        for (int j_p = 0; j_p<c_tmp->queueLength(); j_p++) {
            if ( (np_tmp = c_tmp->queueLookup(i_packet)) != NULL ) {
                int p_size = hdr_cmn::access(np_tmp)->size();
                i_packet++;

                if ( (c_tmp->getFragmentBytes()>0) && (already_frag == 0) ) {
                    p_size = p_size - c_tmp->getFragmentBytes() + 4;
                    already_frag = 1;
                }
                realQueueSize += p_size;

                // several PDUs share one burst vr@tud
                //int num_of_slots = (int) ceil((double)p_size/(double)phy->getSlotCapacity(dlul_map_mod,UL_));
                //real_bytes = real_bytes + (int) ceil((double)num_of_slots*(double)(phy->getSlotCapacity(dlul_map_mod,UL_)));
            }
        }

        double slotCapacity =  phy->getSlotCapacity( mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);
        int nbOfSlots = int( ceil( double( realQueueSize)/ slotCapacity));
        realQueueSize = int( nbOfSlots * slotCapacity);

        slot_br = realQueueSize - max_tmp_data;
        if (slot_br <0) {
            slot_br = 0;
        }
	    
        if (max_tmp_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE) {
            slot_br = realQueueSize;
        }
        header->br = slot_br;

        header->cid = c->get_cid();
        debug10 ("\t   Sending BW-REQ, cid :%d, qBytes :%d, qLen :%d, bw-req :%d, realQueueSize :%d\n", header->cid, c->queueByteLength(), c->queueLength(), header->br, realQueueSize);

        wimaxHdr->header.type_frag = 0;
        wimaxHdr->header.type_fbgm = 0;

        int assigned_subchannels;
        int symbol_offset;
        ch->size() = bw_req_packet;

        assigned_subchannels = phy->getNumSubchannels(b_data, mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);
        numsubchannels = phy->getNumSubchannels(ch->size(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);

        // int num_of_slots = (int) ceil((double)ch->size()/phy->getSlotCapacity(dlul_map_mod,UL_)); //get the slots needs.
        symbol_offset = b->getStarttime() + num_symbol_per_slot * ((b->getSubchannelOffset()+assigned_subchannels)/ phy->getNumsubchannels(UL_));
        txtime_s = (int)(ceil((double)(numsubchannels + subchannel_offset) / (double)phy->getNumsubchannels(UL_)))*num_symbol_per_slot;
        offset = b->getStarttime( )+ txtime_s-num_symbol_per_slot;
        subchannel_offset = (int) ceil((numsubchannels + subchannel_offset) % (phy->getNumsubchannels(UL_)));

        ch->txtime() = txtime_s*phy->getSymbolTime ();

        debug10 ("\t   BW-REQ, In station %d, Check packet to send: con id in BURST :%d, con id in connection :%d, size :%d, txtime :%f(%d), duration :%d(%f), numsubchnn :%d, subchnoff :%d\n",
                 mac_->getNodeType(), c->get_cid(),b->getCid(), ch->size(),txtime, txtime_s, b->getDuration(), b->getDuration()*phy->getSymbolTime (),b->getnumSubchannels(),b->getSubchannelOffset ());

        wimaxHdr->phy_info.num_subchannels = numsubchannels;
        wimaxHdr->phy_info.subchannel_offset = initial_subchannel_offset;//subchannel_offset;
        wimaxHdr->phy_info.num_OFDMSymbol = txtime_s;
        wimaxHdr->phy_info.OFDMSymbol_offset = symbol_offset; //initial_offset;
        wimaxHdr->phy_info.channel_index = (mac_->getPeerNode_head())->getchannel();
        wimaxHdr->phy_info.direction = 1;

        debug10 ("\t   BW-REQ, In station %d, Packet phy header: numsymbols :%d, symboloffset :%d, subchanneloffset :%d, numsubchannel :%d, Channel_num :%d, direction :%d\n", mac_->getNodeType(), txtime_s,initial_offset, initial_subchannel_offset,numsubchannels,wimaxHdr->phy_info.channel_index,wimaxHdr->phy_info.direction);

        b->enqueue(p1);      //enqueue into burst

        //b_data += wimaxHdr->phy_info.num_subchannels*phy->getSlotCapacity(dlul_map_mod,UL_);
        // bandwidth request has a fixed size
        b_data += HDR_MAC802_16_SIZE;

        int t16time = (ceil)((double)(mac_->macmib_.t16_timeout)/(double)(mac_->macmib_.frame_duration));
        c->setBW_REQ_QUEUE(t16time);

        initial_offset = offset;
        initial_subchannel_offset = subchannel_offset;

        if (mac_->getNodeType()!=STA_BS) {
            if ( mac_->getMap()->getUlSubframe()->getBw_req()->getRequest (c->get_cid()) != NULL ) {
                mac_->getMap()->getUlSubframe()->getBw_req()->setTIMEOUT (c->get_cid(), t16time);
                debug10("Reset CDMA_BW_REQ timer since send BW-REQ out for cid [%d].\n",c->get_cid());
            }
        }

        if (mac_->getNodeType()!=STA_BS) {
            if ( mac_->getMap()->getUlSubframe()->getBw_req()->getRequest (c->get_cid()) != NULL ) {
                Packet *p_tmp = NULL;
                FrameMap *map1 = mac_->getMap();
                Burst *b1;

                if ((p_tmp = mac_->getMap()->getUlSubframe()->getBw_req()->getPacket_P(c->get_cid())) != NULL) {
                    int flag_transmit = mac_->getMap()->getUlSubframe()->getBw_req()->getFLAGNOWTRANSMIT(c->get_cid());

                    debug10 ("Search flag_transmit :%d; remove cdma_bw_req if flag == 1, nbpdu :%d\n", flag_transmit, map1->getUlSubframe()->getNbPdu ());
                    if (flag_transmit == 1) {
                        int break_frame = 0;
                        for (int index = 0 ; index < map1->getUlSubframe()->getNbPdu (); index++) {
                            debug10 ("Search index :%d for CDMA enqueue :%d\n", index, c->get_cid());
                            b1 = map1->getUlSubframe()->getPhyPdu (index)->getBurst (0);

                            if (b1->getIUC()==UIUC_REQ_REGION_FULL) {
                                int $index_p_burst = 0;
                                int q_len = map1->getUlSubframe()->getPhyPdu(index)->getBurst(0)->getQueueLength_packets();

                                while (q_len>0) {
                                    Packet *p_tmp_b = map1->getUlSubframe()->getPhyPdu(index)->getBurst(0)->lookup($index_p_burst);
                                    q_len--;
                                    $index_p_burst++;

                                    if (p_tmp_b) {
                                        // unused hdr_cmn* ch_tmp = HDR_CMN(p_tmp_b);
                                        hdr_mac802_16 *wimaxHdr_send;
                                        wimaxHdr_send = HDR_MAC802_16(p_tmp_b);
                                        cdma_req_header_t *header_s = (cdma_req_header_t *)&(HDR_MAC802_16(p_tmp_b)->header);
                                        if (c->get_cid() == header_s->cid) {
                                            debug10 ("\tFound enqueued cdma_bw_req in BURST (remove it) CID :%d, #symbol :%d, #symbol_offset :%d, #subchannel :%d, #subchannel_offset :%d, code :%d\n", header_s->cid, wimaxHdr_send->phy_info.num_OFDMSymbol, wimaxHdr_send->phy_info.OFDMSymbol_offset, wimaxHdr_send->phy_info.num_subchannels, wimaxHdr_send->phy_info.subchannel_offset, header_s->code);
                                            b1->remove(p_tmp_b);
                                            mac_->getMap()->getUlSubframe()->getBw_req()->setFLAGNOWTRANSMIT(c->get_cid(), 0);
                                            break_frame = 1;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (break_frame == 1) break;
                        }
                    }
                }
            }
        }//end SS
    }
//end sending BW-REQ packet


    if (max_data < HDR_MAC802_16_SIZE ||(c->getFragmentationStatus()!=FRAG_NOFRAG
                                         && max_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE)) {
        debug2("In station %d returning as no space\n", mac_->getNodeType());
        return b_data; //not even space for header
    }


    if (flag_bw_req>0) {
        if ( (max_data-flag_bw_req) < HDR_MAC802_16_SIZE ||(c->getFragmentationStatus()!=FRAG_NOFRAG
                && (max_data-flag_bw_req) < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE)) {
            debug2("In station %d returning as no space\n", mac_->getNodeType());
            return b_data; //not even space for header
        }
    }

    int gm_flag = 0;

#ifdef IF_PIG
    //Piggybacking 1-May
    int more_bw = 0;

    if (flag_pig > 0 ) {
        more_bw = c->queueByteLength() - max_data;
        if (mac_->getNodeType()!=STA_BS&&p) {
            ch = HDR_CMN(p);
            wimaxHdr = HDR_MAC802_16(p);

            debug10("In station %d (SS) ch-size :%d, max_data :%d, queue_bytes :%d, queue_len :%d\n", mac_->getNodeType(), ch->size(),  max_data, c->queueByteLength(), c->queueLength());
            if (max_data < c->queueByteLength()) {
                wimaxHdr->header.type_fbgm = true;
                wimaxHdr->grant_subheader.piggyback_req = more_bw;
                debug10 ("In station %d (SS) Piggybacking flag :%d, queue_bytes :%d, queue_len :%d, max_data :%d, askmore :%d\n", mac_->getNodeType(), wimaxHdr->header.type_fbgm, c->queueByteLength(), c->queueLength(), max_data, more_bw);
                gm_flag = 2;
            } else {
                wimaxHdr->header.type_fbgm = 0;
                debug10 ("In station %d (SS) no gm :%d, maxdata :%d, queue :%d\n",  mac_->getNodeType(), wimaxHdr->header.type_fbgm, max_data, c->queueByteLength());
            }
        }
    }

    //Remove cdma-bw-req since we send piggybacking
    if (mac_->getNodeType()!=STA_BS) {
        if ( mac_->getMap()->getUlSubframe()->getBw_req()->getRequest (c->get_cid()) != NULL ) {
            Packet *p_tmp = NULL;
            FrameMap *map1 = mac_->getMap();
            Burst *b1;

            if ((p_tmp = mac_->getMap()->getUlSubframe()->getBw_req()->getPacket_P(c->get_cid())) != NULL) {
                int flag_transmit = mac_->getMap()->getUlSubframe()->getBw_req()->getFLAGNOWTRANSMIT(c->get_cid());

                debug10 ("Search flag_transmit :%d; remove cdma_bw_req if flag == 1, nbpdu :%d\n", flag_transmit, map1->getUlSubframe()->getNbPdu ());
                if (flag_transmit == 1) {
                    int break_frame = 0;
                    for (int index = 0 ; index < map1->getUlSubframe()->getNbPdu (); index++) {
                        debug10 ("Search index :%d for CDMA enqueue :%d\n", index, c->get_cid());
                        b1 = map1->getUlSubframe()->getPhyPdu (index)->getBurst (0);

                        if (b1->getIUC()==UIUC_REQ_REGION_FULL) {
                            int $index_p_burst = 0;
                            int q_len = map1->getUlSubframe()->getPhyPdu(index)->getBurst(0)->getQueueLength_packets();

                            while (q_len>0) {
                                Packet *p_tmp_b = map1->getUlSubframe()->getPhyPdu(index)->getBurst(0)->lookup($index_p_burst);
                                q_len--;
                                $index_p_burst++;

                                if (p_tmp_b) {
                                    hdr_cmn* ch_tmp = HDR_CMN(p_tmp_b);
                                    hdr_mac802_16 *wimaxHdr_send;
                                    wimaxHdr_send = HDR_MAC802_16(p_tmp_b);
                                    cdma_req_header_t *header_s = (cdma_req_header_t *)&(HDR_MAC802_16(p_tmp_b)->header);
                                    if (c->get_cid() == header_s->cid) {
                                        debug10 ("\tFound enqueued cdma_bw_req in BURST (remove it) CID :%d, #symbol :%d, #symbol_offset :%d, #subchannel :%d, #subchannel_offset :%d, code :%d\n", header_s->cid, wimaxHdr_send->phy_info.num_OFDMSymbol, wimaxHdr_send->phy_info.OFDMSymbol_offset, wimaxHdr_send->phy_info.num_subchannels, wimaxHdr_send->phy_info.subchannel_offset, header_s->code);
                                        b1->remove(p_tmp_b);
                                        mac_->getMap()->getUlSubframe()->getBw_req()->setFLAGNOWTRANSMIT(c->get_cid(), 0);
                                        break_frame = 1;
                                        break;
                                    }
                                }
                            }
                        }
                        if (break_frame == 1) break;
                    }
                }
            }
            debug10("\tRemove CDMA_BW_REQ if enqueued since send BW-REQ (PIGGYBACKING) out for cid [%d].\n",c->get_cid());
            mac_->getMap()->getUlSubframe()->getBw_req()->removeRequest (c->get_cid());
        }
    }//end SS
#endif

    // overloaded function added by rpi

    while (p) {
        debug10 ("In station %d Entering while loop, connection type %d, CID :%d\n", mac_->getNodeType(), c->get_category(), c->get_cid());
        ch = HDR_CMN(p);
        wimaxHdr = HDR_MAC802_16(p);

        if (ch->size() < 0 ) debug2(" packet size negative --- panic!!! ");

        if (mac_->getNodeType()==STA_BS) {
            max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_) - b_data;
        } else {
            max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_) - b_data - gm_flag;
        }

        if (max_data <= HDR_MAC802_16_SIZE ||	(c->getFragmentationStatus()!=FRAG_NOFRAG
                                               && max_data <= HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE))
            return b_data; //not even space for header

        if (c->getFragmentationStatus()==FRAG_NOFRAG && max_data <= HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE
                && (ch->size()>HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE) ) {
            return b_data; //not even space for header
        }

        if (c->getFragmentationStatus()!=FRAG_NOFRAG) {
            if (max_data >= ch->size()-c->getFragmentBytes()+HDR_MAC802_16_FRAGSUB_SIZE) { //getFrag => include MACHEADER
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                //no need to fragment again
                wimaxHdr->frag_subheader.fc = FRAG_LAST;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                int more_bw = 0;


                c->dequeue();  /*remove packet from queue */
                ch->size() = ch->size()-c->getFragmentBytes()+HDR_MAC802_16_FRAGSUB_SIZE; //new packet size
                //update fragmentation
                if (ch->size() < 0 )
                    debug2(" packet size negative -- panic !!! \n");

                debug2 ("\nEnd of fragmentation %d, CID :%d, (max_data :%d, bytes to send :%d, getFragmentBytes :%d, getFragNumber :%d, updated Frag :%d, update FragBytes :%d, con->qBytes :%d, con->qlen :%d, more_bw :%d)\n", wimaxHdr->frag_subheader.fsn, c->get_cid(), max_data, ch->size(), c->getFragmentBytes(), c->getFragmentNumber(), 0, 0, c->queueByteLength(), c->queueLength(), more_bw);

                c->updateFragmentation (FRAG_NOFRAG, 0, 0);
                if (gm_flag>0) {
                    gm_flag = 0;
                    ch->size() = ch->size() + HDR_PIG;
                }

            } else {
                //need to fragment the packet again
                p = p->copy(); //copy packet to send
                ch = HDR_CMN(p);
                wimaxHdr = HDR_MAC802_16(p);
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                wimaxHdr->frag_subheader.fc = FRAG_CONT;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                int more_bw = 0;
                ch->size() = max_data; //new packet size

                if (gm_flag>0) {
                    gm_flag = 0;
                    ch->size() = ch->size() + HDR_PIG;
                }
                //update fragmentation

                debug2 ("\nContinue fragmentation %d, CID :%d, (max_data :%d, bytes to send :%d, getFragmentBytes :%d, getFragNumber :%d, updated Frag :%d, update FragBytes :%d, con->qBytes :%d, con->qlen :%d, more_bw :%d)\n", wimaxHdr->frag_subheader.fsn, c->get_cid(), max_data, ch->size(),  c->getFragmentBytes(), c->getFragmentNumber(), (c->getFragmentNumber()+1)%8, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE), c->queueByteLength(), c->queueLength(), more_bw);

                c->updateFragmentation (FRAG_CONT, (c->getFragmentNumber ()+1)%8, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE));
            }
        } else {//else no flag
            if (max_data < ch->size() && c->isFragEnable()) {
                //need to fragment the packet for the first time
                p = p->copy(); //copy packet to send
                ch = HDR_CMN(p);
                int ori_ch = ch->size();
                wimaxHdr = HDR_MAC802_16(p);
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                wimaxHdr->frag_subheader.fc = FRAG_FIRST;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                int more_bw = 0;
                ch->size() = max_data; //new packet size

                debug2 ("\nFirst fragmentation %d, CID :%d, (max_data :%d, bytes to send :%d, ori_size :%d, getFragmentBytes :%d, FRAGSUB :%d, getFragNumber :%d, updated Frag ;%d, update FragBytes :%d, con->qBytes :%d, con->qlen :%d, more_bw :%d)\n", wimaxHdr->frag_subheader.fsn, c->get_cid(), max_data, ch->size(), ori_ch, c->getFragmentBytes(), HDR_MAC802_16_FRAGSUB_SIZE, c->getFragmentNumber (), 1, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE),c->queueByteLength(), c->queueLength (), more_bw);

                c->updateFragmentation (FRAG_FIRST, 1, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE));
                if (gm_flag>0) {
                    gm_flag = 0;
                    ch->size() = ch->size() + HDR_PIG;
                }
            } else if (max_data < ch->size() && !c->isFragEnable()) {
                //the connection does not support fragmentation
                //can't move packets anymore
                return b_data;
            } else {
                //no fragmentation necessary
                int more_bw = 0;
                if (gm_flag>0) {
                    gm_flag = 0;
                    ch->size() = ch->size() + HDR_PIG;
                }

                debug2 ("\nNo fragmentation %d, (max_data :%d, bytes to send :%d, con->qBytes :%d, con->qlen :%d, more_bw :%d\n", wimaxHdr->frag_subheader.fsn, max_data, ch->size(), c->queueByteLength(), c->queueLength(), more_bw);
                c->dequeue();

            }//end frag
        }

        //    debug10 ("B_data1 :%d, ch->size :%d\n", b_data, ch->size());
        //you did send something; reset BW_REQ_QUEUE
        c->setBW_REQ_QUEUE(0);

        if (mac_->getNodeType()!=STA_BS) {
            if ( mac_->getMap()->getUlSubframe()->getBw_req()->getRequest (c->get_cid()) != NULL ) {
                Packet *p_tmp = NULL;
                FrameMap *map1 = mac_->getMap();
                Burst *b1;

                if ((p_tmp = mac_->getMap()->getUlSubframe()->getBw_req()->getPacket_P(c->get_cid())) != NULL) {
                    int flag_transmit = mac_->getMap()->getUlSubframe()->getBw_req()->getFLAGNOWTRANSMIT(c->get_cid());

                    debug10 ("Search flag_transmit :%d; remove cdma_bw_req if flag == 1, nbpdu :%d\n", flag_transmit, map1->getUlSubframe()->getNbPdu ());
                    //	     debug10 ("p_tmp address :%d\n", p_tmp);
                    if (flag_transmit == 1) {
                        int break_frame = 0;
                        for (int index = 0 ; index < map1->getUlSubframe()->getNbPdu (); index++) {
                            debug10 ("Search index :%d for CDMA enqueue :%d\n", index, c->get_cid());
                            b1 = map1->getUlSubframe()->getPhyPdu (index)->getBurst (0);

                            if (b1->getIUC()==UIUC_REQ_REGION_FULL) {
                                int $index_p_burst = 0;
                                int q_len = map1->getUlSubframe()->getPhyPdu(index)->getBurst(0)->getQueueLength_packets();

                                while (q_len>0) {
                                    Packet *p_tmp_b = map1->getUlSubframe()->getPhyPdu(index)->getBurst(0)->lookup($index_p_burst);
                                    q_len--;
                                    $index_p_burst++;

                                    if (p_tmp_b) {
                                        //debug10 ("p_tmp address from burst :%d\n", p_tmp_b);
                                        // hdr_cmn* ch_tmp = HDR_CMN(p_tmp_b);
                                        hdr_mac802_16 *wimaxHdr_send;
                                        wimaxHdr_send = HDR_MAC802_16(p_tmp_b);
                                        cdma_req_header_t *header_s = (cdma_req_header_t *)&(HDR_MAC802_16(p_tmp_b)->header);
                                        if (c->get_cid() == header_s->cid) {
                                            debug10 ("\tFound enqueued cdma_bw_req in BURST (remove it) CID :%d, #symbol :%d, #symbol_offset :%d, #subchannel :%d, #subchannel_offset :%d, code :%d\n", header_s->cid, wimaxHdr_send->phy_info.num_OFDMSymbol, wimaxHdr_send->phy_info.OFDMSymbol_offset, wimaxHdr_send->phy_info.num_subchannels, wimaxHdr_send->phy_info.subchannel_offset, header_s->code);
                                            b1->remove(p_tmp_b);
                                            mac_->getMap()->getUlSubframe()->getBw_req()->setFLAGNOWTRANSMIT(c->get_cid(), 0);
                                            break_frame = 1;
                                            break;
                                        }
                                    }
                                }
                            }
                            if (break_frame == 1)
                                break;
                        }
                    }
                }

                debug10("\tRemove CDMA_BW_REQ (if exist) since send some packet out for cid [%d].\n",c->get_cid());
                mac_->getMap()->getUlSubframe()->getBw_req()->removeRequest (c->get_cid());
            }
        }//end SS

        //    debug10 ("B_data2 :%d, ch->size :%d\n", b_data, ch->size());
        int assigned_subchannels;
        int symbol_offset;
        //Trying to calculate how to fill up things here? Before this, frag and arq business?

        /*Richard fixed*/
        if (mac_->getNodeType()==STA_BS) {
            assigned_subchannels = phy->getNumSubchannels(b_data, mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_);
            numsubchannels = phy->getNumSubchannels(ch->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_);
            debug10("Richard (BS) is going to use [%d] subchannels.\n",numsubchannels);

            Ofdm_mod_rate dlul_map_mod = mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding();
            int num_of_slots = (int) ceil((double)ch->size()/phy->getSlotCapacity(dlul_map_mod,DL_)); //get the slots needs.
            numsubchannels = num_of_slots;
            debug10(" (BS) is going to use [%d] subchannels.\n",numsubchannels);
            symbol_offset = b->getStarttime() + num_symbol_per_slot * ((b->getSubchannelOffset()+assigned_subchannels)/ phy->getNumsubchannels(DL_));

            debug2("number subchannel before calculation is[%d]\t assigned_subchn[%d]\t subch_off[%d]\t PerSYmbolSubch#[%d] %f\n",
                   numsubchannels,assigned_subchannels, subchannel_offset, phy->getNumsubchannels(DL_),
                   ceil((double)(numsubchannels + subchannel_offset) / phy->getNumsubchannels(DL_)));

            txtime_s = (int)(ceil((double)(numsubchannels + subchannel_offset) / (double)phy->getNumsubchannels(DL_))) * num_symbol_per_slot;
            offset = b->getStarttime( )+ txtime_s-num_symbol_per_slot;
            subchannel_offset = (int) ceil((numsubchannels + subchannel_offset) % (phy->getNumsubchannels(DL_)));
        } else { //if its the SS
            assigned_subchannels = phy->getNumSubchannels(b_data, mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);

            numsubchannels = phy->getNumSubchannels(ch->size(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);
            // Ofdm_mod_rate dlul_map_mod = mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding();
            //int num_of_slots = (int) ceil((double)ch->size()/phy->getSlotCapacity(dlul_map_mod,UL_)); //get the slots needs.
            symbol_offset = b->getStarttime() + num_symbol_per_slot * ((b->getSubchannelOffset()+assigned_subchannels)/ phy->getNumsubchannels(UL_));

            debug2("number subchannel before calculation is[%d]\t assigned_subchn[%d]\t subch_off[%d]\t PerSYmbolSubch#[%d] %f\n",
                   numsubchannels,assigned_subchannels, subchannel_offset, phy->getNumsubchannels(UL_),
                   ceil((double)(numsubchannels + subchannel_offset) / phy->getNumsubchannels(UL_)));

            txtime_s = (int)(ceil((double)(numsubchannels + subchannel_offset) / (double)phy->getNumsubchannels(UL_)))*num_symbol_per_slot;
            offset = b->getStarttime( )+ txtime_s-num_symbol_per_slot;
            subchannel_offset = (int) ceil((numsubchannels + subchannel_offset) % (phy->getNumsubchannels(UL_)));
        }
        debug2 ("txtime_s=%d, symbol time = %f txtime=%f\n", txtime_s, phy->getSymbolTime (), txtime_s*phy->getSymbolTime ());

        ch->txtime() = txtime_s*phy->getSymbolTime ();

        debug2 ("In station %d Check packet to send: con id in burst = %d con id in connection = %d size=%d txtime=%f(%d) duration=%d(%f) numsubchnn = %d subchnoff = %d\n",
                mac_->getNodeType(), c->get_cid(),b->getCid(), ch->size(),ch->txtime(), txtime_s, b->getDuration(), b->getDuration()*phy->getSymbolTime (),b->getnumSubchannels(),b->getSubchannelOffset ());

        wimaxHdr = HDR_MAC802_16(p);
        wimaxHdr->phy_info.num_subchannels = numsubchannels;
        wimaxHdr->phy_info.subchannel_offset = initial_subchannel_offset;//subchannel_offset;
        wimaxHdr->phy_info.num_OFDMSymbol = txtime_s;
        wimaxHdr->phy_info.OFDMSymbol_offset = symbol_offset; //initial_offset;

        debug10("In transfer_packets1-- packets info: symbol_off[%d]\t symbol_#[%d]\t subch_Off[%d]\t subch_#[%d]\n",
                wimaxHdr->phy_info.OFDMSymbol_offset,wimaxHdr->phy_info.num_OFDMSymbol,
                wimaxHdr->phy_info.subchannel_offset,wimaxHdr->phy_info.num_subchannels);

        //wimaxHdr->phy_info.channel_index = c->getPeerNode()->getchannel();
        if (mac_->getNodeType()==STA_BS) {
            wimaxHdr->phy_info.direction = 0;
            // debug2("Peer Node is %d\n", c->getPeerNode());
            if (c->getPeerNode()) {
                wimaxHdr->phy_info.channel_index = c->getPeerNode()->getchannel();
                /*Xingting added to support MCS. Here is used to change the MCS index.*/
                debug2("In transfer function, BS [%d] set SS[%d], flag is %d\n",mac_->addr(), c->getPeerNode()->getAddr(), mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()));
                if (mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()) == TRUE && mac_->amc_enable_ == 1) {
                    debug2("in transfer function, BS set SS[%d] gonna set mcs index to %d\n",c->getPeerNode()->getAddr(),c->getPeerNode()->getCurrentMcsIndex());
                    wimaxHdr->phy_info.mcs_index_ = c->getPeerNode()->getCurrentMcsIndex();
                    mac_->set_change_modulation_flag(c->getPeerNode()->getAddr(), FALSE);
                } else {
                    debug2("in transfer function, no change modulation. amc_enable %d   But MCS index set to %d  SS [%d]  get_change_modulation_flag[%d] .\n",
                           mac_->amc_enable_,  c->getPeerNode()->getCurrentMcsIndex(), c->getPeerNode()->getAddr(), mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()));
                    wimaxHdr->phy_info.mcs_index_  = c->getPeerNode()->getCurrentMcsIndex(); //-1;
                }
            }
        } else {
            wimaxHdr->phy_info.channel_index = (mac_->getPeerNode_head())->getchannel();
            wimaxHdr->phy_info.direction = 1;
        }

        debug2("In station %d Packet phy header : numsymbols =%d , symboloffset =%d,  subchanneloffset= %d, numsubchannel = %d, Channel_num = %d, direction =%d \n",
               mac_->getNodeType(), txtime_s,initial_offset, initial_subchannel_offset,numsubchannels,wimaxHdr->phy_info.channel_index,wimaxHdr->phy_info.direction);

        initial_offset = offset;
        initial_subchannel_offset = subchannel_offset;

        //  rpi end put phy header info - like num subchannels and ofdmsymbols....
        //    debug10 ("B_data3 :%d, ch->size :%d\n", b_data, ch->size());

        b->enqueue(p);      //enqueue into burst

        /* Several MAC PDU may share one burst \\ vr@tud 04-09
         * On the MAC Layer Bytes and not Slots are the smallest allocation units */
       /* Ofdm_mod_rate dlul_map_mod;

        if (mac_->getNodeType()==STA_BS) {
            dlul_map_mod = mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding();
        } else {
            dlul_map_mod = mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding();
        }
        b_data += wimaxHdr->phy_info.num_subchannels*phy->getSlotCapacity(dlul_map_mod,UL_);
		*/

        b_data += ch->size(); //increment amount of data enqueued
        debug2 ("In station %d packet enqueued b_data = %d \n", mac_->getNodeType(), b_data);





        if (!pkt_transfered && mac_->getNodeType()!=STA_BS) { //if we transfert at least one packet, remove bw request
            pkt_transfered = true;
            mac_->getMap()->getUlSubframe()->getBw_req()->removeRequest (c->get_cid());
        }
        p = c->get_queue()->head(); //get new head
    }
    return b_data;
}

/**
 * Transfer the packets from the given connection to the given burst
 * @param con The connection
 * @param b The burst
 * @param b_data Amount of data in the burst
 * @return the new burst occupation
 */
int WimaxScheduler::transfer_packets_with_fragpackarq(Connection *c, Burst *b, int b_data)
{
    Packet *p, *mac_pdu, *p_temp, *p_current;
    hdr_cmn* ch, *ch_pdu, *ch_temp, *ch_current;
    hdr_mac802_16 *wimaxHdr, *wimaxHdr_pdu, *wimaxHdr_temp, *wimaxHdr_current;
    double txtime = 0.0;
    int txtime_s = 0;
    bool pkt_transfered = false;
    OFDMAPhy *phy = mac_->getPhy();
    int offset = b->getStarttime( );
    PeerNode *peer;
    int numsubchannels,subchannel_offset;
    int initial_offset = b->getStarttime( );
    int initial_subchannel_offset = b->getSubchannelOffset ();
    int bw_req_packet = GENERIC_HEADER_SIZE;
    int flag_bw_req = 0;

    int num_symbol_per_slot;
    if (mac_->getNodeType()==STA_BS) {
        num_symbol_per_slot = phy->getSlotLength (DL_);
    } else {
        num_symbol_per_slot = phy->getSlotLength (UL_);
    }

    debug2 ("Entered transfer packets with ARQ for connection %d with bdata = %d\n ", c->get_cid(), b_data);
    peer = mac_->getPeerNode_head();
    debug2("ARQ Transfer_Packets with Frag pack arq\n");

    /*Firstly, check the retransmission queue status. */
    if (c->getArqStatus () != NULL && c->getArqStatus ()->isArqEnabled() == 1) {
        if ((c->getArqStatus ()->arq_retrans_queue_) && (c->getArqStatus ()->arq_retrans_queue_->length() > 0)) {
            debug2("ARQ Transfer Packets: Transferring a Retransmission packet\n");
            mac_pdu = c->getArqStatus ()->arq_retrans_queue_->head();
        } else
            mac_pdu = c->get_queue()->head();
    } else /*If ARQ is not supported, directly get the header packet of the queue.*/
        mac_pdu = c->get_queue()->head();

    if (!mac_pdu) {
        debug2("ARQ: NO Data present to be sent\n");
        return b_data;
    }

    int max_data;
    wimaxHdr_pdu= HDR_MAC802_16(mac_pdu);
    ch_pdu = HDR_CMN(mac_pdu);
    //printf("this brust has [%d] of subchannels.\n",b->getnumSubchannels());

    // int arq_block_size = ch_pdu->size ();

    /*get the maximum size of the data which could be transmitted using this data burst without changing anything.*/
    if (mac_->getNodeType()==STA_BS) {
        max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_) - b_data;
        debug2 ("ARQ: In Mac %d max data=%d (b_data=%d) ch_pdu size=%d\n", mac_->addr(), max_data, b_data,ch_pdu->size ());
    } else {
        max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_) - b_data;
        debug2 ("ARQ: In Mac %d max data=%d (b_data=%d) ch_pdu size=%d\n", mac_->addr(), max_data, b_data,ch_pdu->size ());

        /*If this data burst could hold one BW REQ message and could not hold even one ARQ block. We need sponser a BW REQ message.*/
        if ( (max_data >= bw_req_packet) && (max_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE+ch_pdu->size ()) ) {
            debug10 ("\t   CheckBW-REQ, SS Should generate BW-REQ, maxdata :%d, bw-req-size :%d, queue :%d\n",
                     max_data, bw_req_packet, c->queueByteLength());
            flag_bw_req = 1;
        } else /*Otherwise, this data burst could hold the all the ARQ block or even could not hold a BW REQ message, we donot need a
		       BW REQ.*/
        {
            flag_bw_req = 0;
        }
    }


//#ifdef WIMAX_TEST
    /*This data burst could not hold even a BW REQ message, then we simply return. Maybe this will be a dead-end loop?????*/
    if (max_data < HDR_MAC802_16_SIZE ||
            (c->getFragmentationStatus()!=FRAG_NOFRAG && max_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE)) {
        debug2("In station %d returning as no space\n", mac_->getNodeType());
        return b_data; //not even space for header
    }

    /*If this data burst could hold a BW REQ, we fill out the necessary info. to ask for more BW.*/
    if (flag_bw_req == 1) {
        Packet *p1= mac_->getPacket();
        hdr_cmn* ch = HDR_CMN(p1);
        hdr_mac802_16 *wimaxHdr;

        wimaxHdr = HDR_MAC802_16(p1);

        bw_req_header_t *header = (bw_req_header_t *)&(HDR_MAC802_16(p1)->header);
        header->ht=1;
        header->ec=1;
        header->type = 0x1; //aggregate
        //header->br = c->queueByteLength()+HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE;  /*set the BW we need.*/
        header->br = c->getArqStatus ()->arq_retrans_queue_->byteLength()+ c->queueByteLength() +HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE;  /*set the BW we need.*/
        debug2("In ARQ: MS is gonna require BW %d\n", header->br);
#if 0
        if (c->getArqStatus ()->arq_retrans_queue_->byteLength()!=0) {
            header->br = c->getArqStatus ()->arq_retrans_queue_->byteLength() + c->get_queue()->byteLength()+HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE;  /*set the BW we need.*/
        } else {
            header->br =c->get_queue()->byteLength() +HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE;
        }
#endif
        header->cid = c->get_cid();
        debug10 ("\t   Sending BW-REQ, cid :%d, qBytes :%d \n", header->cid, header->br);

        int assigned_subchannels;
        int symbol_offset;
        ch->size() = bw_req_packet;

        assigned_subchannels = phy->getNumSubchannels(b_data, mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);
        numsubchannels = phy->getNumSubchannels(ch->size(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);
        symbol_offset = b->getStarttime() + (b->getSubchannelOffset()+assigned_subchannels)/phy->getNumsubchannels(UL_);

        /*test code begin*/
        debug2("b->starttime() %d,  b->getsubchoffset %d, symbol_offset %d\n", b->getStarttime(), b->getSubchannelOffset(), symbol_offset);
        subchannel_offset = (assigned_subchannels + b->getSubchannelOffset()) % (phy->getNumsubchannels(UL_));
        txtime_s = (int)(ceil((double)(numsubchannels + subchannel_offset) / (double)phy->getNumsubchannels(UL_)));
        /*end*/

        ch->txtime() = txtime_s*phy->getSymbolTime ();

        debug2 ("\t   In station %d, 1.1 Check packet to send: con id in burst :%d, con id in connection :%d, size :%d, txtime :%f(%d), duration :%d(%f), numsubchnn :%d, subchnoff :%d\n",
                mac_->getNodeType(), c->get_cid(),b->getCid(), ch->size(),txtime, txtime_s, b->getDuration(), b->getDuration()*phy->getSymbolTime (),b->getnumSubchannels(),b->getSubchannelOffset ());

        wimaxHdr->phy_info.num_subchannels = numsubchannels;
        wimaxHdr->phy_info.subchannel_offset = initial_subchannel_offset;//subchannel_offset;
        wimaxHdr->phy_info.num_OFDMSymbol = txtime_s;
        wimaxHdr->phy_info.OFDMSymbol_offset = symbol_offset; //initial_offset;
        wimaxHdr->phy_info.channel_index = (mac_->getPeerNode_head())->getchannel();
        wimaxHdr->phy_info.direction = 1;

        debug2 ("\t   In station %d, 1.1 Packet phy header: numsymbols :%d, symboloffset :%d, subchanneloffset :%d, numsubchannel :%d, Channel_num :%d, direction :%d\n",
                mac_->getNodeType(), txtime_s,symbol_offset, subchannel_offset,numsubchannels,wimaxHdr->phy_info.channel_index,wimaxHdr->phy_info.direction);

        b->enqueue(p1);      //enqueue into burst
        // mac_->getMap()->getUlSubframe()->getBw_req()->removeRequest (c->get_cid());
        b_data += ch_pdu->size();
        return b_data;
    }
//#endif



    // Frag/pack subheader size is always equal to 2 bytes
    // For each MAC PDU Created, there is definitely MAC Header and Packet Sub Header and at least one ARQ Block
    //if (max_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE+ch_pdu->size ())
    debug2("ARQ retrans :The max_data is %d\n", max_data);
    if (max_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE) {
        debug2("ARQ: Not enough space even for one block and headers.\n");
        return b_data; //not even space for one block and headers
    }

    //If the Current window for ARQ is less than one, cannot transmit the packet
    debug2("ARQ1.0 Transfer_Packets PDU: Current window : %d , FSN: %d\n", (((c->getArqStatus()->getAckSeq()) + (c->getArqStatus()->getMaxWindow())) &0x7FF),(wimaxHdr_pdu->pack_subheader.sn));
    int seqno = wimaxHdr_pdu->pack_subheader.sn;
    int ack_seq = (c->getArqStatus()->getAckSeq());
    int max_window = (c->getArqStatus()->getMaxWindow());
    int max_seq =  (c->getArqStatus()->getMaxSeq());
    int curr_window = (((c->getArqStatus()->getAckSeq()) + (c->getArqStatus()->getMaxWindow())) &0x7FF);  // Get the right most of the ARQ slide window point

    debug2("xingting== seqno=%d,  ack_seq=%d,  max_window=%d,   max_seq=%d,  curr_window=%d\n",
           seqno,ack_seq,max_window, max_seq,curr_window);
    /*if the current ARQ block is outside the ARQ slide window, do not transfer it.*/
    //if((((seqno - ack_seq) > 0) && ((seqno - ack_seq) > max_window))
    if ((((seqno - ack_seq) > 0) && ((seqno - ack_seq) >= max_window))
            //|| ((seqno >= curr_window && seqno < max_window) && (ack_seq < max_seq && ack_seq >= (max_seq - max_window))))
            ||((seqno-ack_seq)<0 && ((seqno+max_seq-ack_seq)>=max_window))) {
        debug2("ARQ1.0 Transfer_Packets: Current window Low :curr_window_ : %d\n", (((c->getArqStatus()->getAckSeq()) + (c->getArqStatus()->getMaxWindow())) &0x7FF));
        debug2("ARQ1.0 Transfer_Packets: this ARQ block is outside the slide window. will not be transferred.\n");
        return b_data;
    }


    // MAC PDU will be the final packet to be transmitted
    //mac_pdu = mac_pdu->copy();
    wimaxHdr_pdu= HDR_MAC802_16(mac_pdu);
    ch_pdu = HDR_CMN(mac_pdu);

#if 0
    int gm_flag = 0;
    ch = HDR_CMN(p);
    wimaxHdr = HDR_MAC802_16(p);

    int more_bw = c->queueByteLength() - max_data + HDR_MAC802_16_SIZE + HDR_MAC802_16_FRAGSUB_SIZE;
    if (mac_->getNodeType()==STA_BS) {
        //     wimaxHdr->header.type_fbgm = false;
    } else {
        printf("SS ch-size :%d, max_data :%d, queue :%d\n", ch->size(),  max_data, c->queueByteLength());
        if (max_data < c->queueByteLength()+ GENERIC_HEADER_SIZE + HDR_MAC802_16_FRAGSUB_SIZE) {
            wimaxHdr->header.type_fbgm = true;
            wimaxHdr->grant_subheader.piggyback_req = more_bw;
            ch->size() = ch->size()+HDR_MAC802_16_FRAGSUB_SIZE; //new packet size
            debug10 ("In station %d Piggybacking flag :%d, queue_bytes :%d, max_data :%d, askmore :%d\n", mac_->getNodeType(), wimaxHdr->header.type_fbgm, c->queueByteLength(), max_data, more_bw);
            gm_flag = 2;
        } else {
            wimaxHdr->header.type_fbgm = false;
            printf("SS no gm :%d, maxdata :%d, queue :%d\n", wimaxHdr->header.type_fbgm, max_data, c->queueByteLength());
        }
    }
#endif


    //We can remove the packet from the queue now
    if (c->getArqStatus () != NULL && c->getArqStatus()->isArqEnabled() == 1) {
        if ((c->getArqStatus ()->arq_retrans_queue_) && (c->getArqStatus ()->arq_retrans_queue_->length() > 0)) {
            debug2("ARQ Transfer Packets: Transferring a Retransmission packet\n");
            p = c->getArqStatus ()->arq_retrans_queue_->deque();
        } else
            p = c->dequeue();
    } else
        p = c->dequeue();

    //set header information
    wimaxHdr_pdu->header.ht = 0;
    wimaxHdr_pdu->header.ec = 1;
    wimaxHdr_pdu->header.type_mesh = 0;
    wimaxHdr_pdu->header.type_ext = 1; /*ARQ is enabled */
    wimaxHdr_pdu->header.type_frag = 0;
    wimaxHdr_pdu->header.type_pck = 1; /*Packing header is present */
    wimaxHdr_pdu->header.type_fbgm = 0;
    wimaxHdr_pdu->header.ci = 1;
    wimaxHdr_pdu->header.eks = 0;
    wimaxHdr_pdu->header.cid = c->get_cid ();
    wimaxHdr_pdu->header.hcs = 0;
    ch_pdu->size() += HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE+HDR_MAC802_16_CRC_SIZE; /*Size of generic Mac Header and CRC is set (Note one packet header and first ARQ Block is present)*/
    debug2 ("ARQ: MAC [%d] Generated PDU Size: %d FSN is %d \n",mac_->addr(),ch_pdu->size (),  wimaxHdr_pdu->pack_subheader.sn);

    // Set the Initial Conditions
    p_temp = mac_pdu;
    wimaxHdr_temp= HDR_MAC802_16(mac_pdu);
    ch_temp = HDR_CMN(mac_pdu);
    debug2 ("ARQ: MAX_data %d \t MAC %d PDU: ARQ Fragment Added... PDU Size: %d FSN is %d re-trans-Q len is %d  data-trans-Q len is %d\n",
            max_data, mac_->addr(),ch_temp->size (),  wimaxHdr_temp->pack_subheader.sn, c->getArqStatus ()->arq_retrans_queue_->length()+1, c->get_queue()->length()+1);

    if (c->getArqStatus () != NULL && c->getArqStatus ()->isArqEnabled() == 1) {
        if ((c->getArqStatus ()->arq_retrans_queue_) && (c->getArqStatus ()->arq_retrans_queue_->length() > 0)) {
            debug2("ARQ Transfer Packets: Transferring a Retransmission packet\n");
            p = c->getArqStatus ()->arq_retrans_queue_->head();
        } else
            p = c->get_queue()->head();
    } else
        p = c->get_queue()->head();

    while (p) {
        ch = HDR_CMN(p);
        wimaxHdr = HDR_MAC802_16(p);

        if (mac_->getNodeType()==STA_BS)
            max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_) - b_data;
        else
            max_data = phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_) - b_data;

        debug2 ("ARQ : In Mac %d max data=%d (burst duration=%d, b_data=%d) PDU SIZE = %d\n", mac_->addr(), max_data, b->getDuration(), b_data, ch_pdu->size());

        if ((wimaxHdr->pack_subheader.fc == FRAG_FIRST)
                || (((wimaxHdr->pack_subheader.sn - wimaxHdr_temp->pack_subheader.sn) &0x7FF)!= 1)
                || (wimaxHdr->pack_subheader.fc == FRAG_NOFRAG)) {
            //if (max_data < ch_pdu->size()+ ch->size()+HDR_MAC802_16_FRAGSUB_SIZE)  // why at less need to fit in 2 blocks?
            if (max_data < ch_pdu->size()+HDR_MAC802_16_FRAGSUB_SIZE) { // why at less need to fit in 2 blocks?
                debug2("Can not fit in 2 blocks + frag Subheader in this data burst.\n");
                break; //cannot fit in more
            }
        } else {
            //if (max_data < ch_pdu->size()+ ch->size())   // why at less need to fit in 2 blocks??
            if (max_data < ch_pdu->size()) {
                debug2("Can not fit in 2 blocks in this data burst.\n");
                break; //cannot fit in more
            }
        }

        // If the Current window for ARQ is less than one, cannot transmit the packet
        debug2("ARQ2.0 Transfer_Packets: Current window : %d , FSN: %d\n", (((c->getArqStatus()->getAckSeq()) + (c->getArqStatus()->getMaxWindow())) &0x7FF),(wimaxHdr->pack_subheader.sn));
        int seqno = wimaxHdr->pack_subheader.sn;
        int ack_seq = (c->getArqStatus()->getAckSeq());
        int max_window = (c->getArqStatus()->getMaxWindow());
        int max_seq =  (c->getArqStatus()->getMaxSeq());
        // int curr_window = (((c->getArqStatus()->getAckSeq()) + (c->getArqStatus()->getMaxWindow())) &0x7FF);
        if ((((seqno - ack_seq) > 0) && ((seqno - ack_seq) > max_window))
                //|| ((seqno >= curr_window && seqno < max_window) && (ack_seq < max_seq && ack_seq >= (max_seq - max_window))))
                ||((seqno-ack_seq)<0 && ((seqno+max_seq-ack_seq)>max_window))) {
        	debug2("ARQ2.0 Transfer_Packets: seqno=%d, ack_seq=%d, 1st judge=%d,  2nd judge=%d",
                   seqno,ack_seq,(((seqno - ack_seq) > 0) && ((seqno - ack_seq) > max_window)),((seqno-ack_seq)<0 && ((seqno+max_seq-ack_seq)>max_window)));
            debug2("ARQ2.0 Transfer_Packets: this ARQ block is outside of ARQ slide window, Will not be transferred.\n ");
            break;
        }

        p_temp->allocdata (sizeof (class Packet));
        p_current = (Packet*) p_temp->accessdata();

        HDR_CMN(p)->direction() = hdr_cmn::UP; //to be received properly

        // Copies the entire contents of the current packet into data part of the previous packet
        memcpy(p_current, p, sizeof (class Packet));
        ch_current = HDR_CMN(p_current);
        wimaxHdr_current = HDR_MAC802_16(p_current);

        // Update the Mac PDU Size
        if ((wimaxHdr->pack_subheader.fc == FRAG_FIRST) || (((wimaxHdr->pack_subheader.sn - wimaxHdr_temp->pack_subheader.sn) &0x7FF)!= 1)|| (wimaxHdr->pack_subheader.fc == FRAG_NOFRAG))
            ch_pdu->size() += ch->size()+HDR_MAC802_16_FRAGSUB_SIZE ;
        else
            ch_pdu->size() += ch->size();

        debug2 ("ARQ: MAC PDU: ARQ Fragment Added... PDU Size: %d FSN is %d \n",ch_pdu->size (),  wimaxHdr_current->pack_subheader.sn);

        // Set the Conditions
        p_temp = p_current;
        wimaxHdr_temp= wimaxHdr_current;
        ch_temp = ch_current;

        // Remove the packet from the queue
        if (c->getArqStatus () != NULL && c->getArqStatus()->isArqEnabled() == 1) {
            if ((c->getArqStatus ()->arq_retrans_queue_) && (c->getArqStatus ()->arq_retrans_queue_->length() > 0)) {
                debug2("ARQ Transfer Packets: Transferring a Retransmission packet\n");
                p = c->getArqStatus ()->arq_retrans_queue_->deque();
            } else
                p = c->dequeue();
        } else
            p = c->dequeue();

        // Fetch the next packet
        if (c->getArqStatus () != NULL && c->getArqStatus ()->isArqEnabled() == 1) {
            if ((c->getArqStatus ()->arq_retrans_queue_) && (c->getArqStatus ()->arq_retrans_queue_->length() > 0)) {
                debug2("ARQ Transfer Packets: Transferring a Retransmission packet\n");
                p = c->getArqStatus ()->arq_retrans_queue_->head();
            } else
                p = c->get_queue()->head();
        } else
            p = c->get_queue()->head();
    }
    debug2 ("ARQ: MAC PDU Generated Size : %d \n", ch_pdu->size());
    int assigned_subchannels;
    int symbol_offset;
    //Trying to calculate how to fill up things here? Before this, frag and arq business?
    if (mac_->getNodeType()==STA_BS) {
        assigned_subchannels = phy->getNumSubchannels(b_data, mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_);
        numsubchannels = phy->getNumSubchannels(ch_pdu->size(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_);
        symbol_offset = b->getStarttime() + (b->getSubchannelOffset()+assigned_subchannels)/phy->getNumsubchannels(DL_);

        if ((numsubchannels+assigned_subchannels) > (phy->getNumsubchannels(DL_) - b->getSubchannelOffset () + 1)) { //its crossing over to the next symbol
            txtime_s = num_symbol_per_slot + (int) ((ceil((numsubchannels + assigned_subchannels - (phy->getNumsubchannels(DL_) - b->getSubchannelOffset () + 1)) / (double)phy->getNumsubchannels(DL_)))) * num_symbol_per_slot;
            offset = b->getStarttime( )+ txtime_s-num_symbol_per_slot;
            subchannel_offset = (int) ceil((numsubchannels + assigned_subchannels - (phy->getNumsubchannels(DL_) - b->getSubchannelOffset () + 1)) % (phy->getNumsubchannels(DL_)));
            subchannel_offset++;
        } else {
            txtime_s=num_symbol_per_slot;
            subchannel_offset = numsubchannels + assigned_subchannels;
        }

        debug2("burst subch offset %d, symbol offset %d\n", b->getSubchannelOffset(), b->getStarttime());
        subchannel_offset = ( b->getSubchannelOffset()+assigned_subchannels)%(phy->getNumsubchannels(DL_))+1;
        txtime_s = (int) ceil((double)(subchannel_offset + numsubchannels)/(double)phy->getNumsubchannels(DL_))*num_symbol_per_slot;

    } else { //if its the SS
        assigned_subchannels = phy->getNumSubchannels(b_data, mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);

        numsubchannels = phy->getNumSubchannels(ch_pdu->size(), mac_->getMap()->getUlSubframe()->getProfile (b->getIUC())->getEncoding(), UL_);
        symbol_offset = b->getStarttime() + (b->getSubchannelOffset()+assigned_subchannels)/phy->getNumsubchannels(UL_);
        debug2("assigned_subch_num %d,  alloc_subch_num %d  symbol_off %d\n", assigned_subchannels, numsubchannels, symbol_offset);
        debug2("burst subch offset %d, symbol offset %d\n", b->getSubchannelOffset(), b->getStarttime());
        subchannel_offset = ( b->getSubchannelOffset()+assigned_subchannels)%(phy->getNumsubchannels(UL_))+1;
        txtime_s = (int) ceil((double)(subchannel_offset + numsubchannels)/(double)phy->getNumsubchannels(UL_))*num_symbol_per_slot;
        debug2("subchan offset %d,  symbol num %d\n", subchannel_offset, txtime_s);
    }


    ch_pdu->txtime() = txtime_s*phy->getSymbolTime ();

    debug10 ("In station %d 1.2 Check packet to send: con id in burst = %d con id in connection = %d data_size=%d txtime=%f(%d) burst_duration=%d(%f) burst_numsubchnn = %d burst_subchnoff = %d\n",
             mac_->getNodeType(), c->get_cid(),b->getCid(), ch_pdu->size(),txtime, txtime_s, b->getDuration(), b->getDuration()*phy->getSymbolTime (),b->getnumSubchannels(),b->getSubchannelOffset ());

    // rpi start put phy header info - like num subchannels and ofdmsymbols....

    wimaxHdr_pdu->phy_info.num_subchannels = numsubchannels;
    wimaxHdr_pdu->phy_info.subchannel_offset = subchannel_offset;
    wimaxHdr_pdu->phy_info.num_OFDMSymbol = txtime_s;
    wimaxHdr_pdu->phy_info.OFDMSymbol_offset = symbol_offset; //initial_offset;
    /*  if (mac_->getNodeType()==STA_BS)
        {
          wimaxHdr_pdu->phy_info.direction = 0;
          wimaxHdr_pdu->phy_info.channel_index = c->getPeerNode()->getchannel();
        }
      else
        {
          wimaxHdr_pdu->phy_info.channel_index = (mac_->getPeerNode_head())->getchannel();
          wimaxHdr_pdu->phy_info.direction = 1;
        }*/

    if (mac_->getNodeType()==STA_BS) {
        wimaxHdr_pdu->phy_info.direction = 0;

        if ( c->getPeerNode() ==NULL) {
            debug2("connection %d does not have peer node.\n", c->get_cid());
        } else {
            debug2("Peer Node is %d\n", c->getPeerNode()->getAddr());
            wimaxHdr_pdu->phy_info.channel_index = c->getPeerNode()->getchannel();
            if (c->getPeerNode() && mac_->amc_enable_ == 1) {
                /*Xingting added to support MCS. Here is used to change the MCS index.*/
                debug2("In transfer function, BS [%d] set SS[%d], flag is %d\n",mac_->addr(), c->getPeerNode()->getAddr(), mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()));
                if (mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()) == TRUE && mac_->amc_enable_ == 1) {
                    debug2("in transfer function, BS set SS[%d] gonna set mcs index to %d\n",c->getPeerNode()->getAddr(),c->getPeerNode()->getCurrentMcsIndex());
                    wimaxHdr_pdu->phy_info.mcs_index_ = c->getPeerNode()->getCurrentMcsIndex();
                    mac_->set_change_modulation_flag(c->getPeerNode()->getAddr(), FALSE);
                } else {
                    debug2("in transfer function, no change modulation. amc_enable %d   But MCS index set to %d  SS [%d]  get_change_modulation_flag[%d] .\n",
                           mac_->amc_enable_,  c->getPeerNode()->getCurrentMcsIndex(), c->getPeerNode()->getAddr(), mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()));
                    wimaxHdr_pdu->phy_info.mcs_index_  = c->getPeerNode()->getCurrentMcsIndex(); //-1;
                }
            }
        }
    } else {
        debug2("33333333\n");
        wimaxHdr_pdu->phy_info.channel_index = (mac_->getPeerNode_head())->getchannel();
        wimaxHdr_pdu->phy_info.direction = 1;
    }



    debug10 ("In station %d Packet phy header : numsymbols =%d , symboloffset =%d,  subchanneloffset= %d, numsubchannel = %d, Channel_num = %d, direction =%d \n", mac_->getNodeType(), txtime_s,initial_offset, initial_subchannel_offset,numsubchannels,wimaxHdr_pdu->phy_info.channel_index,wimaxHdr_pdu->phy_info.direction);


    initial_offset = offset;
    initial_subchannel_offset = subchannel_offset;

    b->enqueue(mac_pdu);      //enqueue into burst
    b_data += ch_pdu->size(); //increment amount of data enqueued
    debug2 ("In station %d packet enqueued b_data = %d" , mac_->getNodeType(), b_data);
    if (!pkt_transfered && mac_->getNodeType()!=STA_BS) { //if we transfert at least one packet, remove bw request
        pkt_transfered = true;
        mac_->getMap()->getUlSubframe()->getBw_req()->removeRequest (c->get_cid());
    }

    return b_data;
}

/**
 * Returns the statistic for the downlink scheduling
 */
frameUsageStat_t WimaxScheduler::getDownlinkStatistic()
{
	// set default values an return them
	frameUsageStat_t dummyStat;
	dummyStat.totalNbOfSlots = 100.0;
	dummyStat.usedMrtrSlots = 0.0;
	dummyStat.usedMstrSlots = 0.0;
	return dummyStat;
}


/*
 * Returns the statistic for the uplink scheduling
 */
frameUsageStat_t WimaxScheduler::getUplinkStatistic()
{
	// set default values an return them
	frameUsageStat_t dummyStat;
	dummyStat.totalNbOfSlots = 100.0;
	dummyStat.usedMrtrSlots = 0.0;
	dummyStat.usedMstrSlots = 0.0;
	return dummyStat;
}

/**
 * Common part in the BS and MS scheduling process to add ARQ feedback information
 * to data connection or enqueue a ARQ feedback packet to basic out connection
 */
void WimaxScheduler::sendArqFeedbackInformation()
{
	// We will try to Fill in the ARQ Feedback Information now...
	// TODO: Check for correctness vr@tud
	for (Connection * currentCon = mac_->getCManager ()->get_in_connection (); currentCon; currentCon = currentCon->next_entry()) {
		if ( ( ( currentCon->getArqStatus () != NULL) && ( currentCon->getArqStatus ()->isArqEnabled() == 1) ) &&
			( (currentCon->getArqStatus()->arq_feedback_queue_) && (currentCon->getArqStatus()->arq_feedback_queue_->length() > 0)) ) {
		   // there are arq data to send
			PeerNode * peerNode = currentCon->getPeerNode ();

		   // check if arq feedback in dl data is enabeled
		   if ( getMac()->isArqFbinDlData()) {
			   // find data connection with availible data packets
			   int i = 0;
			   while ( (peerNode->getOutDataCon( i)) &&  ( peerNode->getOutDataCon( i)->queueLength() > 0 ) ) {
				   i++;
			   }
			   // check if a data connection has been found
			   if ( peerNode->getOutDataCon( i)) {
				   Connection * outDataCon = peerNode->getOutDataCon( i);
				   debug2("ARQ BS : Feedback in data Cid %d\n", outDataCon->get_cid());
				   Packet * dataPacket = outDataCon->dequeue();
				   hdr_mac802_16 * dataHeader = HDR_MAC802_16( dataPacket);
				   // is arq feedback pressent
				   if ( dataHeader->header.type_arqfb == 0) {
					   debug2("ARQ BS : Feedback in data cid %d", outDataCon->get_cid());
					   dataHeader->header.type_arqfb = 1;
					   dataHeader->num_of_acks = 0;

					   // get feedback packet
					   Packet * feedbackPacket = currentCon->getArqStatus()->arq_feedback_queue_->deque();
					   hdr_mac802_16 * feedbackHeader = HDR_MAC802_16( feedbackPacket);
					   // copy feedback header to data header
					   for ( int nbOfAcks = 0; nbOfAcks < feedbackHeader->num_of_acks; nbOfAcks++) {
						   dataHeader->arq_ie[ nbOfAcks].cid = feedbackHeader->arq_ie[ nbOfAcks].cid;
						   dataHeader->arq_ie[ nbOfAcks].last = feedbackHeader->arq_ie[ nbOfAcks].last;
						   dataHeader->arq_ie[ nbOfAcks].ack_type = feedbackHeader->arq_ie[ nbOfAcks].ack_type;
						   dataHeader->arq_ie[ nbOfAcks].fsn = feedbackHeader->arq_ie[ nbOfAcks].fsn;
					   }
					   dataHeader->num_of_acks = feedbackHeader->num_of_acks;
					   // increase size of data packet
					   HDR_CMN( dataPacket)->size() += (feedbackHeader->num_of_acks * HDR_MAC802_16_ARQFEEDBK_SIZE);

					   // enque packet in data queue
					   outDataCon->enqueue_head( dataPacket);

				   } else {
					   debug2("ARQ BS: Feedback already present, do nothing \n");
					   outDataCon->enqueue_head( dataPacket);
				   }

			   } else {
				   // no data connection to transport arq feedback
				   Connection * outBasicCon = peerNode->getBasic (OUT_CONNECTION);
				   Packet * basicPacket = mac_->getPacket();
				   hdr_mac802_16 * basicHeader = HDR_MAC802_16( basicPacket);
				   basicHeader->header.cid = outBasicCon->get_cid();
				   basicHeader->num_of_acks = 0;

				   // get feedback packet
				   Packet * feedbackPacket = currentCon->getArqStatus()->arq_feedback_queue_->deque();
				   hdr_mac802_16 * feedbackHeader = HDR_MAC802_16( feedbackPacket);
				   // copy feedback header to data header
				   for ( int nbOfAcks = 0; nbOfAcks < feedbackHeader->num_of_acks; nbOfAcks++) {
					   basicHeader->arq_ie[ nbOfAcks].cid = feedbackHeader->arq_ie[ nbOfAcks].cid;
					   basicHeader->arq_ie[ nbOfAcks].last = feedbackHeader->arq_ie[ nbOfAcks].last;
					   basicHeader->arq_ie[ nbOfAcks].ack_type = feedbackHeader->arq_ie[ nbOfAcks].ack_type;
					   basicHeader->arq_ie[ nbOfAcks].fsn = feedbackHeader->arq_ie[ nbOfAcks].fsn;
				   }
				   basicHeader->num_of_acks = feedbackHeader->num_of_acks;
				   // increase size of data packet
				   HDR_CMN( basicPacket)->size() += (feedbackHeader->num_of_acks * HDR_MAC802_16_ARQFEEDBK_SIZE);

				   // enque packet in sending queue
				   outBasicCon->enqueue( basicPacket);
			   }

		   } else {
			   // No arq feedback in data enabled -> send feedback on basic cid
			   Connection * outBasicCon = peerNode->getBasic (OUT_CONNECTION);
			   Packet * basicPacket = mac_->getPacket();
			   hdr_mac802_16 * basicHeader = HDR_MAC802_16( basicPacket);
			   basicHeader->header.cid = outBasicCon->get_cid();
			   basicHeader->num_of_acks = 0;

			   // get feedback packet
			   Packet * feedbackPacket = currentCon->getArqStatus()->arq_feedback_queue_->deque();
			   hdr_mac802_16 * feedbackHeader = HDR_MAC802_16( feedbackPacket);
			   // copy feedback header to data header
			   for ( int nbOfAcks = 0; nbOfAcks < feedbackHeader->num_of_acks; nbOfAcks++) {
				   basicHeader->arq_ie[ nbOfAcks].cid = feedbackHeader->arq_ie[ nbOfAcks].cid;
				   basicHeader->arq_ie[ nbOfAcks].last = feedbackHeader->arq_ie[ nbOfAcks].last;
				   basicHeader->arq_ie[ nbOfAcks].ack_type = feedbackHeader->arq_ie[ nbOfAcks].ack_type;
				   basicHeader->arq_ie[ nbOfAcks].fsn = feedbackHeader->arq_ie[ nbOfAcks].fsn;
			   }
			   basicHeader->num_of_acks = feedbackHeader->num_of_acks;
			   // increase size of data packet
			   HDR_CMN( basicPacket)->size() += (feedbackHeader->num_of_acks * HDR_MAC802_16_ARQFEEDBK_SIZE);

			   // enque packet in sending queue
			   outBasicCon->enqueue( basicPacket);
		   }
	   }
	}
}


/**
 * Transfer the packets from the given connection to the given burst
 * @param con The connection
 * @param b The burst
 * @param b_data Amount of data in the burst
 * @return the new burst occupation
 */
int WimaxScheduler::transferPacketsDownlinkBurst (Connection * c, Burst * b, int b_data)
{
    Packet *p;
    hdr_cmn* ch;
    hdr_mac802_16 *wimaxHdr;
    OFDMAPhy *phy = mac_->getPhy();
    PeerNode *peer;

    int num_symbol_per_slot;

    if (mac_->amc_enable_ ==1 &&  c->getPeerNode() != NULL ) {
        debug2("CID %d the current MCS index is %d\n", c->get_cid(),c->getPeerNode()->getCurrentMcsIndex());
    }


    if (mac_->getNodeType()==STA_BS) {
        num_symbol_per_slot = phy->getSlotLength (DL_);
    } else {
        num_symbol_per_slot = phy->getSlotLength (UL_);

        // this function is only for downlink burst
        assert(false);
    }

    debug10 ("\nEntered transfer_packets1 for MacADDR :%d, for connection :%d with bdata  :%d, #subchannels :%d, #symbol :%d, IUC :%d, cqichSlotFlag %d\n",
             mac_->addr(), c->get_cid(), b_data, b->getnumSubchannels(), b->getDuration(), b->getIUC(), b->getCqichSlotFlag());


    peer = mac_->getPeerNode_head();
    p = c->get_queue()->head();

    // connection should have packets to send
    assert ( p != NULL);

    int max_data;

    max_data = phy->getMaxPktSize (b->getnumSubchannels() * b->getDuration() / phy->getSlotLength(DL_), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_) - b_data;

    debug10 ("\tBS_transfer1.1, CID :%d with bdata :%d, but MaxData (MaxSize-bdata) :%d, MaxSize :%d, q->bytes :%d\n", c->get_cid(), b_data, max_data, phy->getMaxPktSize (b->getnumSubchannels(), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_), c->queueByteLength() );
    debug10 ("\t   Bduration :%d, Biuc :%d, B#subchannel :%d CqichSlotFlag %d\n",b->getDuration(), b->getIUC(), b->getnumSubchannels(), b->getCqichSlotFlag());


    // handle fragmentation
    if (max_data < HDR_MAC802_16_SIZE ||(c->getFragmentationStatus()!=FRAG_NOFRAG
                                         && max_data < HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE)) {
        debug2("In station %d returning as no space\n", mac_->getNodeType());
        return b_data; //not even space for header
    }


    // transfer packets

    while (p) {
    	debug10 ("In station %d Entering while loop, connection type %d, CID :%d\n", mac_->getNodeType(), c->get_category(), c->get_cid());
        ch = HDR_CMN(p);
        wimaxHdr = HDR_MAC802_16(p);

        if (ch->size() < 0 ) {
        	debug2(" packet size negative --- panic!!! ");
        }

        max_data = phy->getMaxPktSize (b->getnumSubchannels() * b->getDuration() / phy->getSlotLength(DL_), mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_) - b_data;

        if (max_data <= HDR_MAC802_16_SIZE ||	(c->getFragmentationStatus()!=FRAG_NOFRAG && max_data <= HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE)) {
            return b_data; //not even space for header
        }

        if (c->getFragmentationStatus()==FRAG_NOFRAG && max_data <= HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE && (ch->size()>HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE) ) {
            return b_data; //not even space for header
        }

        if (c->getFragmentationStatus()!=FRAG_NOFRAG) {
            if (max_data >= ch->size()-c->getFragmentBytes() + HDR_MAC802_16_FRAGSUB_SIZE) { //getFrag => include MACHEADER
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                //no need to fragment again
                wimaxHdr->frag_subheader.fc = FRAG_LAST;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                int more_bw = 0;


                c->dequeue();  /*remove packet from queue */
                ch->size() = ch->size()-c->getFragmentBytes()+HDR_MAC802_16_FRAGSUB_SIZE; //new packet size
                //update fragmentation
                if (ch->size() < 0 )
                    debug2(" packet size negative -- panic !!! \n");

                debug2 ("\nEnd of fragmentation %d, CID :%d, (max_data :%d, bytes to send :%d, getFragmentBytes :%d, getFragNumber :%d, updated Frag :%d, update FragBytes :%d, con->qBytes :%d, con->qlen :%d, more_bw :%d)\n", wimaxHdr->frag_subheader.fsn, c->get_cid(), max_data, ch->size(), c->getFragmentBytes(), c->getFragmentNumber(), 0, 0, c->queueByteLength(), c->queueLength(), more_bw);

                c->updateFragmentation (FRAG_NOFRAG, 0, 0);

            } else {
                //need to fragment the packet again
                p = p->copy(); //copy packet to send
                ch = HDR_CMN(p);
                wimaxHdr = HDR_MAC802_16(p);
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                wimaxHdr->frag_subheader.fc = FRAG_CONT;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                int more_bw = 0;
                ch->size() = max_data; //new packet size

                //update fragmentation

                debug2 ("\nContinue fragmentation %d, CID :%d, (max_data :%d, bytes to send :%d, getFragmentBytes :%d, getFragNumber :%d, updated Frag :%d, update FragBytes :%d, con->qBytes :%d, con->qlen :%d, more_bw :%d)\n", wimaxHdr->frag_subheader.fsn, c->get_cid(), max_data, ch->size(),  c->getFragmentBytes(), c->getFragmentNumber(), (c->getFragmentNumber()+1)%8, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE), c->queueByteLength(), c->queueLength(), more_bw);

                c->updateFragmentation (FRAG_CONT, (c->getFragmentNumber ()+1)%8, c->getFragmentBytes() + max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE));
            }
        } else {//else no flag
            if (max_data < ch->size() && c->isFragEnable()) {
                //need to fragment the packet for the first time
                p = p->copy(); //copy packet to send
                ch = HDR_CMN(p);
                int ori_ch = ch->size();
                wimaxHdr = HDR_MAC802_16(p);
                //add fragmentation header
                wimaxHdr->header.type_frag = true;
                wimaxHdr->frag_subheader.fc = FRAG_FIRST;
                wimaxHdr->frag_subheader.fsn = c->getFragmentNumber ();
                int more_bw = 0;
                ch->size() = max_data; //new packet size

                debug2 ("\nFirst fragmentation %d, CID :%d, (max_data :%d, bytes to send :%d, ori_size :%d, getFragmentBytes :%d, FRAGSUB :%d, getFragNumber :%d, updated Frag ;%d, update FragBytes :%d, con->qBytes :%d, con->qlen :%d, more_bw :%d)\n", wimaxHdr->frag_subheader.fsn, c->get_cid(), max_data, ch->size(), ori_ch, c->getFragmentBytes(), HDR_MAC802_16_FRAGSUB_SIZE, c->getFragmentNumber (), 1, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE),c->queueByteLength(), c->queueLength (), more_bw);

                c->updateFragmentation (FRAG_FIRST, 1, c->getFragmentBytes()+max_data-(HDR_MAC802_16_SIZE+HDR_MAC802_16_FRAGSUB_SIZE));

            } else if (max_data < ch->size() && !c->isFragEnable()) {
                //the connection does not support fragmentation
                //can't move packets anymore
                return b_data;
            } else {
                //no fragmentation necessary
                int more_bw = 0;

                debug2 ("\nNo fragmentation %d, (max_data :%d, bytes to send :%d, con->qBytes :%d, con->qlen :%d, more_bw :%d\n", wimaxHdr->frag_subheader.fsn, max_data, ch->size(), c->queueByteLength(), c->queueLength(), more_bw);
                c->dequeue();

            }//end frag
        }

        int packetSize = ch->size();

        int startSlot = phy->getNumSubchannels( b_data, mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_);
        int endSlot = phy->getNumSubchannels( b_data + packetSize, mac_->getMap()->getDlSubframe()->getProfile (b->getIUC())->getEncoding(), DL_);

        int numOfSubchannels = endSlot - startSlot;

        int symbolOffset = b->getStarttime() + num_symbol_per_slot * int( floor( double(startSlot) / b->getnumSubchannels()));
        int numOfSymbols = b->getStarttime() + num_symbol_per_slot * int( ceil( double(endSlot) / b->getnumSubchannels())) - symbolOffset;

        debug2("Packet with Size %d, will be allocated from slot %d to slot %d using %d symbols \n", packetSize, startSlot, endSlot, numOfSymbols);


        // sending time of this an other packets
        ch->txtime() = numOfSymbols * phy->getSymbolTime ();


        wimaxHdr = HDR_MAC802_16(p);
        wimaxHdr->phy_info.num_subchannels = numOfSubchannels;
        wimaxHdr->phy_info.subchannel_offset = startSlot;//subchannel_offset within burst;
        wimaxHdr->phy_info.num_OFDMSymbol = numOfSymbols;
        wimaxHdr->phy_info.OFDMSymbol_offset = symbolOffset; //initial_offset;
        wimaxHdr->phy_info.subchannel_lbound = b->getSubchannelOffset();
        wimaxHdr->phy_info.subcarrier_ubound = b->getSubchannelOffset() + b->getnumSubchannels();



        debug10("In transfer_packets1-- packets info: symbol_off[%d]\t symbol_#[%d]\t subch_Off[%d]\t subch_#[%d]\n",
                wimaxHdr->phy_info.OFDMSymbol_offset,wimaxHdr->phy_info.num_OFDMSymbol,
                wimaxHdr->phy_info.subchannel_offset,wimaxHdr->phy_info.num_subchannels);

        //wimaxHdr->phy_info.channel_index = c->getPeerNode()->getchannel();
        wimaxHdr->phy_info.direction = 0;
		// debug2("Peer Node is %d\n", c->getPeerNode());
		if (c->getPeerNode()) {
			wimaxHdr->phy_info.channel_index = c->getPeerNode()->getchannel();
			/*Xingting added to support MCS. Here is used to change the MCS index.*/
			debug2("In transfer function, BS [%d] set SS[%d], flag is %d\n",mac_->addr(), c->getPeerNode()->getAddr(), mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()));
			if (mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()) == TRUE && mac_->amc_enable_ == 1) {
				debug2("in transfer function, BS set SS[%d] gonna set mcs index to %d\n",c->getPeerNode()->getAddr(),c->getPeerNode()->getCurrentMcsIndex());
				wimaxHdr->phy_info.mcs_index_ = c->getPeerNode()->getCurrentMcsIndex();
				mac_->set_change_modulation_flag(c->getPeerNode()->getAddr(), FALSE);
			} else {
				debug2("in transfer function, no change modulation. amc_enable %d   But MCS index set to %d  SS [%d]  get_change_modulation_flag[%d] .\n",
					   mac_->amc_enable_,  c->getPeerNode()->getCurrentMcsIndex(), c->getPeerNode()->getAddr(), mac_->get_change_modulation_flag(c->getPeerNode()->getAddr()));
				wimaxHdr->phy_info.mcs_index_  = c->getPeerNode()->getCurrentMcsIndex(); //-1;
			}
		}



        b->enqueue(p);      //enqueue into burst

        b_data += ch->size(); //increment amount of data enqueued
        debug2 ("In station %d packet enqueued b_data = %d \n", mac_->getNodeType(), b_data);

        p = c->get_queue()->head(); //get new head
    }
    return b_data;
}
