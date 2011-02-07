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

#include "serviceflowhandler.h"
#include "mac802_16.h"
#include "scheduling/wimaxscheduler.h"
#include "arqstatus.h"

#define DEFAULT_POLLING_INTERVAL 1000

static int TransactionID = 0;
/*
 * Create a service flow
 * @param mac The Mac where it is located
 */
ServiceFlowHandler::ServiceFlowHandler ()
{
    LIST_INIT (&flow_head_);
    LIST_INIT (&pendingflow_head_);
    LIST_INIT (&static_flow_head_);
}

/*
 * Set the mac it is located in
 * @param mac The mac it is located in
 */
void ServiceFlowHandler::setMac (Mac802_16 *mac)
{
    assert (mac);

    mac_ = mac;
}

/**
 * Process the given packet. Only service related packets must be sent here.
 * @param p The packet received
 */
void ServiceFlowHandler::process (Packet * p)
{
    hdr_mac802_16 *wimaxHdr = HDR_MAC802_16(p);
    gen_mac_header_t header = wimaxHdr->header;

    //we cast to this frame because all management frame start with
    //a type
    mac802_16_dl_map_frame *frame = (mac802_16_dl_map_frame*) p->accessdata();

    switch (frame->type) {
    case MAC_DSA_REQ:
        processDSA_req (p);
        break;
    case MAC_DSA_RSP:
        processDSA_rsp (p);
        break;
    case MAC_DSA_ACK:
        processDSA_ack (p);
        break;
    default:
        debug2 ("Unknow frame type (%d) in flow handler\n", frame->type);
    }
    //Packet::free (p);
}

/**
 * Add a flow with the given qos
 * @param qos The QoS for the flow
 * @return the created ServiceFlow
 */
ServiceFlow* ServiceFlowHandler::addFlow (ServiceFlowQosSet * qosSet)
{
    return NULL;
}

/**
 * Remove the flow given its id
 * @param id The flow id
 */
void ServiceFlowHandler::removeFlow (int id)
{

}

/**
 * Send a flow request to the given node
 * @param index The node address
 * @param out The flow direction
 */
void ServiceFlowHandler::sendFlowRequest (int index, bool out)
{
    Packet *p;
    struct hdr_cmn *ch;
    hdr_mac802_16 *wimaxHdr;
    mac802_16_dsa_req_frame *dsa_frame;
    PeerNode *peer;

    //create packet for request
    peer = mac_->getPeerNode(index);
    p = mac_->getPacket ();
    ch = HDR_CMN(p);
    wimaxHdr = HDR_MAC802_16(p);
    p->allocdata (sizeof (struct mac802_16_dsa_req_frame));
    dsa_frame = (mac802_16_dsa_req_frame*) p->accessdata();
    dsa_frame->type = MAC_DSA_REQ;
    dsa_frame->uplink = (out && mac_->getNodeType()==STA_MN) || (!out && mac_->getNodeType()==STA_BS) ;
    dsa_frame->transaction_id = TransactionID++;
    debug2(" sampad in send flow request");
    if (mac_->getNodeType()==STA_MN)
        ch->size() += GET_DSA_REQ_SIZE (0);
    else {
        //assign a CID and include it in the message
        Connection *data = new Connection (CONN_DATA);
        /*
            data->setCDMA(0);
            data->initCDMA();
            data->setPOLL_interval(0);
        */

        mac_->getCManager()->add_connection (data, out);
        if (out) {
            peer->addOutDataCon (data);
            debug2("set outcoming data connection for mac %d\n", mac_->addr());
        } else {
            peer->addInDataCon (data);
            debug2("set incoming data connection for mac %d\n", mac_->addr());
        }
        dsa_frame->cid = data->get_cid();
        ch->size() += GET_DSA_REQ_SIZE (1);
    }

    wimaxHdr->header.cid = peer->getPrimary(OUT_CONNECTION)->get_cid();
    peer->getPrimary(OUT_CONNECTION)->enqueue (p);
}

/**
 * process a flow request
 * @param p The received request
 */
