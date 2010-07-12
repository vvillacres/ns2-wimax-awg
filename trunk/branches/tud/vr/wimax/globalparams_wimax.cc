/** RPI
    This is a singleton class , which has been created to access global functions and variables for wimax.
    There is a global beta table and BLER table.

    RPI**/

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <math.h>
#include "globalparams_wimax.h"

using namespace std;



namespace
{
GlobalParams_Wimax* instance = 0;        //Address of the singleton
}


GlobalParams_Wimax::GlobalParams_Wimax()
{
    Num_SINR_Decimals_ = 1;
    SearchNs2Directory ();
    int FileOK;
    FileOK=ReadTableFromFile();
    LoadBetaFile();

}

GlobalParams_Wimax::~GlobalParams_Wimax()
{

    delete instance;
    instance = 0;

    //release memory
    free(Table_Index_);
    free(Table_SINR_);
    free(Table_BLER_);
    free(Table_Low_SINR_Bounds_);
    free(Table_High_SINR_Bounds_);

}

//The "official" access point

GlobalParams_Wimax* GlobalParams_Wimax::Instance()
{
    //"Lazy" initialization. Singleton not created until it's needed

    if (!instance) {
        instance = new GlobalParams_Wimax;
    }

    return instance;
}


int GlobalParams_Wimax::CountLines( std::ifstream& In )
{
    return static_cast<int>( count( std::istreambuf_iterator<char>( In),
                                    std::istreambuf_iterator<char>(), '\n' ) );
}

double GlobalParams_Wimax::GetTablePrecision(double FullDoubleNumber)
{
    int TruncatedInt;

    double TruncatedDouble;

    FullDoubleNumber = FullDoubleNumber * pow(10, Num_SINR_Decimals_);
    TruncatedInt= (int) FullDoubleNumber;
    TruncatedDouble = (double) TruncatedInt;

    TruncatedDouble = TruncatedDouble / (pow(10, Num_SINR_Decimals_));

    return TruncatedDouble;

}

double GlobalParams_Wimax::TableLookup(int Index, double SINR)
{
    //in the table provided, SINR is given in increments of 0.1 (ie. 1.1, 1.2, 1.3, etc.)
    //we lookup(Index, truncate(SINR)) and the value above it in the table (Index, truncate(SINR + 0.1))
    //we linearly interpolate the BLER to the actual SINR provided

    double BLER_low=double(NULL);
    double BLER_high=double(NULL); //when BLER_low is found, BLER_high is taken to be the next BLER in the table
    double SINR_low=GetTablePrecision(SINR);
    double SINR_high=SINR_low + pow(10, - Num_SINR_Decimals_); //SINR_high is one "table precision unit" above SINR_low

    cout << "SINR_low = " << SINR_low << endl;
    cout << "SINR_high = " << SINR_high << endl;

    cout << "SINR: " << SINR << endl;
    cout << "Low: " << Table_Low_SINR_Bounds_[Index] << endl;
    cout << "High: " << Table_High_SINR_Bounds_[Index] << endl;

    if (SINR < Table_Low_SINR_Bounds_[Index]) {
        printf("outside of low limit range.\n");
        return 1; // we are outside the range (lower limit) of the table, so the packet is definilty in error
    }

    if (SINR > Table_High_SINR_Bounds_[Index]) {
        printf("outside of upper limit range.\n");
        return 0; //we are outside the rang of the table (upper limit), so the packet is definitly received correctly
    }
    //Check if we have an exact match.  If so, simply return the value from the SINR lookup.
    for (int i=0; i < NumLines_ - 1 ; i++) {
        if ( (Table_Index_[i] == Index) && (Table_SINR_[i] == SINR) ) {
            cout << "Exact match (in TableLookup() ): " << Table_BLER_[i] << endl;
            return Table_BLER_[i];
            break;
        }
    }


    //if we get to here, there has not been an exact match
    //cout << endl << "No exact match... getting interpolated match..." << endl;

    //get bounding BLERs
    for (int i=0; i < NumLines_ - 1 ; i++) {
        //cout << Table_Index[i] << " " << Index << endl;
        //cout << Table_SINR[i] << " " << SINR_low << endl;
        if ( (Table_Index_[i] == Index) && ( Table_SINR_[i] == SINR_low) ) {
            cout << "BLER_low found to be " << Table_BLER_[i] << endl;
            BLER_low = Table_BLER_[i];
            BLER_high = Table_BLER_[i+1];
            cout << "BLER_low found to be " << Table_BLER_[i] <<" high = "<<Table_BLER_[i+1]<< endl;
        }
    }


    double Percent_between_SINR;
    //check if there is a partial match
    if (BLER_low != double(NULL)) {
        //interpolate BLER values
        Percent_between_SINR = (SINR - SINR_low) / (SINR_high - SINR_low); //percent between SINR

        double TableLookupBLER = (BLER_low + (Percent_between_SINR * (BLER_high - BLER_low) ) );
        cout << "BLER after interpolation found to be " << TableLookupBLER << endl;

        return TableLookupBLER;
    }

    //if all else fails
    cout << "No Match Found!" << endl;
    return double( NULL);


}

