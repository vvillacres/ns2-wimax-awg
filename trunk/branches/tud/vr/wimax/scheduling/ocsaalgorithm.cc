/*
 * ocsaalgorithm.cc
 *
 *  Created on: 26.04.2011
 *      Author: richter
 */

#include "ocsaalgorithm.h"
#include <iostream>
#include <algorithm>
#include <math.h>

OcsaAlgorithm::OcsaAlgorithm() {
	// TODO Auto-generated constructor stub

}

OcsaAlgorithm::~OcsaAlgorithm() {
	// TODO Auto-generated destructor stub
}

void OcsaAlgorithm::sortAnfrageList(vector<anfrage *>* anfrageList)
{
	vector<anfrage *>::iterator it_AL;
    sort (anfrageList->begin(), anfrageList->end(), sortAnfrageObject);
 //   cout<<"-----------------------after sort AnfrageList : --------------------"<<endl;
 /*   for (it_AL = anfrageList->begin(); it_AL < anfrageList->end(); it_AL++)
    {
    	(* it_AL)->displayAnfrage();
    }*/
}

void OcsaAlgorithm::sortFreeareaList(vector<freearea *>* freeareaList)
{
	vector<freearea *>::iterator it_FL;
    sort (freeareaList->begin(), freeareaList->end(), sortFreeareaObject);

}

void OcsaAlgorithm::deleteFromAnfrageList(vector<anfrage *>* anfrageList,int i)
{
	//cout<<"deleted Anfrage object :  ";
	//cout<<"slot: "<<AnfrageList->at(i)->get_slot()<<"  cid: "<<AnfrageList->at(i)->get_cid()<<endl;
	 //delete anfrageList->at(i);
	anfrageList->erase (anfrageList->begin() + i);
}

//------------------------------------------------------------------------------
//put the Anfrage to burst,if anfrage smaller than max_burst_slot,a burst hat more than one Anfrage.
//and a burst must smaller als remain free area.
//-------------------------------------------------------------------------------
OscaBurst* OcsaAlgorithm::build_a_burst(vector<anfrage *>* anfrageList,int max_burst_slot,int freeArea){
	//cout<<"free area = "<<freeArea<<endl;
	OscaBurst * burstObject;
	burstObject = new OscaBurst();
//	cout<<"build a burst  "<<endl;
	for (unsigned int k = 0;k != anfrageList->size();k++)
	{
		if(anfrageList->at(k)->get_slot() == freeArea)
		{
			burstObject->addToAnfrageList(anfrageList->at(k));
			deleteFromAnfrageList(anfrageList,k);
		//	cout<<"build a burst: first return"<<endl;
			return burstObject;
		}
	}

	for (unsigned int i = 0;i != anfrageList->size();i++)
   {
    if(anfrageList->at(i)->get_slot() < freeArea)
    {
//    	cout<<"enter next if"<< endl;
		if(anfrageList->at(i)->get_slot() < max_burst_slot)
		{
			burstObject->addToAnfrageList(anfrageList->at(i));

			deleteFromAnfrageList(anfrageList,i);
//			 cout<<"size=  "<< anfrageList->size()<<endl;
		  for (unsigned int j = i;j != anfrageList->size();j++){
/*			  cout<<"J ="<<j<< endl;
			  cout<<"s1 = "<<anfrageList->at(j)->get_slot()<< endl;
			  cout<<"s2 = "<<max_burst_slot -  burstObject->get_slot()<< endl;
			  cout<<"s3 = "<<freeArea - burstObject->get_slot()<< endl;
			  cout<<"DIUC1 = "<<anfrageList->at(j)->get_diuc()<< endl;
			  cout<<"DIUC2 = "<<burstObject->get_anfrageList().back()->get_diuc()<< endl;*/
			  if ( (anfrageList->at(j)->get_diuc()==burstObject->get_anfrageList().back()->get_diuc())
					  &&(anfrageList->at(j)->get_slot() <= max_burst_slot -  burstObject->get_slot())
					  &&(anfrageList->at(j)->get_slot() <= freeArea - burstObject->get_slot()))
			  {
				  burstObject->addToAnfrageList(anfrageList->at(j));

				  deleteFromAnfrageList(anfrageList,j);
				  j--;
				//  cout<<"size=  "<< anfrageList->size()<<endl;
			  }
		  }
		}
		else //anfrage bigger than or equal maximal burst area,an anfrage = a burst
		{
			burstObject->addToAnfrageList(anfrageList->at(i));
			deleteFromAnfrageList(anfrageList,i);

		}
		//cout<<"build a burst: second return"<<endl;
		return burstObject;
    }
  }

}

