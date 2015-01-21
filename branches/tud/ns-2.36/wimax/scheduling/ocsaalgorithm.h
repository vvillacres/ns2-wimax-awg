/*
 * ocsaalgorithm.h
 *
 *  Created on: 26.04.2011
 *      Author: richter
 */

#ifndef OCSAALGORITHM_H_
#define OCSAALGORITHM_H_

#include "anfrage.h"
#include "frame.h"
#include <vector>


class OcsaAlgorithm {
public:
	OcsaAlgorithm();
	virtual ~OcsaAlgorithm();

	struct sortAnfrage
	{
		bool operator()(anfrage * first,anfrage * second)
		{
			return (first->get_slot() > second->get_slot());
		}

	}sortAnfrageObject;

	struct sortFreearea
		{
			bool operator()(freearea * first,freearea * second)
			{
				return (first->get_area() < second->get_area());
			}

		}sortFreeareaObject;

	void sortAnfrageList(vector<anfrage *>* anfrageList);
	void sortFreeareaList(vector<freearea *>* freeareaList);
	void deleteFromAnfrageList(vector<anfrage *>* anfrageList, int i);
	void calculateOverhead(frame * frameObject);

    OscaBurst * build_a_burst(vector<anfrage *>* anfrageList,int max_burst_slot,int freeArea);

    int packing(vector <anfrage *>* anfrageList, frame * frameObject);
	int initialFirstBurst(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot);
	anfrage * findSmallstAnfrage(vector<anfrage *>* anfrageList);
	int packOneBurst(frame * frameObject, OscaBurst * burstObject);
	void finishToPackingInAColumn(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot);
	void getFreearea(frame * frameObject);
	void finishToPackingFrame(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot);
	int PackingOneBrustInOverheadColumn(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot,int tmp_height);
	int finishTOPackingBrustInOverheadColumn(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot);
	bool haveOneAnfragePackingIntoFrame(vector<anfrage *>* anfrageList,frame * frameObject);
	int check_include(frame * frameObject);
	bool checkCollision_first (vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot);
	bool checkCollision_after (vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot);



};

#endif /* OCSAALGORITHM_H_ */
