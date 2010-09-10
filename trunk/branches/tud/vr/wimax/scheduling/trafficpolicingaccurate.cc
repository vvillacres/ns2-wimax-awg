/*
 * trafficpolicingaccurate.cpp
 *
 *  Created on: 07.09.2010
 *      Author: tung & richter
 */

#include "trafficpolicingaccurate.h"
#include "serviceflowqosset.h"
#include "serviceflow.h"
#include "connection.h"


/*
 * Timebaseboundary * FrameDuration defines the time when an deque elements is removed
 */
#define TIMEBASEBOUNDARY 1.5

TrafficPolicingAccurate::TrafficPolicingAccurate(double frameDuration) : TrafficPolicingInterface( frameDuration)
{
    // Nothing to do
}

TrafficPolicingAccurate::~TrafficPolicingAccurate()
{
    //deque mrtr und mstr delete ausführen
    AllocationListIt_t mapIterator;
    // ich laufe von Anfang bis Ende des MAP
    for(mapIterator=mapLastAllocationList_.begin(); mapIterator!=mapLastAllocationList_.end(); mapIterator++) {
        // lösche komplete deque object
        delete mapIterator->second.ptrDequeAllocationElement;

    }
}


/*
 * Calculate predicted mrtr and msrt sizes
 */
MrtrMstrPair_t TrafficPolicingAccurate::getDataSize(Connection *connection)
{
    //-----------------------Initialization of the Map---------------------------------
    AllocationListIt_t mapIterator;
    int currentCid = connection->get_cid();

    // QoS Parameter auslesen
    ServiceFlowQosSet* sfQosSet = connection->getServiceFlow()->getQosSet();

    //ist die CID in Map ?
    // Suchen der CID in der Map - Eintrag vorhanden ?
    mapIterator = mapLastAllocationList_.find(currentCid);

    u_int32_t wantedMrtrSize;
    u_int32_t wantedMstrSize;

    if ( mapIterator == mapLastAllocationList_.end()) {
        // CID nicht vorhanden

        // mrtrsize berechnen  = 64Kbps VOIP -> 70Kbps
        wantedMrtrSize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

        // mstrsize berechnen  = 25 Mbps MPEG4 AVC/H.264 -> 20 Mbps
        wantedMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );


    } else {


        //----wenn das CID vorhanden ist,hole ich abgelegte Daten aus Strukture aus, um Datasize zu berechnen---//
        //----Strukturenlemente aus Map element  holen für die Berechnung des Prediction DataSize----//


        //-------------------------gültige Datasize ausholen-----------------------------------------//
        double timeBase = double(sfQosSet->getTimeBase()) * 1e-3; // time base in second

        // minimum new allocated data volume = Time base()
        wantedMrtrSize = ( timeBase * sfQosSet->getMinReservedTrafficRate() / 8 ) - mapIterator->second.sumMrtrSize;

        // maximum new allocated data volume = Time base(s)*mstr(bit/s) / 8 - Summe von MSTRSize(Byte)
        //                              	 = Byte
        wantedMstrSize = ( timeBase * sfQosSet->getMaxSustainedTrafficRate() / 8 ) - mapIterator->second.sumMstrSize;

    }

    // get Maximum Traffic Burst Size for this connection
    u_int32_t maxTrafficBurst = sfQosSet->getMaxTrafficBurst();

    // min function Mrtr according to IEEE 802.16-2009
    wantedMrtrSize = MIN( wantedMrtrSize, maxTrafficBurst);
    wantedMrtrSize = MIN( wantedMrtrSize,u_int32_t(connection->queueByteLength()));

    // min function for Mstr
    wantedMstrSize = MIN(wantedMstrSize, maxTrafficBurst);
    wantedMstrSize = MIN( wantedMstrSize, u_int32_t(connection->queueByteLength()));

    MrtrMstrPair_t mrtrMstrPair;
    mrtrMstrPair.first = wantedMrtrSize;
    mrtrMstrPair.second = wantedMstrSize;

    return mrtrMstrPair;

}

/*
 * Sends occurred allocation back to traffic policing
 */