void ServiceFlowHandler::processDSA_req (Packet *p)
{
    mac_->debug ("At %f in Mac %d received DSA request from %d\n", NOW, mac_->addr(), HDR_MAC802_16(p)->header.cid);
    debug2(" sampad in send process DSA_request");
    Packet *rsp;
    struct hdr_cmn *ch;
    hdr_mac802_16 *wimaxHdr_req;
    hdr_mac802_16 *wimaxHdr_rsp;
    mac802_16_dsa_req_frame *dsa_req_frame;
    mac802_16_dsa_rsp_frame *dsa_rsp_frame;
    PeerNode *peer;
    Connection *data;
    Arqstatus *arqstatus;

    //read the request
    wimaxHdr_req = HDR_MAC802_16(p);
    dsa_req_frame = (mac802_16_dsa_req_frame*) p->accessdata();
    peer = mac_->getCManager ()->get_connection (wimaxHdr_req->header.cid, true)->getPeerNode();

    //allocate response
    //create packet for request
    rsp = mac_->getPacket ();
    ch = HDR_CMN(rsp);
    wimaxHdr_rsp = HDR_MAC802_16(rsp);
    rsp->allocdata (sizeof (struct mac802_16_dsa_rsp_frame));
    dsa_rsp_frame = (mac802_16_dsa_rsp_frame*) rsp->accessdata();
    dsa_rsp_frame->type = MAC_DSA_RSP;
    dsa_rsp_frame->transaction_id = dsa_req_frame->transaction_id;
    dsa_rsp_frame->uplink = dsa_req_frame->uplink;
    dsa_rsp_frame->confirmation_code = 0; //OK
    dsa_rsp_frame->staticflow = dsa_req_frame->staticflow;

    if (mac_->getNodeType()==STA_MN) {
        //the message contains the CID for the connection
        data = new Connection (CONN_DATA, dsa_req_frame->cid);
        /*
            data->setCDMA(0);
            data->initCDMA();
            data->setPOLL_interval(0);
        */

        data->set_serviceflow(dsa_req_frame->staticflow);
        // We will move all the ARQ information in the service flow to the isArqStatus that maintains the Arq Information.
        if (data->get_serviceflow ()->getQosSet ()->isArqEnabled () == true ) {
            debug2("ARQ status is enable.\n");
            arqstatus = new Arqstatus ();
            data->setArqStatus (arqstatus);
            data->getArqStatus ()->setArqEnabled (data->get_serviceflow ()->getQosSet ()->isArqEnabled ());
            data->getArqStatus ()->setRetransTime (data->get_serviceflow ()->getQosSet ()->getArqRetransTime ());
            data->getArqStatus ()->setMaxWindow (data->get_serviceflow ()->getQosSet ()->getArqMaxWindow ());
            data->getArqStatus ()->setCurrWindow (data->get_serviceflow ()->getQosSet ()->getArqMaxWindow ());
            data->getArqStatus ()->setAckPeriod (data->get_serviceflow ()->getQosSet ()->getArqAckProcessingTime ());
            // setting timer array for the flow
            data->getArqStatus ()->arqRetransTimer = new ARQTimer(data);        /*RPI*/
            debug2("In DSA req STA_MN, generate a ARQ Timer and cid is %d.\n", data->get_cid());
        }
        if (dsa_req_frame->uplink) {
            mac_->getCManager()->add_connection (data, OUT_CONNECTION);
            debug2(" dsa-req-frame being processed and connection being added for uplink node = MN\n");
            peer->addOutDataCon (data);

        } else {
            mac_->getCManager()->add_connection (data, IN_CONNECTION);
            debug2(" dsa-req-frame being processed and connection being added for downlink node =MN\n");
            peer->addInDataCon (data);
        }
        ch->size() += GET_DSA_RSP_SIZE (0);
    } else {
        //allocate new connection
        data = new Connection (CONN_DATA);
        /*
            data->setCDMA(0);
            data->initCDMA();
            data->setPOLL_interval(0);
        */

        data->set_serviceflow(dsa_req_frame->staticflow);
        // We will move all the ARQ information in the service flow to the isArqStatus that maintains the Arq Information.
        if (data->get_serviceflow ()->getQosSet ()->isArqEnabled () == true ) {
            debug2("Going to set ARQ parameters.\n");
            arqstatus = new Arqstatus ();
            data->setArqStatus (arqstatus);
            data->getArqStatus ()->setArqEnabled (data->get_serviceflow ()->getQosSet ()->isArqEnabled ());
            data->getArqStatus ()->setRetransTime (data->get_serviceflow ()->getQosSet ()->getArqRetransTime ());
            data->getArqStatus ()->setMaxWindow (data->get_serviceflow ()->getQosSet ()->getArqMaxWindow ());
            data->getArqStatus ()->setCurrWindow (data->get_serviceflow ()->getQosSet ()->getArqMaxWindow ());
            data->getArqStatus ()->setAckPeriod (data->get_serviceflow ()->getQosSet ()->getArqAckProcessingTime ());
            // setting timer array for the flow
            data->getArqStatus ()->arqRetransTimer = new ARQTimer(data);        /*RPI*/
            debug2("In DSA req STA_BS, generate a ARQ Timer and cid is %d.\n", data->get_cid());

        }
        if (dsa_req_frame->uplink) {
            mac_->getCManager()->add_connection (data, IN_CONNECTION);
            debug2(" dsa-req-frame being processed and connection being added for uplink node = not MN\n");
            peer->addInDataCon (data);
        } else {
            mac_->getCManager()->add_connection (data, OUT_CONNECTION);
            debug2(" dsa-req-frame being processed and connection being added for downlink node = not MN\n");
            peer->addOutDataCon (data);
            debug2("set outcoming data connection for mac %d\n", mac_->addr());
//Begin RPI
            if (data->get_serviceflow ()->getQosSet ()->isArqEnabled () == true ) {
                //schedule timer based on retransmission time setting
                //mac_->debug(" ARQ Timer Setting Downlink\n");
                data->getArqStatus ()->arqRetransTimer->sched(data->getArqStatus ()->getRetransTime ());
                //mac_->debug(" ARQ Timer Set Downlink\n");
            }
//End RPI
        }
        dsa_rsp_frame->cid = data->get_cid();
        ch->size() += GET_DSA_RSP_SIZE (1);
    }

    wimaxHdr_rsp->header.cid = peer->getPrimary(OUT_CONNECTION)->get_cid();
    peer->getPrimary(OUT_CONNECTION)->enqueue (rsp);

}

