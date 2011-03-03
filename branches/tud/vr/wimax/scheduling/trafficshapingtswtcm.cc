/*
 * trafficshapingtswtcm.cpp
 *
 *  Created on: 07.09.2010
 *      Author: richter
 */

#include "trafficshapingtswtcm.h"
#include "serviceflowqosset.h"
#include "serviceflow.h"
#include "connection.h"

TrafficShapingTswTcm::TrafficShapingTswTcm( double frameDuration )  : TrafficShapingInterface( frameDuration)
{
    // Nothing to do

}

TrafficShapingTswTcm::~TrafficShapingTswTcm()
{
    // Nothing to do
}

/*
 * Returns wantedMstrSize and wantedMrtrSize as guideline for the scheduling algorithm
 * through call by reference
 */
MrtrMstrPair_t TrafficShapingTswTcm::getDataSizes(Connection *connection)
{
    //-----------------------Initialization of the Map---------------------------------
    LastAllocationSizeIt_t mapIterator;
    int currentCid = connection->get_cid();

    // QoS Parameter auslesen
    ServiceFlowQosSet* sfQosSet = connection->getServiceFlow()->getQosSet();

    //ist die CID in Map ?
    // Suchen der CID in der Map - Eintrag vorhanden ?
    mapIterator = mapLastAllocationSize_.find(currentCid);

    u_int32_t wantedMrtrSize;
    u_int32_t wantedMstrSize;

    if ( mapIterator == mapLastAllocationSize_.end()) {

        // CID nicht vorhanden
        // mrtrsize berechnen  = 64Kbps VOIP -> 70Kbps
        wantedMrtrSize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );
        u_int32_t maxTrafficBurst = sfQosSet->getMaxTrafficBurst();
        /*------------nimm minimalen Wert der Datengröße-----------------------------------------------------------------------//
                //------------die angekommende Datengröße an MAC SAP Trasmitter ,die gerechneten Datengröße für mrtr und die maximale Rahmengröße--------*/
        wantedMrtrSize = MIN(wantedMrtrSize,maxTrafficBurst);
        wantedMrtrSize = MIN(wantedMrtrSize, u_int32_t(connection->queueByteLength()) );

        // mstrsize berechnen  = 25 Mbps MPEG4 AVC/H.264 -> 20 Mbps
        wantedMstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );
        /*------------nimm minimalen Wert der Datengröße----------------------------------------------------------------------//
                //------------die angekommende Datengröße an MAC SAP Trasmitter,die gerechneten Datengröße für mstr und die maximale Rahmengröße--------*/
        wantedMstrSize = MIN( wantedMstrSize, maxTrafficBurst);
        wantedMstrSize = MIN( wantedMstrSize, u_int32_t(connection->queueByteLength()) );

    } else {

        //----wenn das CID vorhanden ist,hole ich abgelegte Daten aus Strukture aus, um Datasize zu berechnen---//
        //----Strukturenlemente aus Map element  holen für die Berechnung des Prediction DataSize----//


        u_int32_t lastMrtr = mapIterator->second.lastMrtr;
        u_int32_t lastMstr = mapIterator->second.lastMstr;
        double lastTime = mapIterator->second.timeStamp;

        //-------------------------gültige Datasize ausholen-----------------------------------------//
        double timeBase = double(sfQosSet->getTimeBase()) * 1e-3; // time base in second

        /*
         * S_max <Byte> = T_b <second> * MSTR <Bit/second> - (T_b <second> - (current_Time - last_Time) <second> * last_MSTR <Bit/second> )
         */
        u_int32_t com_mstrSize = ((timeBase * sfQosSet->getMaxSustainedTrafficRate() - ( timeBase  - (NOW - lastTime)) * lastMstr) / 8);

        //S_max <Byte> = T_b <second> * MSTR <Bit/second> - (T_b <second> - (current_Time - last_Time) <second> * last_MSTR <Bit/second> )
        u_int32_t com_mrtrSize = ((timeBase * sfQosSet->getMinReservedTrafficRate() - (timeBase - (NOW - lastTime )) * lastMrtr) / 8);

        // ----------- gets maximal traffic burst in oder to compare with the computed DataSize----------------//
        u_int32_t maxTrafficBurst = sfQosSet->getMaxTrafficBurst();


        /*------------nimm minimalen Wert der Datengröße----------------------------------------------------------------------//
        //------------die angekommende Datengröße an MAC SAP Trasmitter,die gerechneten Datengröße für mstr und die maximale Rahmengröße--------*/
        com_mstrSize = MIN( com_mstrSize, maxTrafficBurst);
        wantedMstrSize = MIN( com_mstrSize, u_int32_t(connection->queueByteLength()) );

        /*------------nimm minimalen Wert der Datengröße-----------------------------------------------------------------------//
        //------------die angekommende Datengröße an MAC SAP Trasmitter ,die gerechneten Datengröße für mrtr und die maximale Rahmengröße--------*/
        com_mrtrSize = MIN(com_mrtrSize,maxTrafficBurst);
        wantedMrtrSize = MIN(com_mrtrSize, u_int32_t(connection->queueByteLength()) );

    }

    MrtrMstrPair_t mrtrMstrPair;
    mrtrMstrPair.first = wantedMrtrSize;
    mrtrMstrPair.second = wantedMstrSize;

    return mrtrMstrPair;
}

