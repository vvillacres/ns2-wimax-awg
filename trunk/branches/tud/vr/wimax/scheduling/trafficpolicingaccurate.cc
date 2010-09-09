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
void TrafficPolicingAccurate::getDataSize(Connection *con, u_int32_t &wantedMstrSize, u_int32_t &wantedMrtrSize)
{
    //-----------------------Initialization of the Map---------------------------------
	AllocationListIt_t mapIterator;
    int currentCid = con->get_cid();

    // QoS Parameter auslesen
    ServiceFlowQosSet* sfQosSet = con->getServiceFlow()->getQosSet();

    //ist die CID in Map ?
    // Suchen der CID in der Map - Eintrag vorhanden ?
    mapIterator = mapLastAllocationList_.find(currentCid);

    if ( mapIterator == mapLastAllocationList_.end()) {
        // CID nicht vorhanden

        u_int32_t maxTrafficBurst = sfQosSet->getMaxTrafficBurst();

        // mrtrsize berechnen  = 64Kbps VOIP -> 70Kbps
        wantedMrtrSize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );
        wantedMrtrSize = MIN( wantedMrtrSize, maxTrafficBurst);
        wantedMrtrSize = MIN( wantedMrtrSize,u_int32_t(con->queueByteLength()));

         // mstrsize berechnen  = 25 Mbps MPEG4 AVC/H.264 -> 20 Mbps
        wantedMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );
        wantedMstrSize = MIN(wantedMstrSize, maxTrafficBurst);
        wantedMstrSize = MIN( wantedMstrSize, u_int32_t(con->queueByteLength()));

    } else {


        //----wenn das CID vorhanden ist,hole ich abgelegte Daten aus Strukture aus, um Datasize zu berechnen---//
        //----Strukturenlemente aus Map element  holen für die Berechnung des Prediction DataSize----//

       // double lastTime = mapIterator->second.ptrDequeMrtrSize->lastTime;

        //-------------------------gültige Datasize ausholen-----------------------------------------//
        double timeBase = double(sfQosSet->getTimeBase()) * 1e-3; // time base in second

        // maximum new allocated data volume = Time base(s)*mstr(bit/s) / 8 - Summe von MSTRSize(Byte)
         //                              	 = Byte
        u_int32_t com_mstrSize = (timeBase * sfQosSet->getMaxSustainedTrafficRate()/8 - mapIterator->second.sumMstrSize);

        // minimum new allocated data volume = Time base()
        u_int32_t com_mrtrSize = (timeBase * sfQosSet->getMinReservedTrafficRate()/8 - mapIterator->second.sumMrtrSize);

        //------------------------gets maximal traffic burst in oder to compare with the computed DataSize--------------------------//
        u_int32_t maxTrafficBurst = sfQosSet->getMaxTrafficBurst();
        //----------------------------------nimm minimalen Wert der Datengröße------------------------------------------------------//
        //------------die angekommende Datengröße an MAC SAP Trasmitter,die gerechneten Datengröße für mstr und die maximale Rahmengröße--------
        com_mstrSize = MIN( com_mstrSize, maxTrafficBurst);
        wantedMstrSize = MIN( com_mstrSize, u_int32_t(con->queueByteLength()));

        //--------------------------------------nimm minimalen Wert der Datengröße----------------------------------------------//
        //------------die angekommende Datengröße an MAC SAP Trasmitter ,die gerechneten Datengröße für mrtr und die maximale Rahmengröße-------
        com_mrtrSize = MIN(com_mrtrSize,maxTrafficBurst);
        wantedMrtrSize = MIN(com_mrtrSize,u_int32_t(con->queueByteLength()));

    }
}

/*
 * Sends occurred allocation back to traffic policing
 */
