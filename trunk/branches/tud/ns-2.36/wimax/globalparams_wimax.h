/** RPI
    This is a singleton class , which has been created to access global functions and variables for wimax.
    There is a global beta table and BLER table.


    RPI**/

#include <stdio.h>
#include <fstream>
#include <iostream>
#include <string>

// Search pattern for the ns-2 entry in the PATH environment variable vr@tud 11-09
#define NS2_DIRECTORY "/ns-2.36/"

class GlobalParams_Wimax

{

private:

    //SINR table reading variables
    double * Table_SINR_;

    double * Table_BLER_;

    double * Table_Low_SINR_Bounds_;

    double * Table_High_SINR_Bounds_;

    int * Table_Index_;

    int NumLines_;

    int Num_SINR_Decimals_;

    // beta tabel reading variables
    double Beta_[3][33];

    std::string Ns2_Directory_; //Search path for lookup tables

    GlobalParams_Wimax(const GlobalParams_Wimax&);  //Prevents making a copy

    // SINR table reading functions
    int CountLines( std::ifstream& In );

    int ReadTableFromFile ();

    // beta table reading functions
    int LoadBetaFile();

    /*
     * Search ns2 install path from PATH environment variable - vr@tud 11-09
     */
    void SearchNs2Directory ();


protected:

    GlobalParams_Wimax();       //Only a SysParms member can call this , Prevents a user from creating singleton objects

    virtual ~GlobalParams_Wimax();  //Prevents just anyone from deleting the singleton

public:

    static GlobalParams_Wimax* Instance();        //The "official" access point.

    double GetTablePrecision(double FullDoubleNumber);

    double TableLookup(int Index, double SINR);

    inline double GetBeta(int ITU_PDP, int index) { return Beta_[ITU_PDP][index]; }

    /*
     * Return the ns2 installation directory
     */
    inline std::string GetNs2Directory() {
        return Ns2_Directory_;
    }


};