/**
 * process a flow response
 * @param p The received response
 */
void ServiceFlowHandler::processDSA_rsp (Packet *p)
{
    mac_->debug ("At %f in Mac %d received DSA response\n", NOW, mac_->addr());

    Packet *ack;
    struct hdr_cmn *ch;
    hdr_mac802_16 *wimaxHdr_ack;
    hdr_mac802_16 *wimaxHdr_rsp;
    mac802_16_dsa_ack_frame *dsa_ack_frame;
    mac802_16_dsa_rsp_frame *dsa_rsp_frame;
    Connection *data;
    PeerNode *peer;
    Arqstatus * arqstatus;

    //read the request
    wimaxHdr_rsp = HDR_MAC802_16(p);
    dsa_rsp_frame = (mac802_16_dsa_rsp_frame*) p->accessdata();
    peer = mac_->getCManager ()->get_connection (wimaxHdr_rsp->header.cid, true)->getPeerNode();

    //TBD: check if status not OK

    if (mac_->getNodeType()==STA_MN) {
        //the message contains the CID for the connection
        data = new Connection (CONN_DATA, dsa_rsp_frame->cid);
        /*
            data->setCDMA(0);
            data->initCDMA();
            data->setPOLL_interval(0);
        */

        data->set_serviceflow(dsa_rsp_frame->staticflow);
        // We will move all the ARQ information in the service flow to the isArqStatus that maintains the Arq Information.
        if (data->get_serviceflow ()->getQosSet ()->isArqEnabled () == true ) {
            arqstatus = new Arqstatus ();
            data->setArqStatus (arqstatus);
            data->getArqStatus ()->setArqEnabled (data->get_serviceflow ()->getQosSet ()->isArqEnabled ());
            data->getArqStatus ()->setRetransTime (data->get_serviceflow ()->getQosSet ()->getArqRetransTime ());
            data->getArqStatus ()->setMaxWindow (data->get_serviceflow ()->getQosSet ()->getArqMaxWindow ());
            data->getArqStatus ()->setCurrWindow (data->get_serviceflow ()->getQosSet ()->getArqMaxWindow ());
            data->getArqStatus ()->setAckPeriod (data->get_serviceflow ()->getQosSet ()->getArqAckProcessingTime ());
            // setting timer array for the flow
            data->getArqStatus ()->arqRetransTimer = new ARQTimer(data);             /*RPI*/
            debug2("In DSA rsp STA_MN, generate a ARQ Timer and cid is %d.\n", data->get_cid());
        }
        if (dsa_rsp_frame->uplink) {
            mac_->getCManager()->add_connection (data, OUT_CONNECTION);
            peer->addOutDataCon (data);
            debug2("set outcoming data connection for mac %d\n", mac_->addr());
//Begin RPI
            if (data->get_serviceflow ()->getQosSet ()->isArqEnabled () == true ) {
                //schedule timer based on retransmission time setting
                //mac_->debug(" ARQ Timer Setting -Uplink\n");
                data->getArqStatus ()->arqRetransTimer->sched(data->getArqStatus ()->getRetransTime ());
                //mac_->debug(" ARQ Timer Set- Uplink\n");
            }
//End RPI
        } else {
            mac_->getCManager()->add_connection (data, IN_CONNECTION);
            peer->addInDataCon (data);
            debug2("set incoming data connection for mac %d\n", mac_->addr());
        }
    }

    //allocate ack
    //create packet for request
    ack = mac_->getPacket ();
    ch = HDR_CMN(ack);
    wimaxHdr_ack = HDR_MAC802_16(ack);
    ack->allocdata (sizeof (struct mac802_16_dsa_ack_frame));
    dsa_ack_frame = (mac802_16_dsa_ack_frame*) ack->accessdata();
    dsa_ack_frame->type = MAC_DSA_ACK;
    dsa_ack_frame->transaction_id = dsa_rsp_frame->transaction_id;
    dsa_ack_frame->uplink = dsa_rsp_frame->uplink;
    dsa_ack_frame->confirmation_code = 0; //OK
    ch->size() += DSA_ACK_SIZE;

    wimaxHdr_ack->header.cid = peer->getPrimary(OUT_CONNECTION)->get_cid();
    peer->getPrimary(OUT_CONNECTION)->enqueue (ack);

}