void TrafficPolicingAccurate::updateAllocation(Connection *con,u_int32_t wantedMrtrSize,u_int32_t wantedMstrSize)
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

    /*	MAP mapLastAllocationSize_ bis zu MAP_ENDE durchsuchen , if the connection doesn't exist --> Error */



    if ( mapIterator == mapLastAllocationList_.end()) {

    	// CID nicht vorhanden
    	//--neues Map element erstellen-----bzw. neue Strukture erstellen, um aktuellen DatenSize und aktuellen Zeitpunkt ab zu legen----------//

        AllocationList currentAllocationList;

        //----------Zuweisung auf Strukturelemente------------//
        currentAllocationList.sumMrtrSize = wantedMrtrSize;
        currentAllocationList.sumMstrSize = wantedMstrSize;
        //------------------Erstellen der genügenden Speicher für deque_mrtrSize  und deque_mstr <u_int_32_t>------------------------//

        currentAllocationList.ptrDequeAllocationElement = new deque<AllocationElement>;

        // Neuer deque eintrag

        AllocationElement currentAllocationElement;

    	// mrtrSize wieder berechnen  = 64Kbps VOIP -> 70Kbps
        u_int32_t defaultMrtrSize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

        // mstrSize wieder berechnen um angenommennen Datenvolumen in Deque hinzufügen

        u_int32_t defaultMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );


        // as long as the time base is not full , i will fill the default value into the deque
    	double defaulttimeStamp = NOW- frameDuration_;
        while((NOW - defaulttimeStamp) < (timeBase - TIMEBASEBOUNDARY*frameDuration_ ) )
    	 {
        	// default value of the deque elements
          currentAllocationElement.mrtrSize =  defaultMrtrSize;
          currentAllocationElement.mstrSize =  defaultMstrSize;
          currentAllocationElement.timeStamp =   -timeBase + frameDuration_ - defaulttimeStamp;

          //defaulttimeStamp -= currentAllocationElement.timeStamp;
          defaulttimeStamp -= frameDuration_;
          // update summe +
                  mapIterator->second.sumMrtrSize += defaultMrtrSize;
                  mapIterator->second.sumMstrSize += defaultMstrSize;
    	  // now i push the value into the front of the Deque
          currentAllocationList.ptrDequeAllocationElement->push_back( currentAllocationElement);

    	 }


        // now i fill the current element in the Deque

                        currentAllocationElement.mrtrSize = wantedMrtrSize;
                        currentAllocationElement.mstrSize = wantedMstrSize;
                        currentAllocationElement.timeStamp = NOW;
                        // update summe +
                               mapIterator->second.sumMrtrSize += defaultMrtrSize;
                               mapIterator->second.sumMstrSize += defaultMstrSize;
                        // Neuen Eintrag in deque spreichern
                        currentAllocationList.ptrDequeAllocationElement->push_back( currentAllocationElement);


        // aktuelle AllocationList in der Map speichern
        //------------neues Map element lastAllocationSize in MAP mapLastAllocationSize_ hinzufügen----------------------//
        mapLastAllocationList_.insert( pair<int,AllocationList>( currentCid, currentAllocationList) );

        //---Ab jetzt können wir Cid und die Strukture lastAllocationSize(mrtrSize,mstrSize und timeStamp) nutzen-aus MAP--//

    } else {

        // Wenn der Cid vorhanden ist,lege ich neues Allocation Element an
        AllocationElement currentAllocationElement;
        // Neue Werte speichern
        currentAllocationElement.mrtrSize = wantedMrtrSize;
        currentAllocationElement.mstrSize = wantedMstrSize;
        currentAllocationElement.timeStamp = NOW;

        //-----in Deque speichern
        mapIterator->second.ptrDequeAllocationElement->push_back(currentAllocationElement);

        // update summe +
        mapIterator->second.sumMrtrSize += wantedMrtrSize;
        mapIterator->second.sumMstrSize += wantedMstrSize;

        //----Überprüfen ob Element entfernt werden muss ?

        AllocationElement begin_deque ;
        begin_deque = mapIterator->second.ptrDequeAllocationElement->front();

        double time_begin = begin_deque.timeStamp;
        // solange (time_end - time_begin) > timeBase noch wahr ist ,lösche ich letzte Element mit pop_front()
        while(( NOW - time_begin) >= ( timeBase - TIMEBASEBOUNDARY * frameDuration_)){
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
/* Beispiel :
 * FRAMEDURATION 0.005,
 *  time base = 25 ms,
 * 1 pakcet 100Byte kommt aller 5 ms
 * darauf kommt es:
 * mrtr = 800 bit 10+e3/5 sekunden = 160 Kbps (bit/sec).
 *
 *
 *
 *wantedMrtrSize = 160*1e3  * frameDuration_ / 8.0
 *wantedMrtrSize = 100 Byte
        wantedMrtrSize = MIN( wantedMrtrSize, maxTrafficBurst);
        wantedMrtrSize = MIN( wantedMrtrSize,u_int32_t(con->queueByteLength()));
 *
 * 1.Iteration
 * 1.getDataSize(Connection *con,u_int32_t & wantedmstrSize,u_int32_t & wantedmrtrSize ) wurde aufgerufen.
 * Kein Verbindung existiert
 * 1. wantedmrtrSize = 160 Kbps * 5-e3 / 8 = 100 Byte ;
 *
 *
 * 1.updateAllocation(Connection *con,u_int32_t wantedmstrSize ,u_int32_t wantedmrtrSize = 100 Byte)
 * bei 1.Iteration wurde die mrtrSize in die Summegröße Variable hinzugefügt .
 *
 * MAP: Size:		100
 *      TempStamp:	0
 *
 * // Neuer deque eintrag
		AllocationElement currentAllocationElement;

 * currentAllocationList.sumMrtrSize = mrtrSize;
 *                       sumMrtrSize = 100 Byte
 * currentAllcoationElement.timeStamp = 0;
 * defaultTimeStamp = 0 - 5
 *
 * 1.Iteration der While schleife ((NOW(0)-(-5) < 17.5 )
 *   mrtrSize = 100 Byte;
 *   TimeStamp = - 25 + 5 +5 = -15
 *   Summe = 100
 *   defaultTimeStamp = -5 -5 = -10;
 *  füge ein neuer Deque-Eintrag in Deque hinzu
 *        ----------------Deque----------
 *        |                          100|
 *        |------------------------------
 *        |                          -15|
 *        |------------------------------
 *
 * 2.Iteration der While schleife ((NOW(0)-(-10) < 17.5 )
 *   mrtrSize = 100 Byte = defaultTimeStamp;
 *   TimeStamp = - 25 + 5 + 10 = -10;
 *   Summe = 200
 *   defaultTimeStamp = -5-10 = -15;
 *        ----------------Deque----------
 *        |                    |100 |100|
 *        |------------------------------
 *        |                    |-15 |-10|
 *        |------------------------------
 *         füge ein neuer Deque-Eintrag in Deque hinzu
 * 3.Iteration der While schleife ((NOW(0)-(-15) < 17.5 )
 *   mrtrSize = 100 Byte = defaultTimeStamp;
 *   TimeStamp = - 15 = defaulttimeStamp;
 *   Summe = 300
 *   defaultTimeStamp = -10-10 = -20;
 *
 *        ----------------Deque-----------
 *        |                 |100|100 |100|
 *        |-------------------------------
 *        |                 |-15|-10 |-5 |
 *        |-------------------------------
 *    füge ein neuer Deque-Eintrag in Deque hinzu
 * ausser while schliefe
 *    mrtrSize = 100 Byte
 *   TimeStamp = 0
 *   Summe = 400
 *
 *        -----------------Deque---------------
 *        |                 |100|100 |100|100 |
 *        |-----------------------------------|
 *        |                 |-15|-10 |-5 | 0  |
 *        -------------------------------------
2.iteration
2.getDataSize()
2. mrtrSize = 160 Kbps * 25-e3 - 100
            = 500 Byte - 100 Byte = 400 Byte
2. UpdateAllocation(wantedMrtrSize)
 AllocationElement currentAllocationElement;
        currentAllocationElement.mrtrSize = wantedMrtrSize =100;
          currentAllocationElement.timeStamp = NOW = 5 ms;
 // Neuen Eintrag in deque spreichern
                currentAllocationList.ptrDequeAllocationElement->push_back( currentAllocationElement);

 *
 *
 *
 *        ----------------Deque--------------------
 *        |                 |100|100 |100|100 |100|
 *        |----------------------------------------
 *        |                 |-15|-10 |-5 | 0  | 5 |
 *        |---------------------------------------|
 *
 *
 *
*/
