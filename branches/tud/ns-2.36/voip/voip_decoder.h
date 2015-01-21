/*
 * TODO: INSERT LICENSE HERE
 */

#ifndef __NS2_VOIP_DECODER_H
#define __NS2_VOIP_DECODER_H

#include <app.h>
#include <object.h>
#include <voip_frame.h>
#include <tfunctor.h>

class VoipDecoder : public Application {
public:
	//! Create an empty VoIP source.
	VoipDecoder ();

	//! Process incoming VoIP payloads.
	virtual void process_data(int size, AppData* data);

	//! Tcl interface.
	int command (int argc, const char*const* argv);

protected:
	//! Collect statistics after playout buffering frame by frame.
	virtual void collectFrame (VoipFrame& frame);
	//! Collect statistics after playout buffering talkspurt by talkspurt.
	virtual void collectTalkspurt (unsigned int nframes) { }
	//! Collect statistics after playout buffering at the end of simulation.
	virtual void collectEnd () { }

	//! Compute the Mean Opinion Score (MOS).
	virtual double eModel (double delay, double loss);

	//! Create a call-back object to manage statistical data collection.
	TSpecificFunctor<VoipDecoder> functor_;

protected:
	//! True if debug is enabled.
	bool debug_;

	//! Statistic Metric ID.
	int id_;
	//! Cell Id. The identifier of my cell. Set via Tcl.
	int cellId_;
	//! Peer Id. The identifier of my peer. Set via Tcl.
	int peerId_;

	// :TODO: commentare
	unsigned int currentFrame_;
	unsigned int currentTalk_;
	unsigned int currentTalkSize_;
	double delaySum_;
	unsigned int delayNum_;
	bool firstFrame_;

	//! Transmission Rating Factor Ro Parameters
	/** The term Ro expresses the basic signal-to-noise ratio 
	* (received speech level relative to circuit and acoustic 
	* noise).
	*/
	double emodel_Ro_;

        //! Expectation Factor (Advantage Factor)

        /** The factor is representing a "bonus", a tolerance given by
         *  the users to some technologies or circumstances.
         *  This value is given in units of impairments, so that it 
         *  can be directly used in conjunction with other Impairmant 
         *  Factors.
         *  - No Unit (units of impairments)
         *  - Typical range is [0,20].
         *  <ul>
         *   <li>  0 - Conventional fixed line PSTN </li>
         *   <li>  5 - Local area wireless (cordless phone) </li>
         *   <li> 10 - Wide area wireless (cell phone) </li>
         *   <li> 20 - Satellite </li>
         *  </ul>
	 */
	double emodel_A_;

	//! Equipment Impairment Factor
	/** Specific (modern) equipments used in the transmission 
	*  path, such as ADPCM encoders, compression 
	*  (G.728, GSM 6.x0) produce distortions causing a decrease 
	*  of the perceived quality. They cannot be described with 
	*  the Quantizing Distortion Unit, and have been estimated 
	*  subjectively.
	*  This value is given in units of impairments, so that it 
	*  can be directly used in conjunction with other Impairmant 
	*  Factors.
	*  - No Unit (units of impairments)
	*  - Typical range is [0,50].
	*/
	double emodel_Ie_;

	//! Packet Loss Robustness Factor
	/** Bpl is the robustness of a codec to random packet loss.
	*/
	double emodel_Bpl_;

	//! MOS threshold.
	/** If the MOS of a talkspurt is greater than or equal to this threshold,
	 * then the perceived quality of that talkspurt is considered to be good.
	 */
	double mosThreshold_;

	//! Outage MOS threshold.
	/** If the ratio between the 'good' talkspurts, according to the MOS
	 * threshold, and the total number of talkspurts is greater than or equal
	 * to this threshold, then it is assumed that the user perceives an
	 * overall good quality.
	 */
	double cellOutageMosThreshold_;

	//! Outage Loss threshold.
	/** If the percentage of loss packets is under
	 *  this threshold, then it is assumed that the user perceives an
	 * overall good quality.
	 */
	double cellOutageLossThreshold_;

	//! Pointer to the next VoipDecoder
	/** VoIP frames traverses a chain of decoder
	 *  in order to collect more statistic types
	 *  in a single simulation run.
	 */ 
	VoipDecoder* nextDecoder_;
};

#endif // __NS2_VOIP_DECODER_H
