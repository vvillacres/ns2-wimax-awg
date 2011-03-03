/*
 * trafficshapingaccurate.cpp
 *
 *  Created on: 07.09.2010
 *      Author: tung & richter
 */

#include "trafficshapingaccurate.h"
#include "serviceflowqosset.h"
#include "serviceflow.h"
#include "connection.h"


/*
 * Timebaseboundary * FrameDuration defines the time when an deque elements is removed
 */
#define TIMEBASEBOUNDARY 1.5

TrafficShapingAccurate::TrafficShapingAccurate(double frameDuration) : TrafficShapingInterface( frameDuration)
{
    // Nothing to do
}

TrafficShapingAccurate::~TrafficShapingAccurate()
{
    //deque mrtr und mstr delete ausführen
    TrafficShapingListIt_t mapIterator;
    // ich laufe von Anfang bis Ende des MAP
    for(mapIterator=mapLastTrafficShapingList_.begin(); mapIterator!=mapLastTrafficShapingList_.end(); mapIterator++) {
        // lösche komplete deque object
        delete mapIterator->second.ptrDequeTrafficShapingElement;

    }
}


/*
 * Calculate predicted mrtr and msrt sizes
 */
MrtrMstrPair_t TrafficShapingAccurate::getDataSizes(Connection *connection)
{
	// for debug
	/*
	if ( NOW >= 11.008166  ) {
		printf("Breakpoint reached \n");
	}
	*/


	//-----------------------Initialization of the Map---------------------------------
    TrafficShapingListIt_t mapIterator;
    int currentCid = connection->get_cid();

    // QoS Parameter auslesen
    ServiceFlowQosSet* sfQosSet = connection->getServiceFlow()->getQosSet();

    //ist die CID in Map ?
    // Suchen der CID in der Map - Eintrag vorhanden ?
    mapIterator = mapLastTrafficShapingList_.find(currentCid);

    u_int32_t wantedMrtrSize;
    u_int32_t wantedMstrSize;

    if ( mapIterator == mapLastTrafficShapingList_.end()) {
        // CID nicht vorhanden

        // mrtrsize berechnen  = 64Kbps VOIP -> 70Kbps
        wantedMrtrSize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

        // mstrsize berechnen  = 25 Mbps MPEG4 AVC/H.264 -> 20 Mbps
        wantedMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );

        // rounding error
        if ( wantedMstrSize < wantedMrtrSize) {
        	wantedMstrSize = wantedMrtrSize;
        }


    } else {


        //----wenn das CID vorhanden ist,hole ich abgelegte Daten aus Strukture aus, um Datasize zu berechnen---//
        //----Strukturenlemente aus Map element  holen für die Berechnung des Prediction DataSize----//


        //-------------------------gültige Datasize ausholen-----------------------------------------//
        double timeBase = double(sfQosSet->getTimeBase()) * 1e-3; // time base in second

        printf("Current sumMrtrSize %d sumMstrSize %d \n", mapIterator->second.sumMrtrSize, mapIterator->second.sumMstrSize);

        // minimum new allocated data volume = Time base()
        wantedMrtrSize = ( timeBase * sfQosSet->getMinReservedTrafficRate() / 8 ) - mapIterator->second.sumMrtrSize;

        // maximum new allocated data volume = Time base(s)*mstr(bit/s) / 8 - Summe von MSTRSize(Byte)
        //                              	 = Byte
        wantedMstrSize = ( timeBase * sfQosSet->getMaxSustainedTrafficRate() / 8 ) - mapIterator->second.sumMstrSize;


        assert(wantedMrtrSize <= ( timeBase * sfQosSet->getMinReservedTrafficRate() / 8 ));
        assert(wantedMstrSize <= ( timeBase * sfQosSet->getMaxSustainedTrafficRate() / 8 ));
        assert(wantedMrtrSize <= wantedMstrSize);

    }

    // get Maximum Traffic Burst Size for this connection
    u_int32_t maxTrafficBurst = sfQosSet->getMaxTrafficBurst();

    //
    u_int32_t payloadSize = u_int32_t( connection->queuePayloadLength());
    if ( connection->getFragmentBytes() > 0 ) {
    	payloadSize -= connection->getFragmentBytes() - HDR_MAC802_16_SIZE - HDR_MAC802_16_FRAGSUB_SIZE;
    }

    // min function Mrtr according to IEEE 802.16-2009
    wantedMrtrSize = MIN( wantedMrtrSize, maxTrafficBurst);
    wantedMrtrSize = MIN( wantedMrtrSize, payloadSize);

    // min function for Mstr
    wantedMstrSize = MIN(wantedMstrSize, maxTrafficBurst);
    wantedMstrSize = MIN( wantedMstrSize, payloadSize);

    MrtrMstrPair_t mrtrMstrPair;
    mrtrMstrPair.first = wantedMrtrSize;
    mrtrMstrPair.second = wantedMstrSize;


    return mrtrMstrPair;

}

/*
 * Sends occurred allocation back to traffic policing
 */