/*
 * Occurred allocation is send back to the traffic policing algorithm to use this
 * values in the next call
 */
void TrafficShapingTswTcm::updateAllocation(Connection *con,u_int32_t realMrtrSize,u_int32_t realMstrSize)
{
//---------- Pointer erstellen---------//
    LastAllocationSizeIt_t mapIterator;

//-----------Cid holen-----------------//
    int currentCid = con->get_cid();

//  get Windows Size < millisecond > from QoS Parameter Set
    double timeBase =  con->getServiceFlow()->getQosSet()->getTimeBase() * 1e-3 ; // in seconds

// die Position der Abbildung in der MAP mapLastAllocationSize_ entsprechende Cid  suchen-- verweist die  Position auf MAP Iterator//
    mapIterator = mapLastAllocationSize_.find(currentCid);


    /*	MAP mapLastAllocationSize_ bis zu MAP_ENDE durchsuchen , if the connection doesn't exist --> Error */

    if ( mapIterator == mapLastAllocationSize_.end()) {

        //--neues Map element erstellen-----bzw. neue Strukture erstellen, um aktuellen DatenSize und aktuellen Zeitpunkt ab zu legen----------//
        LastAllocationSize lastAllocationSize;

        // Berechnet Ak
        u_int32_t currentMrtr = ( realMrtrSize * 8 / frameDuration_ );
        u_int32_t currentMstr = ( realMstrSize * 8 / frameDuration_ );

        //----------Zuweisung auf Strukturelemente------------//
        lastAllocationSize.lastMrtr = currentMrtr;
        lastAllocationSize.lastMstr = currentMstr;
        lastAllocationSize.timeStamp = NOW;

        //------------neues Map element lastAllocationSize in MAP mapLastAllocationSize_ hinzufügen----------------------//
        mapLastAllocationSize_.insert( pair<int,LastAllocationSize>( currentCid, lastAllocationSize) );

        //---Ab jetzt können wir Cid und die Strukture lastAllocationSize(mrtrSize,mstrSize und timeStamp) nutzen-aus MAP--//


    } else	{



        //hole werte
        u_int32_t lastMrtr = mapIterator->second.lastMrtr;
        u_int32_t lastMstr = mapIterator->second.lastMstr;

        // berechne Zeitdifferent zum letzten Aufruf / T_D
        double timeDelta = NOW - mapIterator->second.timeStamp;


        /*--------------------------die aktuelle MRTR für die Info-Datenrate---------------------------------------------------------//
         * A_k <Bit/second> = ( T_b <second>_double * last_MRTR <Bit/s>_u_int32_t + last_DataSize <Bit>_u_int32_t ) / ( T_b <second>_double + nT_f <second > )
         */
        u_int32_t currentMrtr = ceil(( lastMrtr * timeBase + realMrtrSize * 8.0 ) / ( timeBase + timeDelta ));
        /*--------------------------die aktuelle MSTR für die Info-Datenrate---------------------------------------------------------//
         * A_k <Bit/second> = ( T_b <second>_double * last_MSTR <Bit/s>_u_int32_t + last_DataSize <Bit>_u_int32_t ) / ( T_b <second> + T_f <second > )
         */
        u_int32_t currentMstr = floor(( lastMstr * timeBase + realMstrSize * 8.0 ) / ( timeBase + timeDelta));


        //  Aktuellisierung der Werte  mstrSize mrtrSize in map
        mapIterator->second.lastMrtr = currentMrtr;
        mapIterator->second.lastMstr = currentMstr;
        mapIterator->second.timeStamp = NOW;


    }
}
