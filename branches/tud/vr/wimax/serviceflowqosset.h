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

#ifndef SERVICEFLOWQOSSET_H
#define SERVICEFLOWQOSSET_H

#include <bitset>
#include "packet.h"
#include "queue.h"

/**
 * Defines the supported scheduling mechanism for this UL service flow (cp. IEEE 802.16-2009 11.13.10)
 *
 * 0: Reserved
 * 1: Undefined (BS implementation-dependenta)
 * 2: BE (default)
 * 3: nrtPS
 * 4: rtPS
 * 5: Extended rtPS
 * 6: UGS
 * 7–255: Reserved
 */
enum UlGrantSchedulingType_t {
    UL_UGS,
    UL_ertPS,
    UL_rtPS,
    UL_nrtPS,
    UL_BE
};

/**
 * Defines the Type of Data Delivery Service for the DL service flow (cp. IEEE 802.16-2009 11.13.29)
 *
 * 0: Unsolicited Grant Service
 * 1: Real-Time Variable Rate Service
 * 2: Non-Real-Time Variable Rate Service
 * 3: Best Effort Service
 * 4: Extended Real-Time Variable Rate Service
 */
enum DataDeliveryServiceType_t {
	DL_UGS,
	DL_ERTVR,
	DL_RTVR,
	DL_NRTVR,
	DL_BE
};


/**
 * Class ServiceFlowQoS
 * Defines Qos requirements for the flows
 */
class ServiceFlowQosSet
{

public:
    /**
     * Constructor
     * @param delay The maximum supported delay for the connection
     * @param datarate Average datarate
     * @param burstsize Size of each burst
     */
    ServiceFlowQosSet ();

    ServiceFlowQosSet (
    		u_int8_t trafficPriority,
    		u_int32_t maxSustainedTrafficRate,
    		u_int32_t maxTrafficBurst,
    		u_int32_t minReservedTrafficRate,
    		UlGrantSchedulingType_t	ulGrantSchedulingType,
    		std::bitset<8> reqTransmitPolicy,
    		u_int32_t toleratedJitter,
    		u_int32_t maxLatency,
    		bool isFixedLengthSdu,
    		u_int8_t sduSize,
    		bool isArqEnabled,
    		u_int16_t arqMaxWindow,
    		u_int16_t arqBlockSize,
    		u_int16_t unsolicitedGrantInterval,
    		u_int16_t unsolicitedPollingInterval,
    		DataDeliveryServiceType_t dataDeliveryServiceType,
    		u_int16_t timeBase,
    		double packetErrorRate
    		);

    /*
     * ************* Inline Get Methodes ******************
     * vr@tud
     */

    /**
    * Get Traffic Priority, 0 to 7—Higher numbers indicate higher priority, Default 0. (cp. IEEE 802.16-2009 11.13.5)
    */
    inline u_int8_t getTrafficPriority() {
    	return trafficPriority_;
    }

    /**
    * Get Maximum sustained traffic rate in bits per sec. Does not include MAC Overhead. (cp. IEEE 802.16-2009 11.13.6)
    * Specifies only a upper bound, which is averaged over time. If zero there is no mandated maximum rate.
    */
    inline u_int32_t getMaxSustainedTrafficRate() {
    	return maxSustainedTrafficRate_;
    }

    /**
    * Get Maximum Traffic Burst in bytes (cp. IEEE 802.16-2009 11.13.7)
    */
    inline u_int32_t getMaxTrafficBurst() {
    	return maxTrafficBurst_;
    }

    /**
    * Get Minimum reserved traffic rate in bits per second. Does not include MAC Overhead. (cp. IEEE 802.16-2009 11.13.8)
    * Specifies the minimum rate for a service. If zero no guaranteed rate is available.
    */
    inline u_int32_t getMinReservedTrafficRate() {
    	return minReservedTrafficRate_;
    }

    /**
     * Get UL Grant Scheduling Type Parameter (UGS, ertPS, rtPS, nrtPS, BE) (cp. IEEE 802.16-2009 11.13.10)
     */
    inline UlGrantSchedulingType_t	getUlGrantSchedulingType() {
    	return ulGrantSchedulingType_;
    }

    /**
    * Get Request/Transmission Policy Parameter (cp. IEEE 802.16-2009 11.13.11)
    */
    inline std::bitset<8> getReqTransmitPolicy() {
    	return reqTransmitPolicy_;
    }