void OcsaAlgorithm::getFreearea(frame * frameObject)
{
	unsigned int freeareaList_size = frameObject->freeareaList.size();
	for (unsigned int i = 0; i < freeareaList_size; i++)
	{
		if(frameObject->get_burstList().back()->get_yCorr()- frameObject->get_burstList().back()->get_height()< frameObject->freeareaList.at(i)->get_y()
		   && frameObject->get_burstList().back()->get_yCorr() >= frameObject->freeareaList.at(i)->get_y())
		{
			if(frameObject->get_burstList().back()->get_xCorr()- frameObject->get_burstList().back()->get_width()>= frameObject->freeareaList.at(i)->get_x()-frameObject->freeareaList.at(i)->get_width()
			   && frameObject->get_burstList().back()->get_xCorr() <= frameObject->freeareaList.at(i)->get_x())
		   {
	           // create freearea in top side
		       if (frameObject->get_burstList().back()->get_height() < frameObject->freeareaList.at(i)->get_height())
		       {
			   frameObject->addToFreeareaList(
						frameObject->get_burstList().back()->get_xCorr(),
						frameObject->get_burstList().back()->get_yCorr() - frameObject->get_burstList().back()->get_height(),
						frameObject->freeareaList.at(i)->get_width(),
						frameObject->freeareaList.at(i)->get_height() - frameObject->get_burstList().back()->get_height()
						);
		       }
	          // create freearea in left side
	         if (frameObject->get_burstList().back()->get_width() < frameObject->freeareaList.at(i)->get_width())
	          {
		      frameObject->addToFreeareaList(
					frameObject->get_burstList().back()->get_xCorr() - frameObject->get_burstList().back()->get_width(),
					frameObject->get_burstList().back()->get_yCorr(),
					frameObject->freeareaList.at(i)->get_width() - frameObject->get_burstList().back()->get_width(),
					frameObject->get_burstList().back()->get_height()
					);
	          }
	         delete frameObject->freeareaList.at(i);
	         frameObject->freeareaList.erase(frameObject->freeareaList.begin() + i );
	         --i;
	         --freeareaList_size;
		  }
	   }
   }
}

void OcsaAlgorithm::calculateOverhead(frame * frameObject)
{

       float tmp =44 + 16 * frameObject->get_burstList().back()->get_anfrageList().size();
       frameObject->set_overheadarea(frameObject->get_overheadarea() + tmp);//bits.......( QPSK R=1/2 ,1 Slot = 48 bit )

       int i = frameObject->get_overheadX();

       frameObject->set_overheadX(i);
       float tmp1 = frameObject->get_overheadarea()/48;
  //     cout<<"y"<<frameObject->get_overheadarea()/48<<"  "<<tmp1<<"  "<<ceil(tmp1)-frameObject->get_heightOfFrame()*(i-1)<<endl;
       frameObject->set_overheadY(ceil(tmp1)-frameObject->get_heightOfFrame()*(i-1));

       if (i<14 && frameObject->get_overheadarea() > frameObject->get_heightOfFrame()*i*48 )
       {
    	   i+=1;
    	   frameObject->set_overheadX(i);
    	   float tmp2 = frameObject->get_overheadarea()/48-frameObject->get_heightOfFrame()*(i-1);
    	   frameObject->set_overheadY(ceil(tmp2));

       }
}