void TrafficShapingAccurate::updateAllocation(Connection *con,u_int32_t realMrtrSize,u_int32_t realMstrSize)
{
    //---------- Pointer erstellen---------//
    MapLastTrafficShapingList_t::iterator mapIterator;

    // QoS Parameter auslesen
    ServiceFlowQosSet* sfQosSet = con->getServiceFlow()->getQosSet();
    //-----------Cid holen-----------------//
    int currentCid = con->get_cid();

    // debug
    assert(realMrtrSize <= realMstrSize);

    //  get Windows Size < millisecond > from QoS Parameter Set
    double timeBase = double(sfQosSet->getTimeBase()) * 1e-3; // time base in second

    // die Position der Abbildung in der MAP mapLastAllocationSize_ entsprechende Cid  suchen-- verweist die  Position auf MAP Iterator//
    mapIterator = mapLastTrafficShapingList_.find(currentCid);

    //	MAP mapLastAllocationSize_ bis zu MAP_ENDE durchsuchen

    if ( mapIterator == mapLastTrafficShapingList_.end()) {

        // CID nicht vorhanden
        //--neues Map element erstellen-----bzw. neue Strukture erstellen, um aktuellen DatenSize und aktuellen Zeitpunkt ab zu legen----------//

    	TrafficShapingList currentAllocationList;

        //----------Zuweisung auf Strukturelemente------------//
        currentAllocationList.sumMrtrSize = realMrtrSize;
        currentAllocationList.sumMstrSize = realMstrSize;
        //------------------Erstellen der genügenden Speicher für deque_mrtrSize  und deque_mstr <u_int_32_t>------------------------//

        currentAllocationList.ptrDequeTrafficShapingElement = new deque<TrafficShapingElement>;

        // Neuer deque eintrag

        TrafficShapingElement currentTrafficShapingElement;
        // now i fill the current element in the Deque

        currentTrafficShapingElement.mrtrSize = realMrtrSize;
        currentTrafficShapingElement.mstrSize = realMstrSize;
        currentTrafficShapingElement.timeStamp = NOW;

        // Neuen Eintrag in deque spreichern
        currentAllocationList.ptrDequeTrafficShapingElement->push_front( currentTrafficShapingElement);


        // Fill Deque with default values to avoid start oscillations

        // mrtrSize wieder berechnen  = 64Kbps VOIP -> 70Kbps
        u_int32_t defaultMrtrSize = u_int32_t( floor( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

        // mstrSize wieder berechnen um angenommennen Datenvolumen in Deque hinzufügen

        u_int32_t defaultMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );

        // avoid rounding errors
        if ( defaultMrtrSize > defaultMstrSize) {
        	defaultMstrSize = defaultMrtrSize;
        }


        // as long as the time base is not full , i will fill the default value into the deque
        double defaultTimeStamp = NOW - frameDuration_;
        while((NOW - defaultTimeStamp) < (timeBase - TIMEBASEBOUNDARY * frameDuration_ ) ) {
            // default value of the deque elements
            currentTrafficShapingElement.mrtrSize =  defaultMrtrSize;
            currentTrafficShapingElement.mstrSize =  defaultMstrSize;
            currentTrafficShapingElement.timeStamp = defaultTimeStamp;

            // now i push the value into the front of the Deque
            currentAllocationList.ptrDequeTrafficShapingElement->push_front( currentTrafficShapingElement);

            // update sum values +
            currentAllocationList.sumMrtrSize += defaultMrtrSize;
            currentAllocationList.sumMstrSize += defaultMstrSize;

            // decrease defaultTimeStamp
            defaultTimeStamp -= frameDuration_;

        }
        printf("Current lenght %d sumMrtrSize %d sumMstrSize %d \n", int(currentAllocationList.ptrDequeTrafficShapingElement->size()), currentAllocationList.sumMrtrSize, currentAllocationList.sumMstrSize);

        // aktuelle AllocationList in der Map speichern
        //------------neues Map element lastAllocationSize in MAP mapLastAllocationSize_ hinzufügen----------------------//
        mapLastTrafficShapingList_.insert( pair<int,TrafficShapingList>( currentCid, currentAllocationList) );

        //---Ab jetzt können wir Cid und die Strukture lastAllocationSize(mrtrSize,mstrSize und timeStamp) nutzen-aus MAP--//

    } else {

        // Wenn der Cid vorhanden ist,lege ich neues Allocation Element an
        TrafficShapingElement currentTrafficShapingElement;
        // Neue Werte speichern
        currentTrafficShapingElement.mrtrSize = realMrtrSize;
        currentTrafficShapingElement.mstrSize = realMstrSize;
        currentTrafficShapingElement.timeStamp = NOW;

        //-----in Deque speichern
        mapIterator->second.ptrDequeTrafficShapingElement->push_back(currentTrafficShapingElement);

        // update summe +
        mapIterator->second.sumMrtrSize += realMrtrSize;
        mapIterator->second.sumMstrSize += realMstrSize;



        //----Überprüfen ob Element entfernt werden muss ?

        TrafficShapingElement begin_deque ;
		begin_deque = mapIterator->second.ptrDequeTrafficShapingElement->front();

		double time_begin = begin_deque.timeStamp;
		// solange (time_end - time_begin) > timeBase noch wahr ist ,lösche ich letzte Element mit pop_front()
		while(( NOW - time_begin) >= ( timeBase - TIMEBASEBOUNDARY * frameDuration_)) {
			//------update summe - ,ich subtrahiere begin_deque von meine Summe
			mapIterator->second.sumMrtrSize -= begin_deque.mrtrSize;
			mapIterator->second.sumMstrSize -= begin_deque.mstrSize;

			//------und das begin element löschen
			mapIterator->second.ptrDequeTrafficShapingElement->pop_front();

			//------neues begin laden
			begin_deque   = mapIterator->second.ptrDequeTrafficShapingElement->front();
			time_begin   = begin_deque.timeStamp;
		}

		printf("Current lenght %d sumMrtrSize %d sumMstrSize %d \n", int(mapIterator->second.ptrDequeTrafficShapingElement->size()), mapIterator->second.sumMrtrSize, mapIterator->second.sumMstrSize);
		assert(mapIterator->second.sumMrtrSize <= mapIterator->second.sumMstrSize);
    }

}