/**
 * process a flow request
 * @param p The received response
 */
void ServiceFlowHandler::processDSA_ack (Packet *p)
{
    mac_->debug ("At %f in Mac %d received DSA ack\n", NOW, mac_->addr());
}

/**
 * Add Dynamic Flow
 * includes Admission Control
 */
int addDynamicFlow ( int argc, const char*const* argv){

	ServiceFlowQosSet * serviceFlowQosSet = createServiceFlowQosSet( argc, argv);
	if ( serviceFlowQosSet == NULL ) {
		// an error occurred
		return TCL_ERROR;
	}

	bool admitted = admissionControl_->checkAdmission( serviceFlowQosSet);

	if ( admitted == true ) {
	    /* Create the Static Service Flow */
		ServiceFlow * dynamicServiceFlow = new ServiceFlow ( direction, staticflowqosset);

		/* Add the Service Flow to the Static Flow List*/
		dynamicServiceFlow->insert_entry_head (&static_flow_head_);
		debug2(" dynamic flow static flow created after admission \n");
	} else {
		debug2(" Admission rejected");
	}

	return TCL_OK;
}


/*
 * Add a static flow
 * @param argc The number of parameter
 * @param argv The list of parameters
 */
int ServiceFlowHandler::addStaticFlow (int argc, const char*const* argv)
{

	ServiceFlowQosSet * serviceFlowQosSet = createServiceFlowQosSet( argc, argv);
	if ( serviceFlowQosSet == NULL ) {
		// an error occurred
		return TCL_ERROR;
	}

    /* Create the Static Service Flow */
    ServiceFlow * staticflow = new ServiceFlow ( direction, staticflowqosset);

    /* Add the Service Flow to the Static Flow List*/
    staticflow->insert_entry_head (&static_flow_head_);
    debug2(" service flow static flow created ");
    return TCL_OK;
}

