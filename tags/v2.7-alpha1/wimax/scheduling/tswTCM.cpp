/*
 * tswTCM.cpp
 *
 *  Created on: 08.07.2010
 *      Author: tung
 */

#include "tswTCM.h"
using namespace std;

//---------------------------------Contructor------------------------------
tswTCM::tswTCM() {
	// TODO Auto-generated constructor stub
}
//---------------------------------Initialitation--------------------------

tswTCM::tswTCM( double frameDuration) {
	frameDuration_ = frameDuration;
}
//---------------------------------Decontructor----------------------------
tswTCM::~tswTCM() {
	// TODO Auto-generated destructor stub
}

//--------------------------------getting Data Size ------------------------------
// Scheduler fragt nach den maximal möglichen Zuteilung (Datenvolumen)
void tswTCM::getDataSize(Connection *con,u_int32_t &mstrSize,u_int32_t &mrtrSize )
    {
	//-----------------------Initialization of the Map---------------------------------
	LastAllocationSizeIt_t mapIterator;
	int currentCid = con->get_cid();

	// QoS Parameter auslesen
	ServiceFlowQosSet sfQosSet = con->getServiceFlow()->getQosSet();

	//ist die CID in Map ?
	// Suchen der CID in der Map - Eintrag vorhanden ?
	mapIterator = mapLastAllocationSize_.find(currentCid);


	if ( mapIterator == mapLastAllocationSize_.end()) {

		// CID nicht vorhanden
		// mrtrsize berechnen  = 64Kbps VOIP -> 70Kbps
		mrtrSize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

		// mstrsize berechnen  = 25 Mbps MPEG4 AVC/H.264 -> 20 Mbps
		mstrSize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );


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
		u_int32_t com_mstrSize = ((timeBase * sfQosSet->getMaxSustainedTrafficRate() - ( timeBase  - (NOW - timeStamp)) * lastMstr) / 8);

//S_max <Byte> = T_b <second> * MSTR <Bit/second> - (T_b <second> - (current_Time - last_Time) <second> * last_MSTR <Bit/second> )
		u_int32_t com_mrtrSize = ((timeBase * sfQosSet->getMinReservedTrafficRate() - (timeBase - (NOW - timeStamp )) * lastMrtr) / 8);

// ----------- gets maximal traffic burst in oder to compare with the computed DataSize----------------//
		u_int32_t maxTrafficBurst = sfQosSet->getmaxTrafficBurst();


/*------------nimm minimalen Wert der Datengröße----------------------------------------------------------------------//
//------------die angekommende Datengröße an MAC SAP Trasmitter,die gerechneten Datengröße für mstr und die maximale Rahmengröße--------*/
		com_mstrSize = MIN( com_mstrSize, maxTrafficBurst);
		mstrSize = MIN( com_mstrSize, u_int32_t(con->queueByteLength()) );

/*------------nimm minimalen Wert der Datengröße-----------------------------------------------------------------------//
//------------die angekommende Datengröße an MAC SAP Trasmitter ,die gerechneten Datengröße für mrtr und die maximale Rahmengröße--------*/
		com_mrtrSize = MIN(com_mrtrSize,maxTrafficBurst);
		mrtrSize = MIN(com_mrtrSize, u_int32_t(con->queueByteLength()) );

	}
}

//---------------------------------------------Connection aktualisieren----------------------------------------------------//
// Parameter S_k [Byte]
void tswTCM::updateAllocation(Connection *con,u_int32_t mstrSize,u_int32_t mrtrSize)
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

	if ( mapIterator == mapLastAllocationSize_.end())
	{
         // Cid nicht vorhanden
		//--neues Map element erstellen-----bzw. neue Strukture erstellen, um aktuellen DatenSize und aktuellen Zeitpunkt ab zu legen----------//
		LastAllocationSize lastAllocationSize;

		// Berechnet Ak
		u_int32_t currentMrtr = ( mrtrSize * 8 / frameDuration_ );
		u_int32_t currentMstr = ( mstrSize * 8 / frameDuration_ );

		//----------Zuweisung auf Strukturelemente------------//
		lastAllocationSize.lastMrtr = currentMrtr;
		lastAllocationSize.lastMstr = currentMstr;
		lastAllocationSize.timeStamp = NOW;

		//------------neues Map element lastAllocationSize in MAP mapLastAllocationSize_ hinzufügen----------------------//
		mapLastAllocationSize_.insert( pair<int,LastAllocationSize>( currentCid, lastAllocationSize) );

        //---Ab jetzt können wir Cid und die Strukture lastAllocationSize(mrtrSize,mstrSize und timeStamp) nutzen-aus MAP--//


	} else	{
		// CiD vorhanden
		//hole werte und verwende die Werte
		u_int32_t lastMrtr = mapIterator->second.lastMrtr;
		u_int32_t lastMstr = mapIterator->second.lastMstr;

		// berechne Zeitdifferent zum letzten Aufruf / T_D
		double timeDelta = NOW - mapIterator->second.timeStamp;


		/*--------------------------die aktuelle MRTR für die Info-Datenrate---------------------------------------------------------//
		 * A_k <Bit/second> = ( T_b <second>_double * last_MRTR <Bit/s>_u_int32_t + last_DataSize <Bit>_u_int32_t ) / ( T_b <second>_double + nT_f <second > )
		 */
		u_int32_t currentMrtr = ceil(( lastMrtr * timeBase + mrtrSize * 8.0 ) / ( timeBase + timeDelta ));
/*--------------------------die aktuelle MSTR für die Info-Datenrate---------------------------------------------------------//
 * A_k <Bit/second> = ( T_b <second>_double * last_MSTR <Bit/s>_u_int32_t + last_DataSize <Bit>_u_int32_t ) / ( T_b <second> + T_f <second > )
 */
		u_int32_t currentMstr = floor(( lastMstr * timeBase + mstrSize * 8.0 ) / ( timeBase + timeDelta));


		//  Aktuellisierung der Werte  mstrSize mrtrSize in map
		mapIterator->second.lastMrtr = currentMrtr;
		mapIterator->second.lastMstr = currentMstr;
		mapIterator->second.timeStamp = NOW;
	}
}



