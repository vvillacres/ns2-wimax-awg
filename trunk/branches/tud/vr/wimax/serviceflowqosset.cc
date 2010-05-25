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

#include "serviceflowqosset.h"

/**
 * Default Constructor
 */
ServiceFlowQosSet::ServiceFlowQosSet ()
{

}

/**
 * Constructor
 */
ServiceFlowQosSet::ServiceFlowQosSet (
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
		)
{
	trafficPriority_ = trafficPriority;
	maxSustainedTrafficRate_ = maxSustainedTrafficRate;
	maxTrafficBurst_ = maxTrafficBurst;
	minReservedTrafficRate_ = minReservedTrafficRate;
	ulGrantSchedulingType_ = ulGrantSchedulingType;
	reqTransmitPolicy_ = reqTransmitPolicy;
	toleratedJitter_ = toleratedJitter;
	maxLatency_ = maxLatency;
	isFixedLengthSdu_ = isFixedLengthSdu;
	sduSize_ = sduSize;
	isArqEnabled_ = isArqEnabled;
	arqMaxWindow_ = arqMaxWindow;
	arqBlockSize_ = arqBlockSize;
	unsolicitedGrantInterval_ = unsolicitedGrantInterval;
	unsolicitedPollingInterval_ = unsolicitedPollingInterval;
	dataDeliveryServiceType_ = dataDeliveryServiceType;
	timeBase_ = timeBase;
	packetErrorRate_ = packetErrorRate;
}