/**
 * Process input string to build ServiceFlowQosSet
 */
ServiceFlowQosSet * createServiceFlowQosSet( int argc, const char*const* argv)
{

	ServiceFlowQosSet * newServiceFlowQosSet = NULL;

	// see serviceflowqosset.h for explanations of the parameters
	Dir_t direction; // Direction of the service flow

	u_int8_t trafficPriority = 0;
    u_int32_t maxSustainedTrafficRate;
    u_int32_t maxTrafficBurst;
    u_int32_t minReservedTrafficRate;
    UlGrantSchedulingType_t	ulGrantSchedulingType = UL_BE;
    std::bitset<8> reqTransmitPolicy;
    u_int32_t toleratedJitter;
    u_int32_t maxLatency;
    bool isFixedLengthSdu;
    u_int8_t sduSize;
    bool isArqEnabled;
    u_int16_t arqMaxWindow;
    double arqRetransTime; // Not IEEE 802.16-2009 conform
    u_int8_t ackProcessingTime; // Not conform
    u_int16_t arqBlockSize;
    u_int16_t unsolicitedGrantInterval = 0;
    u_int16_t unsolicitedPollingInterval = 0;
    DataDeliveryServiceType_t dataDeliveryServiceType = DL_BE;
    u_int16_t timeBase;
    double packetErrorRate;


	// TODO: Check for mandatory parameters

	// get direction of the service flow
	if ( strcmp(argv[2], "DL") == 0 ) {
		direction = DL;

		// get data delivery service type
		if ( strcmp(argv[3], "UGS") == 0 ) {
			dataDeliveryServiceType = DL_UGS;
		} else if ( strcmp(argv[3], "ERT-VR") == 0) {
			dataDeliveryServiceType = DL_ERTVR;
		} else if ( strcmp(argv[3], "RT-VR") == 0) {
			dataDeliveryServiceType = DL_RTVR;
		} else if ( strcmp(argv[3], "NRT-VR") == 0 ) {
			dataDeliveryServiceType = DL_NRTVR;
		} else if ( strcmp(argv[3], "BE") == 0 ) {
			dataDeliveryServiceType = DL_BE;
		} else {
			return newServiceFlowQosSet;
		}

	} else if (strcmp(argv[2], "UL") == 0) {
		direction = UL;

		// get uplink grant scheduling type
		if ( strcmp(argv[3], "UGS") == 0 ) {
			ulGrantSchedulingType = UL_UGS;
		} else if ( strcmp(argv[3], "ertPS") == 0 ) {
			ulGrantSchedulingType = UL_ertPS;
		} else if ( strcmp(argv[3], "rtPS") == 0 ) {
			ulGrantSchedulingType = UL_rtPS;
		} else if ( strcmp(argv[3], "nrtPS") == 0 ) {
			ulGrantSchedulingType = UL_nrtPS;
		} else if ( strcmp(argv[3], "BE") == 0 ){
				ulGrantSchedulingType = UL_BE;
		} else {
			return newServiceFlowQosSet;
		}
	}

	// Read arguments

	// Set Trafic Priority
	trafficPriority = u_int8_t( atoi( argv[4]));
	if (trafficPriority  < 0) {
		trafficPriority = 0;
		fprintf(stderr, "QoS Parameter Traffic Priority is out of range from 0 to 7 \n");
	}
	if (trafficPriority > 7) {
		trafficPriority = 7;
		fprintf(stderr, "QoS Parameter Traffic Priority is out of range from 0 to 7 \n");
	}

	// Set Maximum Sustained Traffic Rate
	maxSustainedTrafficRate = u_int32_t( atof( argv[5]));
	if ( maxSustainedTrafficRate <= 0) {
		// "0" specifies that no limit is given
		maxSustainedTrafficRate = 4294967295;
	}

	// Set Maximum Traffic Burst Size
	maxTrafficBurst = u_int32_t( atof( argv[6]));

	// Set Minimum Reserved Traffic Rate
	minReservedTrafficRate = u_int32_t( atof( argv[7]));
	if ( minReservedTrafficRate > maxSustainedTrafficRate ) {
		minReservedTrafficRate = maxSustainedTrafficRate;
		fprintf(stderr, "Minimum Revered Traffic Rate can not be greater than the Maximum Sustained Traffic Rate \n");
	}

	// Set Request/Transmission Policy (Currently Unused)
	reqTransmitPolicy = std::bitset<8>( atoi( argv[8]));

	// Set Tolerated Jitter
	toleratedJitter = u_int32_t( atof( argv[9]));

	// Set Maximum Latency
	maxLatency = u_int32_t( atof( argv[10]));

	// Set Fixed Length SDU (Currently Unused)
	isFixedLengthSdu = bool( atoi( argv[11]));
	sduSize = u_int8_t( atoi( argv[12]));

	// Enable ARQ
	isArqEnabled = bool( atoi( argv[13]));
	if (isArqEnabled) {
		debug2("ARQ is enabled for this static flow.\n");
	}
    arqMaxWindow = u_int16_t( atoi( argv[14]));
    arqRetransTime = double( atof( argv[15])); // Not IEEE 802.16-2009 conform
    ackProcessingTime = u_int8_t( atoi( argv[16]));
    arqBlockSize = u_int16_t( atoi( argv[17]));

    // Set Unsolicited Grant Interval or unsolicited Polling Interval

    // Frame Duration is in seconds but Grant or Polling Interval is defined in Milliseconds
	double frameDuration = mac_->getFrameDuration() * 1e3;
	// only for uplink connections
	if ( direction == UL ) {
		if (( ulGrantSchedulingType == UL_UGS) or ( ulGrantSchedulingType == UL_ertPS) ) {
			unsolicitedGrantInterval = u_int16_t( atof( argv[18]));
			// check if Grant Interval is less than the frame duration
			if ( unsolicitedGrantInterval < frameDuration) {
				fprintf(stderr, "Grant Intervals below Frame Duration are not possible \n");
				unsolicitedGrantInterval = u_int16_t( frameDuration);
			}
		} else if ( ulGrantSchedulingType == UL_rtPS) {
			unsolicitedPollingInterval = u_int16_t( atof( argv[18])); // IEEE 802.16-2009 page 372
			if ( unsolicitedPollingInterval == 0) {
				unsolicitedPollingInterval = u_int16_t( DEFAULT_POLLING_INTERVAL);
			}
			if ( unsolicitedPollingInterval < frameDuration) {
				fprintf(stderr, "Polling Intervals below Frame Duration are not possible \n");
				unsolicitedPollingInterval = u_int16_t( frameDuration);
			}
		} else if ( ulGrantSchedulingType == UL_nrtPS) {
			// Parameter not defined of nrtPS
			// default for nrtPS (one second or less)
			unsolicitedPollingInterval = u_int16_t( DEFAULT_POLLING_INTERVAL);
		}
	}

	// Set Time Base
	timeBase = u_int16_t( atof( argv[19]));
	if ( timeBase < frameDuration ) {
		fprintf(stderr, "Time Base below Frame Duration are not possible \n");
		timeBase = u_int16_t( frameDuration);
	}

	packetErrorRate = double( atof( argv[20]));


	/* Create new Service Flow QoS Set Object */

	ServiceFlowQosSet * newServiceFlowQosSet = new ServiceFlowQosSet (
			trafficPriority,
			maxSustainedTrafficRate,
			maxTrafficBurst,
			minReservedTrafficRate,
			ulGrantSchedulingType,
			reqTransmitPolicy,
			toleratedJitter,
			maxLatency,
			isFixedLengthSdu,
			sduSize,
			isArqEnabled,
			arqMaxWindow,
			arqRetransTime, // Not IEEE 802.16-2009 conform
			ackProcessingTime, // Not confo
			arqBlockSize,
			unsolicitedGrantInterval,
			unsolicitedPollingInterval,
			dataDeliveryServiceType,
			timeBase,
			packetErrorRate
			);


    return newServiceFlowQosSet;

}

