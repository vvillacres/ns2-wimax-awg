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



//--------------------------Interne Methoden-------------------------------
void tswTCM::interneMethode ()
{
	// ----------------- MAP Element muss aktualliziert werden--------------------
	//---------------neues MAP Element erstellen--------------------//

lastAllocationSize.mrtrSize = mrtr;

lastAllocationSize.mstrSize = mstr;

lastAllocationSize.timeStamp = lastTime;

}



/*struct LastAllocationSize{
				u_int32_t mrtrSize;
				u_int32_t mstrSize;
				double timeStamp;
};*/



//--------------------------------getting Data Size ------------------------------
void tswTCM::getDataSize(Connection *con,u_int32_t mstrsize,u_int32_t mrtrsize )
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
		// mrtrsize berechnen  = 64Kbps VOIP-> 70Kbps
		mrtrsize = u_int32_t( ceil( double(sfQosSet->getMinReservedTrafficRate()) * frameDuration_ / 8.0 ) );

		// mstrsize berechnen  = 344 Kbps MPEG -> 300 Kbps
		mstrsize = u_int32_t( floor( double(sfQosSet->getMaxSustainedTrafficRate()) * frameDuration_/ 8.0 ) );

		// neues Map element erstellen
		LastAllocationSize lastAllocationSize;

		// Zuweisung an Strukturelemente
		lastAllocationSize.mrtrSize = mrtrsize;
		lastAllocationSize.mstrSize = mstrsize;
		lastAllocationSize.timeStamp = NOW;

		// neues Map element in MAP hinzufügen
		mapLastAllocationSize_.insert( pair<int,LastAllocationSize>( currentCid, lastAllocationSize) );

	} else
	{
		//------------------------ QoS Parameter auslesen---------------------------
					ServiceFlowQosSet sfQosSet = con->getServiceFlow()->getQosSet();
		// CID vorhanden
		lastAllocationSize = mapIterator->second;

		// Structurelemente aus Map element  holen für Berechnung des Prediction Size
		u_int32_t lastmrtr = lastAllocationSize.mrtrSize;
		u_int32_t lastmstr = lastAllocationSize.mstrSize;
		double lastTime = lastAllocationSize.timeStamp;

		//-------------------------gültige Datasize ausholen-----------------------------------------//
		u_int32_t timeBase_ = sfQosSet->getTimeBase();

		u_int32_t com_mstrSize = (timeBase_* sfQosSet->getMaxSustainedTrafficRate() - (timeBase_-frameDuration_)*mstr);

		u_int32_t com_mrtrSize = (timeBase_* sfQosSet->getMinReservedTrafficRate() - (timeBase_-frameDuration_)*mrtr);

		u_int32_t maxTrafficBurst_ = sfQosSet->getmaxTrafficBurst();

	//---------nimm minimalen Wert der Datengröße------------------------------
		u_int32_t mrtrsize = MIN(lastmrtr,mrtrsize);
		          mrtrsize = MIN(mrtrsize,com_mrtrSize);

     //---------nimm minimalen Wert der Datengröße------------------------------
		u_int32_t mstrsize = MIN(lastmstr,mstrsize);
			      mstrsize = MIN(mstrsize,com_mstrSize);
	}

}

//-------------------------------Connection aktualisieren-----------------------------------------------
void tswTCM::UpdateAllocation(Connection *con,u_int32_t mstrsize,u_int32_t mrtsize)
{
	LastAllocationSizeIt_t mapIterator;
	int currentCid = con->get_cid();

	// QoS Parameter auslesen
	ServiceFlowQosSet sfQosSet = con->getServiceFlow()->getQosSet();

	//  get Windows Size from QoS Parameter Set
	u_int16_t windowSize =  con->get_serviceflow()->getQoS()->getTimeBase();



	//------------------------ die Position der Abbildung in der MAP - Eintrag suchen--------------------
	mapIterator = mapLastAllocationSize_.find(currentCid);

	//----------------------gets the Structure elements from Map element CID of the MAP-----------------------------//
	lastAllocationSize = mapIterator->second;
	u_int32_t lastmrtr = lastAllocationSize.mrtrSize;
	u_int32_t lastmstr = lastAllocationSize.mstrSize;
	double lastTime = lastAllocationSize.timeStamp;


	//--------------------------die aktuelle MinReser Info-Datenrate---------------------------------------------------------//
	u_int32_t mrtr = u_int32_t( ceil ((mrtrsize* windowSize + lastmrtr)  / u_int32_t(windowSize + frameDuration_)));


	//--------------------------die aktuelle MaxSus Info-Datenrate---------------------------------------------------------//
	u_int32_t mstr = u_int32_t( floor ((mstrsize* u_int32_t(windowSize) + lastmstr)  / u_int32_t(windowSize + frameDuration_)));

	//-----------------Update the Structure elements in the MAP-----------------
	interneMethode();


}

//--------------------Clear the Connection -------------------------------------------------------------
void tswTCM::ClearConnection(Connection *con)
{
	LastAllocationSizeIt_t mapIterator;
	int currentCid = con->get_cid();

	// QoS Parameter auslesen
	ServiceFlowQosSet sfQosSet = con->getServiceFlow()->getQosSet();

	//ist die CID in Map ?
	// CID aus dem MAP löschen
	mapIterator = mapLastAllocationSize_.find(currentCid);
		if ( mapIterator == mapLastAllocationSize_.end()) {
			cout << "  Don't exist the Connection  ";
		}else
		{
			// Connection exists in MAP so erases Connection
			mapLastAllocationSize_.erase( currentCid );
		}
}