int GlobalParams_Wimax::ReadTableFromFile ()
{

    cout << "Enter ReadTableFromFile()" << endl;

    int NumIndex = 32;
    //open file to get numLines
    ifstream TableFile;
    TableFile.open( string( Ns2_Directory_ + "BLER_LookupTable.txt").c_str());

    if (TableFile.fail()) {

    	cerr << "Error opening file BLER_LookupTable.txt in directory " << Ns2_Directory_ << endl;
    	// try to open this file in the current directory
    	TableFile.open( "BLER_LookupTable.txt");

    	if (TableFile.fail()) {

    		// File not found
    		cerr << "Error opening file BLER_LookupTable.txt in the current directory " << endl;
    		exit (1);
    	}
    }

    NumLines_ = CountLines(TableFile);
    //cout << "num lines = " << NumLines << endl;

    // Replaces reopen file
    TableFile.seekg(0, ios::beg);

    Table_Index_ = (int *) malloc(NumLines_ * sizeof(int) );
    //check if malloc worked
    if (Table_Index_ == NULL) {
        fprintf(stderr,"Error: Table_Index_ is NULL\n");
        free(Table_Index_);
        exit(1);
    }

    Table_SINR_ = (double *) malloc(NumLines_ * sizeof(double) );
    //check if malloc worked
    if (Table_SINR_ == NULL) {
        fprintf(stderr,"Error: Table_SINR_ is NULL\n");
        free(Table_SINR_);
        exit(1);
    }

    Table_BLER_ = (double *) malloc(NumLines_ * sizeof(double) );
    //check if malloc worked
    if (Table_BLER_ == NULL) {
        fprintf(stderr,"Error: Table_BLER_ is NULL\n");
        free(Table_BLER_);
        exit(1);
    }

    Table_Low_SINR_Bounds_ = (double *) malloc((NumIndex + 1) * sizeof(double) ); //so array indexes match table indexes
    //check if malloc worked
    if (Table_Low_SINR_Bounds_ == NULL) {
        fprintf(stderr,"Error: Table_Low_SINR_Bounds is NULL\n");
        free(Table_Low_SINR_Bounds_);
        exit(1);
    }

    Table_High_SINR_Bounds_ = (double *) malloc((NumIndex + 1)* sizeof(double) ); //so array indexes match table indexes
    //check if malloc worked
    if (Table_High_SINR_Bounds_ == NULL) {
        fprintf(stderr,"Error: Table_High_SINR_Bounds_ is NULL\n");
        free(Table_High_SINR_Bounds_);
        exit(1);
    }

    //cout << "Before input loop" << endl;

    int CurrentLine=0;
    while (!TableFile.eof()) {
        TableFile >> Table_Index_[CurrentLine];
        //cout << "Index: " << Table_Index[CurrentLine] << endl;
        TableFile >> Table_SINR_[CurrentLine];
        //cout << "SINR: " << Table_SINR[CurrentLine] << endl;
        TableFile >> Table_BLER_[CurrentLine];
        //cout << "BLER: " << Table_BLER[CurrentLine] << endl;

        CurrentLine++;
    }

    TableFile.close(); // close the file

    //cout << "after read file" << endl;

    Table_Low_SINR_Bounds_[0]=double(NULL); //not used
    Table_High_SINR_Bounds_[0]=double(NULL); //not used

    Table_Low_SINR_Bounds_[1]=Table_SINR_[0]; //first line in file
    //	cout << "LOW 1 = " << Table_Low_SINR_Bounds[1] << endl;

    Table_High_SINR_Bounds_[NumIndex]=Table_SINR_[NumLines_ - 1]; //last line in file
    //	cout << "HIGH 32 = " << Table_High_SINR_Bounds[NumIndex] << endl;

    int BreaksFound = 0;
    //cout << "Before break finding loop..." << endl;

    for (int counter=1; counter <= NumLines_; counter++) {
        int TempIndex1, TempIndex2;
        TempIndex1=Table_Index_[counter];
        TempIndex2=Table_Index_[counter + 1];
        if (TempIndex1 != TempIndex2) {
            Table_High_SINR_Bounds_[BreaksFound + 1] = Table_SINR_[counter];
            Table_Low_SINR_Bounds_[BreaksFound + 2] = Table_SINR_[counter + 1];

            BreaksFound++;
            //			cout << "Breaks Found = " << BreaksFound << endl;

            //			cout << "HIGH " << (BreaksFound) << " = " << Table_High_SINR_Bounds[BreaksFound] << endl;
            //			cout << "LOW " << (BreaksFound + 1) << " = " << Table_Low_SINR_Bounds[BreaksFound + 1] << endl;
            //			cout << endl;
        }

        if (BreaksFound == (NumIndex - 1) ) {
            break;
        }
    }

    //if we get to here, all is well
    return 1;
}