/*
 * Remove a static flow
 * @param argc The number of parameter
 * @param argv The list of parameters
 */
int ServiceFlowHandler::removeStaticFlow (int argc, const char*const* argv)
{

	// see serviceflowqosset.h for explanations of the parameters


	int sfid = atoi( argv[2]);

	// TODO: Ugly find better solution

	ServiceFlow * currentFlow =  reinterpret_cast<ServiceFlow*>( &static_flow_head_) ;

	while ( currentFlow != NULL) {
		if ( currentFlow->getSFID() == sfid) {
			if ( currentFlow->getQosSet() != NULL ) {
				delete currentFlow->getQosSet();
			}
			currentFlow->remove_entry();
			delete currentFlow;
			currentFlow = NULL;
			printf("Service Flow d% removed \n", sfid);
		} else {
			currentFlow = currentFlow->next_entry();
		}
	}
	printf("Service Flow not found");
    return TCL_OK;
}


/*
 * Add a static flow
 * @param argc The number of parameter
 * @param argv The list of parameters
 */
/* old version vr@tud
int ServiceFlowHandler::addStaticFlow (int argc, const char*const* argv)
{
    dir_t dir;
    int32_t datarate = atoi(argv[3]);
    SchedulingType_t  flow_type;
    double data_size = 0.0;
    u_int16_t period = 0;
    u_int8_t isArqEnabled = atoi(argv[7]);
    double arq_retrans_time = atof(argv[8]);
    u_int32_t arq_max_window = atoi(argv[9]);
    u_int8_t ack_period = atoi(argv[10]);
    int delay = 0 ;
    int burstsize = 0;

    if (strcmp(argv[2], "DL") == 0) {
        dir = DL;
    } else if (strcmp(argv[2], "UL") == 0) {
        dir = UL;
    } else {
        return TCL_ERROR;
    }

    if (strcmp(argv[4], "BE") == 0) {
        flow_type = SERVICE_BE;
    } else if (strcmp(argv[4], "UGS") == 0) {
        flow_type = SERVICE_UGS;
        if (argc != 21) {
            return TCL_ERROR;
        } else {
            // convert user defined bytes to time
            //data_size = (atoi(argv[5])<<3)/(double)(mac_->phymib_.getDataRate());
            period = atoi(argv[6]);
            data_size = atoi(argv[5]);

        }
    } else if (strcmp(argv[4], "ertPS") == 0) {
        flow_type = SERVICE_ertPS;
        if (argc != 21) {
            return TCL_ERROR;
        } else {
            period = atoi(argv[6]);
            data_size = atoi(argv[5]);
        }
    } else if (strcmp(argv[4], "rtPS") == 0) {
        flow_type = SERVICE_rtPS;
        if (argc != 21) {
            return TCL_ERROR;
        } else {
            period = atoi(argv[6]);
        }
    } else if (strcmp(argv[4], "nrtPS") == 0) {
        flow_type = SERVICE_nrtPS;
        if (argc != 21) {
            return TCL_ERROR;
        } else {
            period = atoi(argv[6]);
        }
    } else {
        return TCL_ERROR;
    }


    // Create the Service Flow Qos object
    ServiceFlowQoS * staticflowqos = new ServiceFlowQoS (delay, datarate, burstsize) ;
    staticflowqos->setDataSize(data_size);
    staticflowqos->setPeriod(period);
    staticflowqos->setIsArqEnabled(isArqEnabled);
    debug2("ARQ is enabled for this static flow.\n");
    staticflowqos->setArqRetransTime(arq_retrans_time);
    staticflowqos->setArqMaxWindow(arq_max_window);
    staticflowqos->setArqAckPeriod(ack_period);


    staticflowqos->setTrafficPriority(atoi(argv[11]));
    staticflowqos->setPeakTrafficRate(atoi(argv[12]));
    staticflowqos->setMinReservedTrafficRate(atoi(argv[13]));
    staticflowqos->setReqTransmitPolicy(atoi(argv[14]));
    staticflowqos->setJitter(atoi(argv[15]));
    staticflowqos->setSDUIndicator(atoi(argv[16]));
    staticflowqos->setMinTolerableTrafficRate(atoi(argv[17]));
    staticflowqos->setSDUSize(atoi(argv[18]));
    staticflowqos->setMaxBurstSize(atoi(argv[19]));
    staticflowqos->setSAID(atoi(argv[20]));



    // Create the Static Service Flow
    ServiceFlow * staticflow = new ServiceFlow (flow_type, staticflowqos);
    staticflow->setDirection(dir);

    // Add the Service Flow to the Static Flow List
    staticflow->insert_entry_head (&static_flow_head_);
    debug2(" service flow static flow created ");
    return TCL_OK;
}
end old version
*/