    /**
    * Get Tolerated Jitter in milliseconds (cp. IEEE 802.16-2009 11.13.12)
    */
    inline u_int32_t getToleratedJitter() {
    	return toleratedJitter_;
    }

    /**
     * Get Maximum Latency Parameter in milliseconds. Specifies the maximum interval between the entry at the CS
     * and the forwarding of the SDU to the air interface. (cp. IEEE 802.16-2009 11.13.13)
     */
    inline u_int32_t getMaxLatency() {
    	return maxLatency_;
    }

    /**
    * Is Fixed-length SDU enable. 0: Variable Length 1: Fixed-length Default = 0 (cp. IEEE 802.16-2009 11.13.14)
    */
    inline bool isFixedLengthSdu() {
    	return isFixedLengthSdu_;
    }

    /**
    * Get SDU size parameter in Byte. Specifies the length of fixed Size SDUs  (cp. IEEE 802.16-2009 11.13.15)
    */
    inline u_int8_t getSduSize() {
    	return sduSize_;
    }

    /**
	* Return the ARQ Status
	*/
   inline bool  isArqEnabled () {
	   return isArqEnabled_;
   }

   /**
   * Return the ARQ maximum window
   */
   inline u_int16_t getArqMaxWindow () {
	   return arqMaxWindow_;
   }

   /**
   * Return the ARQ retrans time
   */
  /* inline double getArqRetransTime () {
	   return arqRetransTime_;
   }*/
   /**
   * Return the ARQ Acknowledgement Period
   */
   /*inline u_int8_t getArqAckPeriod () {
	   return ackPeriod_;
   }*/

   /**
    * Return the ARQ Block Size
    */
   inline u_int16_t getArqBlockSize () {
	   return arqBlockSize_;
   }


    /**
     * Get Unsolicited Grant Interval for UL service flows UGS and ertPS in Milliseconds (cp. IEEE 802.16-2009 11.13.19)
     */
    inline u_int16_t getGrantInterval() {
    	return unsolicitedGrantInterval_;
    }

    /**
     * Get Unsolicited Polling Interval for UL service flows in Milliseconds (cp. IEEE 802.16-2009 11.13.20)
     */
    inline u_int16_t getPollingInterval() {
    	return unsolicitedPollingInterval_;
    }

    /**
     * Get Type of Data Delivery Services for DL service flows (cp. IEEE 802.16-2009 11.13.24)
     */
    inline DataDeliveryServiceType_t getDataDeliveryServiceType() {
    	return dataDeliveryServiceType_;
    }

    /**
     * Time Base parameter for rate measurement in milliseconds (cp. IEEE 802.16-2009 11.13.26)
     */
    inline u_int16_t getTimeBase() {
    	return timeBase_;
    }

    /**
     * Packet Error Rate (PER) indicated the target error rate of the service flow
     * In this model the post ARQ error rate shall be taken  (cp. IEEE 802.16-2009 11.13.37)
     */
    inline double getPacketErrorRate() {
    	return packetErrorRate_;
    }



    /*
     * ********* Inline Set Methodes ************
     */


    /**
    * Set Traffic Priority, 0 to 7—Higher numbers indicate higher priority, Default 0. (cp. IEEE 802.16-2009 11.13.5)
    */
    inline void setTrafficPriority( u_int8_t trafficPriority) {
    	trafficPriority_ = trafficPriority;
    }

    /**
    * Set Maximum sustained traffic rate in bits per sec. Does not include MAC Overhead. (cp. IEEE 802.16-2009 11.13.6)
    * Specifies only a upper bound, which is averaged over time. If zero there is no mandated maximum rate.
    */
    inline void setMaxSustainedTrafficRate( u_int32_t maxSustainedTrafficRate) {
    	maxSustainedTrafficRate_ = maxSustainedTrafficRate;
    }

    /**
    * Set Maximum Traffic Burst in bytes (cp. IEEE 802.16-2009 11.13.7)
    */
    inline void setMaxTrafficBurst( u_int32_t maxTrafficBurst) {
    	maxTrafficBurst_ = maxTrafficBurst;
    }