int GlobalParams_Wimax::LoadBetaFile()
{

    ifstream BetaFile;
    BetaFile.open( string( Ns2_Directory_ + "BetaTable.txt").c_str());

    if (BetaFile.fail()) {

    	cerr << "Error opening file BetaTable.txt in directory " << Ns2_Directory_ << endl;

    	// try to open the file in the current directory

    	BetaFile.open( "BetaTable.txt");

    	if (BetaFile.fail()) {

    		// File not found
    		cerr << "Error opening file BetaTable.txt in directory " << endl;
    		exit (1);
    	}
    }

    /*Model Index
      PedB=1
      VehA=2
    */

    int Model, Index;
    double TempBeta;

    while (!BetaFile.eof()) {
        BetaFile >> Model;
        //cout << "Model " << Model << endl;
        BetaFile >> Index;
        //cout << "Index " << Index << endl;
        BetaFile >> TempBeta;
        //cout << "Beta " << TempBeta << endl;

        Beta_[Model][Index] = TempBeta;
    }

    //cout << endl << Beta[1][16] << endl;

    return 0;
}

/*
 * Get ns2 install path from PATH environment variable - vr@tud 11-09
 */
void GlobalParams_Wimax::SearchNs2Directory ()
{
    string path_environment_variable;
    size_t position_of_ns2_entry;
    size_t first_pos_of_entry;
    size_t last_pos_of_entry;

    path_environment_variable = string( getenv( "PATH"));
    position_of_ns2_entry = path_environment_variable.find( NS2_DIRECTORY);

    if ( position_of_ns2_entry == string::npos) {
        // search pattern not found --> take working directory
        Ns2_Directory_ = "./";
    } else {
        // search pattern found --> extract path
        first_pos_of_entry = path_environment_variable.find_last_of( ':', position_of_ns2_entry);
        last_pos_of_entry = path_environment_variable.find_first_of( ':', position_of_ns2_entry);
        Ns2_Directory_ = path_environment_variable.substr( first_pos_of_entry + 1, last_pos_of_entry - first_pos_of_entry - 1);
    }

    cout << "Path to the ns2 directory is: " << Ns2_Directory_ << endl;
}
