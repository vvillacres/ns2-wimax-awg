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

TrafficPolicingAccurate::TrafficPolicingAccurate(double frameDuration) : TrafficPolicingInterface( frameDuration)
{
    // Nothing to do
}

TrafficPolicingAccurate::~TrafficPolicingAccurate()
{
    // Nothing to do
}


/*
 * Calculate predicted mrtr and msrt sizes
 */
void TrafficPolicingAccurate::getDataSize(Connection *con, u_int32_t &wantedMstrSize, u_int32_t &wantedMrtrSize)
{
    //-----------------------Initialization of the Map---------------------------------
	AllocationSizeIt_t mapIterator;
    int currentCid = con->get_cid();

    // QoS Parameter auslesen
    ServiceFlowQosSet* sfQosSet = con->getServiceFlow()->getQosSet();

    //ist die CID in Map ?
    // Suchen der CID in der Map - Eintrag vorhanden ?
    mapIterator = mapLastAllocationSize_.find(currentCid);

    if ( mapIterator == mapLastAllocationSize_.end()) {
        // CID nicht vorhanden

        // mrtrsize berechnen  = 64Kbps VOIP -> 70Kbps
        wantedMrtrSize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

        // mstrsize berechnen  = 25 Mbps MPEG4 AVC/H.264 -> 20 Mbps
        wantedMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );


    } else {

    	/*
        //----wenn das CID vorhanden ist,hole ich abgelegte Daten aus Strukture aus, um Datasize zu berechnen---//
        //----Strukturenlemente aus Map element  holen für die Berechnung des Prediction DataSize----//

       // double lastTime = mapIterator->second.ptrDequeMrtrSize->lastTime;

        //-------------------------gültige Datasize ausholen-----------------------------------------//
        double timeBase = double(sfQosSet->getTimeBase()) * 1e-3; // time base in second

        //
         / S_k <Bit> = T_b <second> * MSTR <Bit/second> - (T_b <second> - (current_Time - last_Time) <second> * last_MSTR <Bit/second> )
         //
        u_int32_t com_mstrSize = (timeBase * sfQosSet->getMaxSustainedTrafficRate() - ( timeBase  - (NOW - timeStamp)) * lastmstr);

        //S_k <Bit> = T_b <second> * MSTR <Bit/second> - (T_b <second> - (current_Time - last_Time) <second> * last_MSTR <Bit/second> )
        u_int32_t com_mrtrSize = (timeBase * sfQosSet->getMinReservedTrafficRate() - (timeBase - (NOW - timeStamp )) * lastmrtr);

        //------------------------gets maximal traffic burst in oder to compare with the computed DataSize--------------------------//
        u_int32_t maxTrafficBurst = sfQosSet->getmaxTrafficBurst();
        //----------------------------------nimm minimalen Wert der Datengröße------------------------------------------------------//
        //------------die angekommende Datengröße an MAC SAP Trasmitter,die gerechneten Datengröße für mstr und die maximale Rahmengröße--------
        com_mstrSize = MIN( com_mstrSize, maxTrafficBurst);
        mstrSize = MIN( com_mstrSize, con->queueByteLength());

        //--------------------------------------nimm minimalen Wert der Datengröße----------------------------------------------//
        //------------die angekommende Datengröße an MAC SAP Trasmitter ,die gerechneten Datengröße für mrtr und die maximale Rahmengröße-------
        com_mrtrSize = MIN(com_mrtrSize,maxTrafficBurst);
        mrtrSize = MIN(com_mrtrSize,con->queueByteLength());
        */
    }
}

/*
 * Sends occurred allocation back to traffic policing
 */
void TrafficPolicingAccurate::updateAllocation(Connection *con,u_int32_t realMstrSize,u_int32_t realMrtSize)
{
    //---------- Pointer erstellen---------//
    LastAllocationSizeIt_t mapIterator;

    //-----------Cid holen-----------------//
    int currentCid = con->get_cid();

    //  get Windows Size < millisecond > from QoS Parameter Set
    double timeBase =  con->get_serviceflow()->getQoS()->getTimeBase() * 1e-3 ; // in seconds

    // die Position der Abbildung in der MAP mapLastAllocationSize_ entsprechende Cid  suchen-- verweist die  Position auf MAP Iterator//
    mapIterator = mapLastAllocationSize_.find(currentCid);

    /*	MAP mapLastAllocationSize_ bis zu MAP_ENDE durchsuchen , if the connection doesn't exist --> Error */

    if ( mapIterator == mapLastAllocationSize_.end()) {
        //--neues Map element erstellen-----bzw. neue Strukture erstellen, um aktuellen DatenSize und aktuellen Zeitpunkt ab zu legen----------//

        AllocationList currentAllocationList;

        //----------Zuweisung auf Strukturelemente------------//
        currentAllocationList.sumMrtrSize = realMrtrSize;
        currentAllocationList.sumMstrSize = realMstrSize;
        //------------------Erstellen der genügenden Speicher für deque_mrtrSize  und deque_mstr <u_int_32_t>------------------------//

        currentAllocationList.ptrDequeAllocationElement = new deque<AllocationElement>;


        // Neuer deque eintrag
        AllocationElement currentAllocationElement;

        currentAllocationElement.mrtrSize = realMrtrSize;
        currentAllocationElement.mstrSize = realMstrSize;
        currentAllcoationElement.timeStamp = NOW;

        // Neuen Eintrag in deque spreichern
        currentAllocationList.ptrDequeAllocationElement->push_back( currentAllocationElement);

        // aktuelle AllocationList in der Map speichern
        //------------neues Map element lastAllocationSize in MAP mapLastAllocationSize_ hinzufügen----------------------//
        mapLastAllocationSize_.insert( pair<int,AllocationList>( currentCid, currentAllocationList) );

        //---Ab jetzt können wir Cid und die Strukture lastAllocationSize(mrtrSize,mstrSize und timeStamp) nutzen-aus MAP--//

    } else {

        // Wenn der Cid vorhanden ist,lege ich neues Allocation Element an
        AllocationElement currentAllocationElement;
        // Neue Werte speichern
        currentAllocationElement.mrtrSize = realMrtrSize;
        currentAllocationElement.mstrSize = realMstrSize;
        currentAllocationElement.timeStamp = NOW;

        //-----in Deque speichern
        mapIterator->second->ptrDequeAllocationElement->push_back(currentAllocationElement);

        // update summe +
        mapIterator->second->sumMrtrSize += realMrtrSize;
        mapIterator->second->sumMstrSize += realMstrSize;

        //----Überprüfen ob Element entfernt werden muss ?

        AllocationElement begin_deque ;
        begin_deque = mapIterator->second->ptrDequeAllocationElement->front();

        double time_begin = begin_deque.timeStamp;
        // solange (time_end - time_begin) > timeBase noch wahr ist ,lösche ich letzte Element mit pop_front()
        while(( NOW - time_begin) >= timeBase) {
            //------update summe - ,ich subtrahiere begin_deque von meine Summe
            mapIterator->second->sumMrtrSize -= begin_deque.mrtrSize;
            mapIterator->second->sumMstrSize -= begin_deque.mstrSize;

            //------und das begin element löschen
            mapIterator->second->ptrDequeAllocationElement->pop_front();

            //------neues begin laden
            begin_deque   = mapIterator->second->ptrDequeAllocationElement->front();
            time_begin   = begin_deque.timeStamp;
        }


    }
}
