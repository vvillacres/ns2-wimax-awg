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

#ifndef SERVICEFLOW_H
#define SERVICEFLOW_H

#include "serviceflowqosset.h"
//#include "connection.h"
#include "packet.h"

#define UNASSIGNED_FLOW_ID -1

/**
 * Defines the state for the serviceflow
 */
enum ServiceFlowState_t {
	PROVISIONED,
	ADMITTED,
	ACTIVE
};



/** Direction of the flow */
enum Dir_t {
    NONE,  // flow not used
    DL,    // downlink flow
    UL     // uplink flow
};

class ServiceFlow;
LIST_HEAD (serviceflow, ServiceFlow);

/**
 * Class ServiceFlow
 * The service flow identifies the service requirement
 * for the associated connection
 */
class ServiceFlow
{
public:
    /**
     * Constructor
     */
    ServiceFlow( Dir_t direction, ServiceFlowQosSet * activeQosSet );

    /**
     * Return the service flow id
     * @return The service flow id. -1 if not yet assigned
     */
    inline int getSFID () {
        return sfid_;
    }

    /**
     * Assign an ID to the service flow
     * @param id The ID to set
     */
    void setSFID (int sfid);

    /**
     * Pick the next available ID. Should be called by a BS to assign a unique ID
     */
    void pickID ();

    /**
     * Return the operational state of the Service Flow
     */
    inline ServiceFlowState_t getServiceFlowState () {
    	return sfState_;
    }

    /**
     * Set the operational state of the Service Flow
     */
    inline void setServiceFlowState(ServiceFlowState_t sfState) {
    	sfState_ = sfState;
    }

    /**
     * Return the QoS for this connection
     */
    inline Dir_t getDirection () {
        return direction_;
    }

    /**
     * Set the QoS for this flow
     * @param qos The new QoS for this flow
     */
    inline void setDirection (Dir_t dir) {
        direction_ = dir;
    }

    /**
     * Return the Active QoS Parameter Set for this service flow
     */
    inline ServiceFlowQosSet * getQoS () {
        return activeQosSet_;
    }

    /**
     * Set the Active QoS Parameter Set for this flow
     */
    inline void setActiveQosSet (ServiceFlowQosSet* activeQosSet) {
        activeQosSet_ = activeQosSet;
    }

    // Chain element to the list
    inline void insert_entry_head(struct serviceflow *head) {
        LIST_INSERT_HEAD(head, this, link);
    }

    // Chain element to the list
    inline void insert_entry(ServiceFlow *elem) {
        LIST_INSERT_AFTER(elem, this, link);
    }

    // Return next element in the chained list
    ServiceFlow* next_entry(void) const {
        return link.le_next;
    }

    // Remove the entry from the list
    inline void remove_entry() {
        LIST_REMOVE(this, link);
    }

protected:

    /**
     * Pointer to next in the list
     */
    LIST_ENTRY(ServiceFlow) link;
    //LIST_ENTRY(ServiceFlow); //for magic draw

private:
    /**
     * The service flow id
     */
    int sfid_;

    /**
     * The operational state of the service flow
     */
    ServiceFlowState_t sfState_;

    /**
     * Flow direction
     */
    Dir_t direction_;

    /**
     * The Active Quality of Service Parameter Set this service flow
     */
    ServiceFlowQosSet * activeQosSet_;

};
#endif //SERVICEFLOW_H