void TrafficPolicingAccurate::updateAllocation(Connection *con,u_int32_t realMstrSize,u_int32_t realMrtrSize)
{
    //---------- Pointer erstellen---------//
    MapLastAllocationList_t::iterator mapIterator;

    // QoS Parameter auslesen
    ServiceFlowQosSet* sfQosSet = con->getServiceFlow()->getQosSet();
    //-----------Cid holen-----------------//
    int currentCid = con->get_cid();


    //  get Windows Size < millisecond > from QoS Parameter Set
    double timeBase = double(sfQosSet->getTimeBase()) * 1e-3; // time base in second

    // die Position der Abbildung in der MAP mapLastAllocationSize_ entsprechende Cid  suchen-- verweist die  Position auf MAP Iterator//
    mapIterator = mapLastAllocationList_.find(currentCid);

    //	MAP mapLastAllocationSize_ bis zu MAP_ENDE durchsuchen

    if ( mapIterator == mapLastAllocationList_.end()) {

        // CID nicht vorhanden
        //--neues Map element erstellen-----bzw. neue Strukture erstellen, um aktuellen DatenSize und aktuellen Zeitpunkt ab zu legen----------//

        AllocationList currentAllocationList;

        //----------Zuweisung auf Strukturelemente------------//
        currentAllocationList.sumMrtrSize = realMrtrSize;
        currentAllocationList.sumMstrSize = realMstrSize;
        //------------------Erstellen der genügenden Speicher für deque_mrtrSize  und deque_mstr <u_int_32_t>------------------------//

        currentAllocationList.ptrDequeAllocationElement = new deque<AllocationElement>;

        // Neuer deque eintrag

        AllocationElement currentAllocationElement;
        // now i fill the current element in the Deque

        currentAllocationElement.mrtrSize = realMrtrSize;
        currentAllocationElement.mstrSize = realMstrSize;
        currentAllocationElement.timeStamp = NOW;

        // Neuen Eintrag in deque spreichern
        currentAllocationList.ptrDequeAllocationElement->push_front( currentAllocationElement);



        // Fill Deque with default values to avoid start oscillations

        // mrtrSize wieder berechnen  = 64Kbps VOIP -> 70Kbps
        u_int32_t defaultMrtrSize = u_int32_t( floor( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

        // mstrSize wieder berechnen um angenommennen Datenvolumen in Deque hinzufügen

        u_int32_t defaultMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );


        // as long as the time base is not full , i will fill the default value into the deque
        double defaultTimeStamp = NOW - frameDuration_;
        while((NOW - defaultTimeStamp) < (timeBase - TIMEBASEBOUNDARY * frameDuration_ ) ) {
            // default value of the deque elements
            currentAllocationElement.mrtrSize =  defaultMrtrSize;
            currentAllocationElement.mstrSize =  defaultMstrSize;
            currentAllocationElement.timeStamp = defaultTimeStamp;

            // now i push the value into the front of the Deque
            currentAllocationList.ptrDequeAllocationElement->push_front( currentAllocationElement);

            // update sum values +
            currentAllocationList.sumMrtrSize += defaultMrtrSize;
            currentAllocationList.sumMstrSize += defaultMstrSize;

            // decrease defaultTimeStamp
            defaultTimeStamp -= frameDuration_;

        }


        // aktuelle AllocationList in der Map speichern
        //------------neues Map element lastAllocationSize in MAP mapLastAllocationSize_ hinzufügen----------------------//
        mapLastAllocationList_.insert( pair<int,AllocationList>( currentCid, currentAllocationList) );

        //---Ab jetzt können wir Cid und die Strukture lastAllocationSize(mrtrSize,mstrSize und timeStamp) nutzen-aus MAP--//

    } else {

        // Wenn der Cid vorhanden ist,lege ich neues Allocation Element an
        AllocationElement currentAllocationElement;
        // Neue Werte speichern
        currentAllocationElement.mrtrSize = realMrtrSize;
        currentAllocationElement.mstrSize = realMstrSize;
        currentAllocationElement.timeStamp = NOW;

        //-----in Deque speichern
        mapIterator->second.ptrDequeAllocationElement->push_back(currentAllocationElement);

        // update summe +
        mapIterator->second.sumMrtrSize += realMrtrSize;
        mapIterator->second.sumMstrSize += realMstrSize;

        //----Überprüfen ob Element entfernt werden muss ?

        AllocationElement begin_deque ;
        begin_deque = mapIterator->second.ptrDequeAllocationElement->front();

        double time_begin = begin_deque.timeStamp;
        // solange (time_end - time_begin) > timeBase noch wahr ist ,lösche ich letzte Element mit pop_front()
        while(( NOW - time_begin) >= ( timeBase - TIMEBASEBOUNDARY * frameDuration_)) {
            //------update summe - ,ich subtrahiere begin_deque von meine Summe
            mapIterator->second.sumMrtrSize -= begin_deque.mrtrSize;
            mapIterator->second.sumMstrSize -= begin_deque.mstrSize;

            //------und das begin element löschen
            mapIterator->second.ptrDequeAllocationElement->pop_front();

            //------neues begin laden
            begin_deque   = mapIterator->second.ptrDequeAllocationElement->front();
            time_begin   = begin_deque.timeStamp;
        }
    }
}