    /**
    * Set Minimum reserved traffic rate in bits per second. Does not include MAC Overhead. (cp. IEEE 802.16-2009 11.13.8)
    * Specifies the minimum rate for a service. If zero no guaranteed rate is available.
    */
    inline void setMinReservedTrafficRate( u_int32_t minReservedTrafficRate) {
    	minReservedTrafficRate_ = minReservedTrafficRate;
    }

    /**
     * Set UL Grant Scheduling Type Parameter (UGS, ertPS, rtPS, nrtPS, BE) (cp. IEEE 802.16-2009 11.13.10)
     */
    inline void setUlGrantSchedulingType( UlGrantSchedulingType_t	 ulGrantSchedulingType) {
    	ulGrantSchedulingType_ = ulGrantSchedulingType;
    }

    /**
    *  Set Request/Transmission Policy Parameter (cp. IEEE 802.16-2009 11.13.11)
    */
    inline void setReqTransmitPolicy( std::bitset<8> reqTransmitPolicy) {
    	reqTransmitPolicy_ = reqTransmitPolicy;
    }

    /**
    * Set Tolerated Jitter in milliseconds (cp. IEEE 802.16-2009 11.13.12)
    */
    inline void setToleratedJitter( u_int32_t	toleratedJitter) {
    	toleratedJitter_ = toleratedJitter;
    }

    /**
     * Set Maximum Latency Parameter in milliseconds. Specifies the maximum interval between the entry at the CS
     * and the forwarding of the SDU to the air interface. (cp. IEEE 802.16-2009 11.13.13)
     */
    inline void setMaxLatency( u_int32_t maxLatency) {
    	maxLatency_ = maxLatency;
    }

    /**
    * Set Fixed-length versus variable-length SDU indicator 0: Variable Length 1: Fixed-length Default = 0 (cp. IEEE 802.16-2009 11.13.14)
    */
    inline void setFixedLendthSdu( bool isFixedLengthSdu) {
    	isFixedLengthSdu_ = isFixedLengthSdu;
    }

    /**
    *  Set SDU size parameter in Byte. Specifies the length of fixed Size SDUs  (cp. IEEE 802.16-2009 11.13.15)
    */
    inline void setSduSize( u_int8_t sduSize) {
    	sduSize_ = sduSize;
    }

    /**
     * Set the ARQ Status
     */
    inline void  setArqEnabled (bool isArqEnabled) {
        isArqEnabled_ = isArqEnabled;
    }

    /**
    * Set the ARQ maximum window
    */
    inline void setArqMaxWindow ( u_int16_t arqMaxWindow) {
        arqMaxWindow_ = arqMaxWindow;
    }

    /**
    * Set the ARQ retrans time
    */
    /*inline void setArqRetransTime (double arqRetransTime) {
        arq_retrans_time_ = arq_retrans_time;
    }*/

    /**
    * Return the ARQ Acknowledgement Period
    */
  /*  inline void setArqAckPeriod (u_int8_t ack_period ) {
        ack_period_ = ack_period;
    }*/

    /**
     * Set the ARQ Block Size
     */
    inline void setArqBlockSize (u_int16_t arqBlockSize) {
    	arqBlockSize_ = arqBlockSize;
    }

    /**
     * Set Unsolicited Grant Interval for UL service flows UGS and ertPS in Milliseconds (cp. IEEE 802.16-2009 11.13.19)
     */
    inline void setGrantInterval( u_int16_t unsolicitedGrantInterval) {
    	unsolicitedGrantInterval_ = unsolicitedGrantInterval;
    }

    /**
     * Set Unsolicited Polling Interval for UL service flows in Milliseconds (cp. IEEE 802.16-2009 11.13.20)
     */
    inline void setPollingInterval( u_int16_t unsolicitedPollingInterval) {
    	unsolicitedPollingInterval_ = unsolicitedPollingInterval;
    }

    /**
     * Set Type of Data Delivery Services for DL service flows (cp. IEEE 802.16-2009 11.13.24)
     */
    inline void setDataDeliveryServiceType( DataDeliveryServiceType_t dataDeliveryServiceType) {
    	dataDeliveryServiceType_ = dataDeliveryServiceType;
    }

    /**
     * Set Time Base parameter for rate measurement in milliseconds (cp. IEEE 802.16-2009 11.13.26)
     */
    inline void setTimeBase( u_int16_t timeBase) {
    	timeBase_ = timeBase;
    }