anfrage * OcsaAlgorithm::findSmallstAnfrage(vector<anfrage *>* anfrageList)
{
	if(!anfrageList->empty()){
    float slots = anfrageList->at(0)->get_slot();
    int index = 0;
	for (unsigned int i = 1;i != anfrageList->size();i++)
	{
		if (slots > anfrageList->at(i)->get_slot())
		{
			slots = anfrageList->at(i)->get_slot();
			index = i;
		}
	}
	return anfrageList->at( index);
	} else {
		return NULL;
	}
}



int OcsaAlgorithm::initialFirstBurst(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot)
{
//	cout<<" 1=  ";
	int tmp = frameObject->get_overheadX();
	if(checkCollision_first (anfrageList,frameObject,max_burst_slot))
	{
		frameObject->set_restwidth(frameObject->get_restwidth() - 1);
	}
//	cout<<" 2=  ";
//	cout<<"restwidth after collision = "<<frameObject->get_restwidth()<<endl;

	if ( frameObject->get_restwidth() > 0 ) {
		anfrage * smallestAnfrage = findSmallstAnfrage(anfrageList);
		if 	( (smallestAnfrage != NULL ) && (( frameObject->get_restwidth() * frameObject->get_heightOfFrame() ) < smallestAnfrage->get_slot() )) {
			smallestAnfrage->set_slot( frameObject->get_restwidth() * frameObject->get_heightOfFrame());
		}
		OscaBurst * burstObject = NULL;
		burstObject = build_a_burst(anfrageList,max_burst_slot,frameObject->get_heightOfFrame()*frameObject->get_restwidth());


		for (int j = 1; j <= frameObject->get_heightOfFrame(); j++)
		{
		 for (int i = 1; i <= frameObject->get_heightOfFrame(); i++)//i = width
		 {
			if (burstObject->get_slot() % i == 0)
			{

			   if (burstObject->get_slot() / i <= frameObject->get_heightOfFrame()&& i <= frameObject->get_restwidth())
				{
			//	   if(frameObject->get_collision())
			//	   {
			//	       burstObject->set_x(frameObject->get_overheadX() + frameObject->get_restwidth()+1);
			//	   }
			//	   else
			//	   {
					   burstObject->set_x(frameObject->get_overheadX() + frameObject->get_restwidth() );
			//	   }
				   burstObject->set_y(frameObject->get_heightOfFrame());
				   burstObject->set_width(i);
				   burstObject->set_height( burstObject->get_slot() / i);
				   frameObject->addToBurstList(burstObject);

				   if (frameObject->get_burstList().back()->get_yCorr() - frameObject->get_burstList().back()->get_height() > 0)
						{
							frameObject->addToFreeareaList(
										frameObject->get_burstList().back()->get_xCorr(),
										frameObject->get_burstList().back()->get_yCorr() - frameObject->get_burstList().back()->get_height(),
										i,
										frameObject->get_burstList().back()->get_yCorr() - frameObject->get_burstList().back()->get_height()
										);
						}
				   check_include(frameObject);

				   // vr@tud overhead calculation through scheduler
				   // calculateOverhead(frameObject);

				   if (frameObject->get_overheadX() > tmp )
				   {
					   frameObject->set_restwidth(frameObject->get_restwidth()-1);
				   }
				   frameObject->set_restwidth(frameObject->get_restwidth()-i);

	   //     	   cout<<"xxxxxxxxxxxx rest width xxxxxxxxx"<<frameObject->get_restwidth()<<endl;
	////			   frameObject->displaynewburst(burstObject);
			/*	   for (unsigned int s = 0; s < anfrageList->size();s++)
										{
											cout<<"anfrage position= "<<s<<endl;
											anfrageList->at(s)->displayAnfrage();

										}*/
				   burstObject = NULL;
				   return 0;

				 }
			}
		 }
		burstObject->set_slot(burstObject->get_slot() + 1);  //if it have not suitable pair,plus 1 slot
		}

	}
	return 0;
}