//---------------------------------Clear the Connection ---------------------------------------------------------------//
void tswTCM::clearConnection(Connection *con)
{
	LastAllocationSizeIt_t mapIterator;
	int currentCid = con->get_cid();

	//ist die CID in Map ?
	// CID aus dem MAP löschen

	mapIterator = mapLastAllocationSize_.find(currentCid);

		if ( mapIterator == mapLastAllocationSize_.end())
		{

			cout << " the connection doesn't exist   " << endl;

		} else
		{
//-------------------- the connection exists in MAP ,so we erase the connection-------------------------------------------//
			mapLastAllocationSize_.erase( currentCid );
		}
}
/* Beispiel :
 * FRAMEDURATION 0.005,
 *  time base = 25 ms,
 * 1 pakcet 100Byte kommt aller 5 ms
 * darauf kommt es:
 * mrtr = 800 bit 10+e3/5 sekunden = 160 Kbps (bit/sec).
 * Cid = 1.
 *
 * (VoIP Service Klasse ) maxTrafficBurst = 153 000 bit --------nehme an dass(16 QAM 1/2 Modulation-Verfahren)   (1 slot  trägt 96 bit ) (8 slots tragen 768 bit / 5ms)
 *
 * 1.Iteration
 * 1.getDataSize(Connection *con,u_int32_t &mstrSize,u_int32_t &mrtrSize ) wurde aufgerufen.
 * Kein Verbindung existiert
 * 1.mrtrSize = 160 Kbps 5-e3 / 8 = 100 Byte ;
 * 1.updateAllocation(Connection *con,u_int32_t mstrSize ,u_int32_t mrtrSize = 100 Byte)
 * u_int32_t currentMrtr = ( mrtrSize * 8 / frameDuration_ )  = 800 / 5-e3 = 160 kbps
 * lastMrtr = 160 kbps
 * 1.timeStamp = 0
 *
 *
 *
 * ----------------------------------------2.Iteration--------------------
 * die Verbindung ist vorhanden
 * 2.getDataSize(Connection *con,u_int32_t &mstrSize,u_int32_t &mrtrSize )
 *
 *                (                          (25e-3 - 5e-3 + 0) * 160e+3) / 8
 * com_mrtrSize = ((25e-3 * 160e+3    -   (25e-3 - (NOW - timeStamp )) * 160 kbps) / 8);
 *              =   (4000              -    3200) / 8
 *              =   100 Byte
 * ------------------------------------------------------------------------------
 * com_mrtrSize = MIN(com_mrtrSize,maxTrafficBurst);
 * mrtrSize = MIN(com_mrtrSize, u_int32_t(con->queueByteLength()) );
 *          = 100 Byte            (Ahnahme dass queueByteLength = 100 Byte )
 * ---------------------------------------------------------------------------------
 *2.updateAllocation(Connection *con,u_int32_t mstrSize ,u_int32_t mrtrSize = 100 Byte)
 *  2.lastMrtr = 1.lastMrtr = 160 kbps
 * timeDelta = NOW - mapIterator->second.timeStamp
 *
 *                                       Annahme timeDelta = 5e-3
 * currentMrtr = ( 160kbps * 25e-3 + 100 * 8.0 ) / ( 25e-3 + timeDelta ));
 *             = (  160 * 25 + 800 ) * 1e+3/ 30
 *             =  160 kbit/s
 * lastMrtr = currentMrtr
 *          = 160 kbit/second;
 * timeStamp = 5e-3
 *

 * ----------------------------------------3.Iteration--------------------
 * die Verbindung ist vorhanden
 * 3.getDataSize(Connection *con,u_int32_t &mstrSize,u_int32_t &mrtrSize )
 *
 *                (                          (25e-3 - 10e-3 + 5e-3) * 160e+3) / 8
 * com_mrtrSize = ((25e-3 * 160e+3    -   (25e-3 - (NOW - timeStamp )) * 160 kbps) / 8);
 *              =   (4000              -    3200) / 8
 *              =   100 Byte
 * ------------------------------------------------------------------------------
 * com_mrtrSize = MIN(com_mrtrSize,maxTrafficBurst);
 * mrtrSize = MIN(com_mrtrSize, u_int32_t(con->queueByteLength()) );
 *          = 100 Byte            (Ahnahme dass queueByteLength = 100 Byte )
 * ---------------------------------------------------------------------------------
 *3.updateAllocation(Connection *con,u_int32_t mstrSize ,u_int32_t mrtrSize = 100 Byte)
 *  3.lastMrtr = 1.lastMrtr = 160 kbps
 * timeDelta = NOW - mapIterator->second.timeStamp
 *           = 10e-3 - 5e-3
 *           = 5e-3
 *                                       Annahme timeDelta = 5e-3
 * currentMrtr = ( 160kbps * 25e-3 + 100 * 8.0 ) / ( 25e-3 + timeDelta ));
 *             = (  160 * 25 + 800 ) * 1e+3/ 30
 *             =  160 kbit/s
 * lastMrtr = currentMrtr
 *          = 160 kbit/second;
 * timeStamp = 10e-3
 *
 *
 *
 * */
