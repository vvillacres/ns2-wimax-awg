// COST231 Channel Model calculations: Raj Iyengar, RPI
//

#include <math.h>
#include <delay.h>
#include <packet.h>
#include <packet-stamp.h>
#include <antenna.h>
#include <mobilenode.h>
#include "propagation.h"
#include "wireless-phy.h"
#include "cost231.h"

#include <iostream> //RPI

static class Cost231Class: public TclClass
{
public:
    Cost231Class() : TclClass("Propagation/Cost231") {}
    TclObject* create(int, const char*const*) {
        return (new Cost231);
    }
} class_cost231;

Cost231::Cost231()
{
}

/*
 * L = 46.3 + 33.9 log10(f) - 13.82 log10(h_b) - C_H + [44.9 -6.55 log10(h_B)]logd + C
 * L: median path loss
 * f: frequency of transmission in MHz
 * h_b: base station antenna effective height (in meters)
 * d: link distance in km
 * C_H: mobile station antenna height correction factor as described for the
 *      Hata model for Urban Areas
 * C: 0 dB for medium cities and suburban areas, 3 dB for metropolitan areas
 */
double Cost231::cost231_formula(double Pt, double Gt, double Gr, double h_b, double h_m, double d, double f) // am using the COST231 formula for now.
{

    //NOTE: if the resulting powers are printed out, then we see that for some of the links, power is actally gained at the receiver
    //compared to what is sent out by the transmitter
    //This happens for MS<->MS links and BS<->BS links, due to antenna heights being the same in both cases, rendering COST231 inapplicable.

    //log() here is natural log
    // h_m: height of the mobile antenna, used in the calculation of C_H
    double loss_in_db, Pr_dB, Pr;
    double C_H; // the mobile antenna height correction factor
    double C = 0 ; // assuming medium cities for the time being, need to make this settable from tcl

    // f=2e9; // 2 GHz // f is now out of the range of the cost hata model but the coverage should be more reasonable than for 2 GHz vr@tud

    C_H  = ( 1.1 * log10( f / 1e6 ) - 0.7)*h_m - 1.56*log10( f / 1e6 ) + 0.8; // from "COST 231 Final Report" chapter 4 page 135

    if (f==0 || h_b==0 || d==0 || Pt==0) {
        fprintf(stderr, "ERROR: Mobile nodes are too close to the BS, please double check the distance and run it again f :%g, h_b :%g, d :%g, Pt :%g\n", f, h_b, d, Pt);
        exit(1);
    }

    loss_in_db = 46.3 + 33.9*log10( f / 1e6 ) - 13.82 * log10(h_b) - C_H + (44.9-6.55*(log10(h_b)))*log10( d / 1e3) + C;

    double Pt_dB = 10*log10(Pt);

    //Pr = 10.0*log(Pt)/2.303 + loss_in_db ; // ORIGINAL
    Pr_dB = Pt_dB - loss_in_db; //RPI

    // in WATTS with antenna gain FACTORS from ns-2 antenna model
    Pr = pow (10.0,0.1*Pr_dB) * Gt * Gr;

    return Pr;
}

double Cost231::Pr(PacketStamp *t, PacketStamp *r, WirelessPhy *ifp)
{
    double rX, rY, rZ;		// location of receiver
    double tX, tY, tZ;		// location of transmitter
    double d;				// distance in m
    double h_b, h_m;		// height of recv and xmit antennas
    double Gt, Gr;			// antenna gain factors of transmit and receive antennas
    double Pr;				// received signal power

    double lambda = ifp->getLambda();	// wavelength
    double frequency = SPEED_OF_LIGHT/lambda;

    r->getNode()->getLoc(&rX, &rY, &rZ);
    t->getNode()->getLoc(&tX, &tY, &tZ);

    rX += r->getAntenna()->getX();
    rY += r->getAntenna()->getY();
    tX += t->getAntenna()->getX();
    tY += t->getAntenna()->getY();

    // find the distance between the two nodes
    d = sqrt((rX - tX) * (rX - tX)
             + (rY - tY) * (rY - tY)
             + (rZ - tZ) * (rZ - tZ));

    if (d< 0.00000000001) {
    	fprintf(stderr,"Distance is zero\n");
        exit(1);
    }

    // ae@tud and vr@tud added distinction between BS and MS
    if (t->getNode()->address() == t->getNode()->base_stn()) {
        h_b = tZ + t->getAntenna()->getZ();
        h_m = rZ + r->getAntenna()->getZ();
    } else if (r->getNode()->address() == r->getNode()->base_stn()) {
        h_b = rZ + r->getAntenna()->getZ();
        h_m = tZ + t->getAntenna()->getZ();
    } else {
        // neither receiver nor transmitter are the base station
    	// we assume, that the station with the greater antenna height is the bs
        h_b = tZ + t->getAntenna()->getZ();
        h_m = rZ + r->getAntenna()->getZ();
        if ( h_b < h_m) {
            double h_temp = h_b;
            h_b = h_m;
            h_m = h_temp;
        }
    }

	Gt = t->getAntenna()->getTxGain(rX - tX, rY - tY, rZ - tZ, t->getLambda());
	Gr = r->getAntenna()->getRxGain(tX - rX, tY - rY, tZ - rZ, r->getLambda());

    Pr = cost231_formula(t->getTxPr(), Gt, Gr, h_b, h_m, d, frequency);
    return Pr;
}

// this really does nothing and i never call it, just left it here for legacy sake, i think channel.cc calls it.
// after my best knowledge it is used for coverage calculations with the threshold RXThresh_
double Cost231::getDist(double Pr, double Pt, double Gt, double Gr, double hr, double ht, double L, double lambda)
{
    /* Get quartic root */
    return sqrt(sqrt(Pt * Gt * Gr * (hr * hr * ht * ht) / Pr));
}