int OcsaAlgorithm::packOneBurst(frame * frameObject,OscaBurst * burstObject)
{

	    int tmp = frameObject->get_overheadX();
	    burstObject->set_x(frameObject->freeareaList.back()->get_x());

	    burstObject->set_y(frameObject->freeareaList.back()->get_y());
	//    cout<<"y "<<frameObject->freeareaList.back()->get_y()<<endl;
	    for (int m = 0; m <= frameObject->freeareaList.back()->get_width()-1; m++)
	    {
	//    	cout<<"m= "<<m<<endl;
	         for (int n = frameObject->freeareaList.back()->get_width(); n != 0; n--)
	         {
	  //      	 cout<<"n= "<<n<<endl;
	              if (burstObject->get_slot() % n == 0)
	                {
	            	  if (burstObject->get_slot() / n <= frameObject->freeareaList.back()->get_height())
	            	  {
	           // 		  cout<<"packing............ "<<endl;
	                	 burstObject->set_width(n);
	                	 burstObject->set_height(burstObject->get_slot() / n);
	                	 frameObject->addToBurstList(burstObject);
	                	 frameObject->set_waste_area(m);
	                	 getFreearea(frameObject);
	                	 check_include(frameObject);

	                	 // vr@tud overhead calculation through scheduler
	                	 // calculateOverhead(frameObject);


	                	 if (frameObject->get_overheadX() > tmp )
	                	 {
	            	          frameObject->set_restwidth(frameObject->get_restwidth()-1);
	                     }
	  ///////              	 frameObject->displaynewburst(burstObject);
	                	 burstObject = NULL;

	                	 return 0;
	            	  }
	                }
	         } burstObject->set_slot(burstObject->get_slot() + 1);
	     }

return 0;
}

void OcsaAlgorithm::finishToPackingInAColumn(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot)
{

	for (int i=0;i< frameObject->get_heightOfFrame();i++)
		{
		if(frameObject->get_overheadX()<frameObject->get_burstList().back()->get_xCorr()&&!anfrageList->empty())
		    {
		    sortFreeareaList(& frameObject->freeareaList);
			int smallstAnfrageArea = findSmallstAnfrage(anfrageList)->get_slot();
     //          cout<<"collision?   "<<checkCollision_after (anfrageList,frameObject,max_burst_slot)<<endl;
			  if(frameObject->freeareaList.size() !=0 && frameObject->freeareaList.back()->get_area()>= smallstAnfrageArea){
				 if(!checkCollision_after (anfrageList,frameObject,max_burst_slot))
			  {
			         OscaBurst * burstObject;
			         burstObject = build_a_burst(anfrageList,max_burst_slot,frameObject->freeareaList.back()->get_area());
			         packOneBurst(frameObject,burstObject);
			  }
				 else
				 {
					 int tmp_overhead = frameObject->get_estimate_overhead();
					 int tmp_overhead_y = tmp_overhead%frameObject->get_heightOfFrame();
					 OscaBurst * burstObject;
					 burstObject = build_a_burst(anfrageList,max_burst_slot,
					 frameObject->freeareaList.back()->get_area()-tmp_overhead_y*frameObject->freeareaList.back()->get_width());
					 packOneBurst(frameObject,burstObject);
				 }
		    }
		    }
    }


	if(frameObject->freeareaList.size() !=0){
	//	 cout<<"xx delete all rest freearealist xx "<<endl;
	 for (unsigned int k=0;k< frameObject->freeareaList.size();k++)
	 {
	     delete frameObject->freeareaList.at(k);
	 }
	 frameObject->freeareaList.clear();
	}
	/* frameObject->addToFreeareaList(frameObject->get_overheadX() + frameObject->get_restwidth(),
			                        frameObject->get_heightOfFrame(),
			                        frameObject->get_restwidth(),
			                        frameObject->get_heightOfFrame());*/
}