    /**
     * Set Packet Error Rate (PER) indicated the target error rate of the service flow
     * In this model the post ARQ error rate shall be taken  (cp. IEEE 802.16-2009 11.13.37)
     */
    inline void setPacketErrorRate( double packetErrorRate) {
    	packetErrorRate_ = packetErrorRate;
    }



protected:

private:

    /**
    * Traffic Priority, 0 to 7—Higher numbers indicate higher priority, Default 0. (cp. IEEE 802.16-2009 11.13.5)
    */
    u_int8_t      trafficPriority_;

    /**
    * Maximum sustained traffic rate in bits per sec. Does not include MAC Overhead. (cp. IEEE 802.16-2009 11.13.6)
    * Specifies only a upper bound, which is averaged over time. If zero there is no mandated maximum rate.
    */
    u_int32_t	maxSustainedTrafficRate_;

    /**
    * Maximum Traffic Burst in bytes (cp. IEEE 802.16-2009 11.13.7)
    */
    u_int32_t	maxTrafficBurst_;

    /**
    * Minimum reserved traffic rate in bits per second. Does not include MAC Overhead. (cp. IEEE 802.16-2009 11.13.8)
    * Specifies the minimum rate for a service. If zero no guaranteed rate is available.
    */
    u_int32_t	minReservedTrafficRate_;

    /**
     * UL Grant Scheduling Type Parameter (UGS, ertPS, rtPS, nrtPS, BE) (cp. IEEE 802.16-2009 11.13.10)
     */
    UlGrantSchedulingType_t	ulGrantSchedulingType_;

    /**
    *  Request/Transmission Policy Parameter (cp. IEEE 802.16-2009 11.13.11)
    */
    std::bitset<8>	reqTransmitPolicy_;

    /**
    * Tolerated Jitter in milliseconds (cp. IEEE 802.16-2009 11.13.12)
    */
    u_int32_t	toleratedJitter_;

    /**
     * Maximum Latency Parameter in milliseconds. Specifies the maximum interval between the entry at the CS
     * and the forwarding of the SDU to the air interface. (cp. IEEE 802.16-2009 11.13.13)
     */
    u_int32_t	maxLatency_;

    /**
    * Fixed-length versus variable-length SDU indicator 0: Variable Length 1: Fixed-length Default = 0 (cp. IEEE 802.16-2009 11.13.14)
    */
    bool	isFixedLengthSdu_;

    /**
    *  SDU size parameter in Byte. Specifies the length of fixed Size SDUs  (cp. IEEE 802.16-2009 11.13.15)
    */
    u_int8_t	sduSize_;

    /**
    *  0 - not enabled, 1- enabled (cp. IEEE 802.16-2009 11.13.17.1)
    */
    bool		isArqEnabled_;

    /**
    *  ARQ window size in ARQ Blocks (cp. IEEE 802.16-2009 11.13.17.2)
    */
    u_int16_t	arqMaxWindow_;

    /**
    * ARQ retransmission time period - sender side
    */
    //double        arq_retrans_time_;

    /**
    * ARQ ack timing period - set by user
    */
    //u_int8_t      ack_period_;

    /**
	 * ARQ Block Size (cp. IEEE 802.16-2009 11.13.17.8
     */
    u_int16_t	arqBlockSize_;


    /**
     * Unsolicited Grant Interval for UL service flows UGS and ertPS in Milliseconds (cp. IEEE 802.16-2009 11.13.19)
     */
    u_int16_t unsolicitedGrantInterval_;

    /**
     * Unsolicited Polling Interval for UL service flows in Milliseconds (cp. IEEE 802.16-2009 11.13.20)
     */
    u_int16_t unsolicitedPollingInterval_;

    /**
     * Type of Data Delivery Services for DL service flows (cp. IEEE 802.16-2009 11.13.24)
     */
    DataDeliveryServiceType_t dataDeliveryServiceType_;

    /**
     * Time Base parameter for rate measurement in milliseconds (cp. IEEE 802.16-2009 11.13.26)
     */
    u_int16_t timeBase_;

    /**
     * Packet Error Rate (PER) indicated the target error rate of the service flow
     * In this model the post ARQ error rate shall be taken  (cp. IEEE 802.16-2009 11.13.37)
     */
    double packetErrorRate_;

    // TODO: Not all possible QoS Parameters are included yet

};
#endif //SERVICEFLOWQOS_H

