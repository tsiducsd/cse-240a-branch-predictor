//========================================================//
//  predictor.c                                           //
//  Source file for the Branch Predictor                  //
//                                                        //
//  Implement the various branch predictors below as      //
//  described in the README                               //
//========================================================//
#include <stdio.h>
#include <math.h>
#include "predictor.h"

//
// TODO:Student Information
//
const char *studentName = "Gandhar Deshpande";
const char *studentID   = "A59005457";
const char *email       = "gdeshpande@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

//define number of bits required for indexing the BHT here. 
int ghistoryBits = 13; // Number of bits used for Global History
int bpType;       // Branch Prediction Type
int verbose;

int tourney_pcBits = 10;
int tourney_localhistBits= 10;
int tourney_ghistoryBits = 12;



//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
//gshare
int *gpredictors;
int gHistoryTable;

//tournament
int tourney_gHistoryTable;
int *tourney_chooser;
int *tourney_gpredictors;
int *tourney_local_bht;
int *tourney_localhist;


//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

//gshare functions
void init_gshare() {
  int historyBits = 1 << ghistoryBits;
  gpredictors = (int*) malloc(historyBits * sizeof(int));
  for(int i = 0; i <= historyBits; i++) {
    gpredictors[i] = WN;
  }
  gHistoryTable = 0;
}

void init_tourney() {
  int global_size = 1 << tourney_ghistoryBits;
  int local_size = 1 << tourney_pcBits;
  int local_width = 1 << tourney_localhistBits;
  tourney_gHistoryTable = 0; //GHR initialized to 0
    tourney_localhist = (int*) malloc( local_size * sizeof(int)); //1024 * 10 local history table
    tourney_local_bht = (int*) malloc( local_width * sizeof(int)); //1024 * 2 local history indexed bht
    tourney_gpredictors = (int*) malloc(global_size * sizeof(int)); //4096 * 2 global indexed bht
    tourney_chooser = (int*) malloc(global_size * sizeof(int)); //4096 * 2 chooser bht
  for(int i = 0; i <= global_size; i++) {
    tourney_gpredictors[i] = WN; 
    tourney_chooser[i] = WN; //weakly choose global predictor initially
  }
  for(int i = 0; i <= local_size; i++) {
    tourney_localhist[i] = WN;
    tourney_local_bht[i] = WN;
  }
  //printf("no problems in initializing\n");
}



uint8_t 
gshare_predict(uint32_t pc) {
  int historyBits = 1 << ghistoryBits;
  int pc_lower_bits = pc & (historyBits - 1);
  int ghistory_lower = gHistoryTable & (historyBits - 1);
  int historyIndex = pc_lower_bits ^ (ghistory_lower);

  switch(gpredictors[historyIndex]) {
    case SN:
      return NOTTAKEN;
    case WN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Undefined state in predictor table");
      return NOTTAKEN;
  }
}

uint8_t 
tourney_global_predict(int ghistory_lower) {

  switch(tourney_gpredictors[ghistory_lower]) {
    case SN:
      return NOTTAKEN;
    case WN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Undefined state in predictor table");
      return NOTTAKEN;
  }
}

uint8_t 
tourney_local_predict(int pc_lower_bits) {

  int local_history = tourney_localhist[pc_lower_bits] & ((1 << tourney_localhistBits) -1);

  switch(tourney_local_bht[local_history]) {
    case SN:
      return NOTTAKEN;
    case WN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Undefined state in predictor table");
      return NOTTAKEN;
  }
}

uint8_t
tourney_predict(uint32_t pc) {
  //printf("predicting \n");
  int historyBits = 1 << tourney_ghistoryBits;
  int pc_lower_bits = pc & ((1 << tourney_pcBits) - 1);
  int ghistory_lower = tourney_gHistoryTable & (historyBits - 1);

  switch(tourney_chooser[ghistory_lower]) {
    case SN:
      return tourney_global_predict(ghistory_lower);
    case WN:
      return tourney_global_predict(ghistory_lower);
    case WT:
      return tourney_local_predict(pc_lower_bits);
    case ST:
      return tourney_local_predict(pc_lower_bits);
    default:
      printf("Undefined state in chooser table");
      return NOTTAKEN;
  }
}


void
train_gshare(uint32_t pc, uint8_t outcome) {
  uint32_t historyBits = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (historyBits - 1);
  uint32_t ghistory_lower = gHistoryTable & (historyBits - 1);
  uint32_t historyIndex = pc_lower_bits ^ (ghistory_lower);

  switch(gpredictors[historyIndex]) {
    case SN:
      gpredictors[historyIndex] = (outcome == TAKEN) ? WN : SN;
      break;
    case WN:
      gpredictors[historyIndex] = (outcome == TAKEN) ? WT : SN;
      break;
    case WT:
      gpredictors[historyIndex] = (outcome == TAKEN) ? ST : WN;
      break;
    case ST:
      gpredictors[historyIndex] = (outcome == TAKEN) ? ST : WT;
      break;
    default:
      break;
  }
  gHistoryTable = ((gHistoryTable << 1 ) | outcome);
}