int OcsaAlgorithm::finishTOPackingBrustInOverheadColumn(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot)
{

	if ( ! anfrageList->empty()) {
	   int tmp_height = 0;
	   if(frameObject->get_overheadX()==frameObject->get_burstList().back()->get_xCorr())
	   {
		   tmp_height=frameObject->get_heightOfFrame() - frameObject->get_burstList().back()->get_yCorr()+frameObject->get_burstList().back()->get_height();
	   }
	   int tmp = ceil((frameObject->get_overheadarea() + 60)/48);
	   int tmp_overhead = tmp%(frameObject->get_heightOfFrame());
	  // cout<<"tmp_overhead1  = "<<tmp_overhead<<endl;
	   float tmp_small = findSmallstAnfrage(anfrageList)->get_slot();

	   while(tmp_height + tmp_overhead + tmp_small <= frameObject->get_heightOfFrame())
	   {

		   int tmp = ceil((frameObject->get_overheadarea() + 60)/48);
		 //  int tmp_overhead = tmp%(frameObject->get_heightOfFrame());
		//   cout<<"tmp_height  = "<<tmp_height<<endl;
		//   cout<<"tmp_overhead2  = "<<tmp_overhead<<endl;
		//   cout<<"tmp_small  = "<<tmp_small<<endl;
		   PackingOneBrustInOverheadColumn(anfrageList,frameObject, max_burst_slot,tmp_height) ;

		   tmp_height += frameObject->get_burstList().back()->get_height();
	   }
	}
	return 0;
}

int OcsaAlgorithm::PackingOneBrustInOverheadColumn(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot,int tmp_height)
{
	int tmp = ceil((frameObject->get_overheadarea() + 60)/48);
	int tmp_overhead = tmp%(frameObject->get_heightOfFrame());
	OscaBurst * burstObject;
	if(!anfrageList->empty() && frameObject->get_restwidth()>=0)
	{
	for (unsigned int i = 1;i != anfrageList->size();i++)
		{
			if(anfrageList->at(i)->get_slot() + tmp_overhead + tmp_height <=frameObject->get_heightOfFrame())
			{
				burstObject = new OscaBurst;
				burstObject->addToAnfrageList(anfrageList->at(i));
				deleteFromAnfrageList(anfrageList,i);

				burstObject->set_x(frameObject->get_overheadX());
			    burstObject->set_y(frameObject->get_heightOfFrame() - tmp_height);
				burstObject->set_width(1);
				burstObject->set_height(burstObject->get_slot());

		        frameObject->addToBurstList(burstObject);
		        frameObject->set_overheadarea(frameObject->get_overheadarea() + 60 );// QPSK R=1/2 ,1 Slot = 48 bit
		        //overheadX is not changed
		        int tmp1 = ceil(frameObject->get_overheadarea()/48);
		        int tmp2 = tmp1%frameObject->get_heightOfFrame();
		        frameObject->set_overheadY(tmp2);

   	/////	    frameObject->displaynewburst(burstObject);
				burstObject = NULL;
			    return 0;
			}
		}
	}
	return 0;
}

void OcsaAlgorithm::finishToPackingFrame(vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot)
{
   	do  {
   	//	int tmp = frameObject->get_overheadX();
   	//	cout<<"-----------------------begin initialFirstBurst ------------------------------------"<<endl;
   	//	bool tmp1 = checkCollision_first(anfrageList,frameObject,max_burst_slot);
   		initialFirstBurst( anfrageList,frameObject, max_burst_slot);
   	//	cout<<"-----------------------finish initialFirstBurst ------------------------------------"<<endl;
   	//	cout<<"-----------------------begin finishToPackingInAColumn ------------------------------------"<<endl;
   		finishToPackingInAColumn(anfrageList,frameObject,max_burst_slot);

   /*	    if(frameObject->get_overheadX()>tmp && !tmp1 )
   	    {
   			frameObject->set_restwidth(frameObject->get_restwidth()-1);
        }*/
   	//    cout<<"-------------------------finish finishToPackingInAColumn ---------------------------------"<<endl;
   	//    cout<<"restwidth  "<<frameObject->get_restwidth()<<endl;
   	//   cout<<"aktuelle anfrage size =   "<< anfrageList->size()<<endl;
   	} while(frameObject->get_restwidth() > 0 && haveOneAnfragePackingIntoFrame(anfrageList,frameObject));
   	//	cout<<"---------------------begin PackingBrustInOverheadColumn ----------------------------------------"<<endl;
   	finishTOPackingBrustInOverheadColumn(anfrageList,frameObject,max_burst_slot);
   	// 	cout<<"--------------------finish PackingBrustInOverheadColumn ----------------------------------------"<<endl;

}