/**
 * Send a flow request to the given node
 * @param index The node address
 * @param uplink The flow direction
 */
void ServiceFlowHandler::init_static_flows (int index)
{
    Packet *p;
    struct hdr_cmn *ch;
    hdr_mac802_16 *wimaxHdr;
    mac802_16_dsa_req_frame *dsa_frame;
    PeerNode *peer;
    Arqstatus * arqstatus;

    debug2(" sampad in init staic flows\n");

    for (ServiceFlow *n=static_flow_head_.lh_first;
            n; n=n->next_entry()) {
        //create packet for request
        peer = mac_->getPeerNode(index);
        p = mac_->getPacket ();
        ch = HDR_CMN(p);
        wimaxHdr = HDR_MAC802_16(p);
        p->allocdata (sizeof (struct mac802_16_dsa_req_frame));
        dsa_frame = (mac802_16_dsa_req_frame*) p->accessdata();
        dsa_frame->type = MAC_DSA_REQ;
        if (n->getDirection() == UL )
            dsa_frame->uplink = true;
        else
            dsa_frame->uplink = false;
        dsa_frame->transaction_id = TransactionID++;
        dsa_frame->staticflow = n;
        if (mac_->getNodeType()==STA_MN)
            ch->size() += GET_DSA_REQ_SIZE (0);
        else {
            //assign a CID and include it in the message
            Connection *data = new Connection (CONN_DATA);
            /*
            		  data->setCDMA(0);
            		  data->initCDMA();
                		  data->setPOLL_interval(0);
            */

            data->set_serviceflow(n);
            // We will move all the ARQ information in the service flow to the isArqStatus that maintains the Arq Information.
            if (data->get_serviceflow ()->getQosSet ()->isArqEnabled () == true ) {
                debug2("add an ARQ enabled connection.\n");
                arqstatus = new Arqstatus ();
                data->setArqStatus (arqstatus);
                data->getArqStatus ()->setArqEnabled (data->get_serviceflow ()->getQosSet ()->isArqEnabled ());
                data->getArqStatus ()->setRetransTime (data->get_serviceflow ()->getQosSet ()->getArqRetransTime ());
                data->getArqStatus ()->setMaxWindow (data->get_serviceflow ()->getQosSet ()->getArqMaxWindow ());
                data->getArqStatus ()->setCurrWindow (data->get_serviceflow ()->getQosSet ()->getArqMaxWindow ());
                data->getArqStatus ()->setAckPeriod (data->get_serviceflow ()->getQosSet ()->getArqAckProcessingTime ());
                // setting timer array for the flow
                data->getArqStatus ()->arqRetransTimer = new ARQTimer(data);      /*RPI*/
                debug2("In init_static_flow, ARQ connection timer is generated.\n");
            }
            if (n->getDirection() == UL ) {
                mac_->getCManager()->add_connection (data, OUT_CONNECTION);
                debug2("ARQ UL connection.\n");
            } else {
                mac_->getCManager()->add_connection (data, IN_CONNECTION);
                debug2("ARQ DL connection.\n");
            }
            if (n->getDirection() == UL) {
                peer->addInDataCon (data);
                debug2("Add incoming data connection for mac %d\n", mac_->addr());
            } else {
                peer->addOutDataCon (data);
                debug2("Add outcoming data connection for mac %d\n", mac_->addr());
            }
            dsa_frame->cid = data->get_cid();
            ch->size() += GET_DSA_REQ_SIZE (1);
        }

        debug2(" sampad in init staic flows before end\n");
        wimaxHdr->header.cid = peer->getPrimary(OUT_CONNECTION)->get_cid();
        peer->getPrimary(OUT_CONNECTION)->enqueue (p);
    }
}
// End RPI