void
tourney_global_train(int ghistory_lower, uint8_t outcome){
     
    switch(tourney_gpredictors[ghistory_lower]) {
    case SN:
      tourney_gpredictors[ghistory_lower] = (outcome == TAKEN) ? WN : SN;
      break; 
    case WN:
      tourney_gpredictors[ghistory_lower] = (outcome == TAKEN) ? WT : SN;
      break;
    case WT:
      tourney_gpredictors[ghistory_lower] = (outcome == TAKEN) ? ST : WN;
      break;
    case ST:
      tourney_gpredictors[ghistory_lower] = (outcome == TAKEN) ? ST : WT;
      break;
    default:
      printf("Undefined state in predictor table");
      break;
    }
}

void
tourney_local_train(int pc_lower_bits, uint8_t outcome){
   
   //printf("pc_lower_bits = %d \n", pc_lower_bits);
    int local_history = tourney_localhist[pc_lower_bits] & ((1 << tourney_localhistBits) -1);
   //printf("local_history = %d \n", local_history);
    switch(tourney_local_bht[local_history]) {
    case SN:
      tourney_local_bht[local_history] = (outcome == TAKEN) ? WN : SN;
      break;
    case WN:
      tourney_local_bht[local_history] = (outcome == TAKEN) ? WT : SN;
      break;
    case WT:
      tourney_local_bht[local_history] = (outcome == TAKEN) ? ST : WN;
      break;
    case ST:
      tourney_local_bht[local_history] = (outcome == TAKEN) ? ST : WT;
      break;
    default:
      break;
    }
    tourney_localhist[pc_lower_bits] = ((tourney_localhist[pc_lower_bits] << 1) | outcome);
}

void
train_tourney(uint32_t pc, uint8_t outcome) {
  //printf("training \n");
  int historyBits = 1 << tourney_ghistoryBits;
  int pc_lower_bits = pc & ((1 << tourney_pcBits) - 1);
  int ghistory_lower = tourney_gHistoryTable & (historyBits - 1);

  //printf("ghistory_lower = %d \n", ghistory_lower);

  switch(tourney_chooser[ghistory_lower]) {
    case SN:
      tourney_chooser[ghistory_lower] = (outcome == tourney_global_predict(ghistory_lower))? SN: WN;
      break;
    case WN:
      tourney_chooser[ghistory_lower] = (outcome == tourney_global_predict(ghistory_lower))? SN: WT;
      break;
    case WT:
      tourney_chooser[ghistory_lower] = (outcome == tourney_local_predict(pc_lower_bits))? ST: WN;
      break;
    case ST:
      tourney_chooser[ghistory_lower] = (outcome == tourney_local_predict(pc_lower_bits))? ST: WT;
      break;
    default:
      printf("Undefined state in chooser table");
      break;
  }
  tourney_global_train(ghistory_lower, outcome);
  tourney_local_train(pc_lower_bits, outcome);
  //printf("training done\n");
  tourney_gHistoryTable = ((tourney_gHistoryTable << 1 ) | outcome);
}




void
cleanup_gshare() {
  free(gpredictors);
}



void
init_predictor()
{
  switch (bpType) {
    case STATIC:
    case GSHARE:
      init_gshare();
      break;
    case TOURNAMENT:
      init_tourney();
      break;
    case CUSTOM:
    default:
      break;
  }
  
}

// Make a prediction for conditional branch instruction at PC 'pc'
// Returning TAKEN indicates a prediction of taken; returning NOTTAKEN
// indicates a prediction of not taken
//
uint8_t
make_prediction(uint32_t pc)
{

  // Make a prediction based on the bpType
  switch (bpType) {
    case STATIC:
      return TAKEN;
    case GSHARE:
      return gshare_predict(pc);
    case TOURNAMENT:
      return tourney_predict(pc);
    case CUSTOM:
    default:
      break;
  }

  // If there is not a compatable bpType then return NOTTAKEN
  return NOTTAKEN;
}

// Train the predictor the last executed branch at PC 'pc' and with
// outcome 'outcome' (true indicates that the branch was taken, false
// indicates that the branch was not taken)
//

void
train_predictor(uint32_t pc, uint8_t outcome)
{

  switch (bpType) {
    case STATIC:
    case GSHARE:
      return train_gshare(pc, outcome);
    case TOURNAMENT:
      return train_tourney(pc, outcome);
    case CUSTOM:
    default:
      break;
  }
  

}