bool OcsaAlgorithm::haveOneAnfragePackingIntoFrame(vector<anfrage *>* anfrageList,frame * frameObject)
{
	for (unsigned int i = 0;i != anfrageList->size();i++)
	  {
	    if(anfrageList->at(i)->get_slot() < frameObject->get_restwidth()*frameObject->get_heightOfFrame())
	    {
	    	return true;
	    }
	  }
	return false;
}

int OcsaAlgorithm::check_include(frame * frameObject)
{
	unsigned int size = 0,size_temp = 0;
	size_temp = frameObject->freeareaList.size();

	do
	{
		size = size_temp;

		for (unsigned int i = 0; i < size_temp; i++ )
		{
			for (unsigned int j = 0; j < size_temp; j++)
			{
				if (i != j)
				{
					if ( frameObject->freeareaList.at(i)->get_x()- frameObject->freeareaList.at(i)->get_width()<= frameObject->freeareaList.at(j)->get_x()-frameObject->freeareaList.at(j)->get_width()
							&& frameObject->freeareaList.at(i)->get_x() >= frameObject->freeareaList.at(j)->get_x())
					{
						if (frameObject->freeareaList.at(i)->get_y() >= frameObject->freeareaList.at(j)->get_y()
							&& frameObject->freeareaList.at(i)->get_y()- frameObject->freeareaList.at(i)->get_height()<= frameObject->freeareaList.at(j)->get_y()-frameObject->freeareaList.at(j)->get_height())
						{
							frameObject->deleteFromFreeareaList(& frameObject->freeareaList, j);
							size_temp = size - 1;
							break;
						}
					}
				}
			}
			if (size != size_temp)
			{
				break;
			}
		}
	}while(size != size_temp);
	return 0;
}


bool OcsaAlgorithm::checkCollision_first (vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot)
{
//	cout<<" 3=  ";
    int tmp =0; // slot of next burst
    int N_anfrage = 0;


    if(!anfrageList->empty()) {

    	vector<anfrage *>::iterator anfrageListIt;
    	int freeArea = frameObject->get_heightOfFrame()*(frameObject->get_restwidth());
   // 	cout<<"freeArea= "<<freeArea<<endl;

        int j=0;
    	anfrageListIt = anfrageList->begin();
    	do {
    		if((*anfrageListIt)->get_slot()<= freeArea){
    			tmp += (*anfrageListIt)->get_slot();
    			N_anfrage++;
    			j = (*anfrageListIt)->get_diuc();
    		}

    		++anfrageListIt;
    //		cout<<"tmp1= "<<tmp<<endl;

    	} while ( (anfrageListIt != anfrageList->end()) && (tmp < max_burst_slot)
    			&& (tmp <= freeArea )&& N_anfrage ==0);

   // 	cout<<"diuc= "<<j<<endl;

    	while ( (anfrageListIt != anfrageList->end()) && (tmp < max_burst_slot)&& (tmp <= freeArea )) {
           if ( (*anfrageListIt)->get_diuc() == j ) {
             if ( tmp + (*anfrageListIt)->get_slot() < max_burst_slot && tmp + (*anfrageListIt)->get_slot()<=freeArea) {
        				tmp += (*anfrageListIt)->get_slot();
        				N_anfrage++;
        			}

           }
           ++anfrageListIt;
    	}

 //   	cout<<"tmp= "<<tmp<<endl;


		int x_toplinks = 0;
		int y_toplinks = 0;



		while ( (x_toplinks == 0 && y_toplinks == 0 )  )
		{

			for (int k = 1; k <= tmp; k++)//k = width
			{
				 if (tmp % k == 0)
				 {

					 if ((tmp/k) <= frameObject->get_heightOfFrame()&& k <= frameObject->get_restwidth())
					 {

						// if(frameObject->get_restwidth()==13) // burstList().empty()
						if ( frameObject->get_burstList().empty()) //vr@tud
						{
							 x_toplinks =14-k;
							 y_toplinks =frameObject->get_heightOfFrame()-(tmp / k);
						}
						else
						{
							 x_toplinks =frameObject->get_burstList().back()->get_xCorr() - frameObject->get_burstList().back()->get_width()-k;
							 y_toplinks =frameObject->get_heightOfFrame()-(tmp / k);

						}
					 }
				}
			}
		 tmp++;
		 }


	  float tmp_overhead_area =44 + N_anfrage*16;
	//  cout<<"overheadarea= "<<frameObject->get_overheadarea()<<endl;
	  tmp_overhead_area +=  frameObject->get_overheadarea();
	  int tmp_overhead = ceil(tmp_overhead_area/48);
	  int tmp_overheadX_bl =  tmp_overhead/frameObject->get_heightOfFrame();
	  int tmp_overheadY_bl =  tmp_overhead%frameObject->get_heightOfFrame();
	//  cout<<"tmp_overhead_area= "<<tmp_overhead_area<<endl;
    //  cout<<"overheadX= "<<tmp_overheadX_bl<<endl;
    //  cout<<"overheadY= "<<tmp_overheadY_bl<<endl;
    //  cout<<"x_toplinks= "<<x_toplinks<<endl;
	  if(tmp_overheadX_bl>=x_toplinks && x_toplinks!=0)
	  {

		  frameObject->set_collision(true);
		  return true;
	  }
    }

  return false;
}


bool OcsaAlgorithm::checkCollision_after (vector<anfrage *>* anfrageList,frame * frameObject,int max_burst_slot)
{
	int tmp =0;  //slot of next burst
	int N_anfrage = 0;
	if(!anfrageList->empty())
	{
		vector<anfrage *>::iterator anfrageListIt;
		int j=0;
		    	anfrageListIt = anfrageList->begin();
		    	do {
		    		if((*anfrageListIt)->get_slot()<= frameObject->freeareaList.back()->get_area()){
		    			tmp += (*anfrageListIt)->get_slot();
		    			N_anfrage++;
		    			j = (*anfrageListIt)->get_diuc();
		    		}

		    		++anfrageListIt;

		    	} while ( (anfrageListIt != anfrageList->end()) && (tmp < max_burst_slot)
		    			&& (tmp <= frameObject->freeareaList.back()->get_area() )&& N_anfrage ==0);

		    	while ( (anfrageListIt != anfrageList->end()) && (tmp < max_burst_slot)&& (tmp <= frameObject->freeareaList.back()->get_area() )) {
		           if ( (*anfrageListIt)->get_diuc() == j ) {
		             if ( tmp + (*anfrageListIt)->get_slot() < max_burst_slot && tmp + (*anfrageListIt)->get_slot()<=frameObject->freeareaList.back()->get_area()) {
		        				tmp += (*anfrageListIt)->get_slot();
		        				N_anfrage++;
		        			}

		           }
		           ++anfrageListIt;
		    	}



	    	/*anfrageListIt = anfrageList->begin();
	    	do {
	    		if ( (*anfrageListIt)->get_diuc() == anfrageList->front()->get_diuc() ) {

	    			if ( tmp + (*anfrageListIt)->get_slot() < max_burst_slot) {
	    				tmp += (*anfrageListIt)->get_slot();
	    				N_anfrage++;
	    			}
	    		}
	    		++anfrageListIt;

	    	} while ( (anfrageListIt != anfrageList->end()) && (tmp < max_burst_slot)&&tmp<frameObject->freeareaList.back()->get_area());*/

	 }

	  float tmp_overhead_area =44 + N_anfrage*16;
	  tmp_overhead_area +=  frameObject->get_overheadarea();
	  int tmp_overhead = ceil(tmp_overhead_area/48);

	  if (tmp_overhead > frameObject->get_overheadX()*frameObject->get_heightOfFrame()&& frameObject->get_restwidth()== 0 )
	  {
		  return true;
		  frameObject->set_estimate_overhead(tmp_overhead);
	  }
	 return false;
}
