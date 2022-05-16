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
const char *studentName = "Siddharth Thinakaran";
const char *studentID   = "A59005429";
const char *email       = "sthinakaran@ucsd.edu";

//------------------------------------//
//      Predictor Configuration       //
//------------------------------------//

// Handy Global for use in output routines
const char *bpName[4] = { "Static", "Gshare",
                          "Tournament", "Custom" };

//define number of bits required for indexing the BHT here. 
int ghistoryBits = 14; // Number of bits used for Global History
int bpType;       // Branch Prediction Type
int verbose;

int tourney_pcBits = 10;
int tourney_localhistBits= 11;
int tourney_ghistoryBits = 12;

int yags_histBits = 12;
int yags_pcBits = 11;
int yags_pcBits_tag = 0;
int yags_histBits_chooser = 2;
int yags_histBits_tag = 0;



//------------------------------------//
//      Predictor Data Structures     //
//------------------------------------//

//
//TODO: Add your own Branch Predictor data structures here
//
//gshare
uint8_t *bht_gshare;
uint64_t ghistory;

//tournament
uint64_t tourney_gHistoryTable;
uint8_t *tourney_chooser;
uint8_t *tourney_gpredictors;
uint8_t *tourney_local_bht;
uint32_t *tourney_localhist;

//custom-perceptron


uint64_t yags_ghistory;
uint8_t *yags_T_bht_table;
uint32_t *yags_T_tag_table;
uint8_t *yags_NT_bht_table;
uint32_t *yags_NT_tag_table;
uint8_t *yags_choice_table;

//------------------------------------//
//        Predictor Functions         //
//------------------------------------//

// Initialize the predictor
//

//gshare functions
void init_gshare() {
 int bht_entries = 1 << ghistoryBits;
  bht_gshare = (uint8_t*)malloc(bht_entries * sizeof(uint8_t));
  int i = 0;
  for(i = 0; i< bht_entries; i++){
    bht_gshare[i] = WN;
  }
  ghistory = 0;
}

void init_tourney() {
  int global_size = 1 << tourney_ghistoryBits;
  int local_size = 1 << tourney_pcBits;
  int local_width = 1 << tourney_localhistBits;

  int tsize = ( ((1 << tourney_pcBits) * tourney_localhistBits) + ((1 << tourney_localhistBits) * 2 ) + (1 << tourney_ghistoryBits)*4);
  printf("Total size = %d\n", tsize);
    tourney_localhist = (uint32_t*) malloc( local_size * sizeof(uint32_t)); //1024 * 10 local history table
    tourney_local_bht = (uint8_t*) malloc( local_width * sizeof(uint8_t)); //1024 * 2 local history indexed bht
    tourney_gpredictors = (uint8_t*) malloc(global_size * sizeof(uint8_t)); //4096 * 2 global indexed bht
    tourney_chooser = (uint8_t*) malloc(global_size * sizeof(uint8_t)); //4096 * 2 chooser bht
  for(int i = 0; i < global_size; i++) {
    tourney_gpredictors[i] = WN; 
    tourney_chooser[i] = WN; //weakly choose global predictor initially
  }
  for(int i = 0; i < local_size; i++) {
    tourney_localhist[i] = 0;
    tourney_local_bht[i] = WN;
  }
  tourney_gHistoryTable = 0;
}


void init_yags() {
  int global_size = 1 << yags_histBits;
  int tag_size = yags_pcBits_tag + yags_histBits_tag ;
  int chooser_size = 1 << (yags_pcBits+yags_histBits_chooser);
  int tsize = global_size * (4+2*tag_size) + chooser_size * 2;

  printf("Total size = %d\n", tsize);
    yags_T_bht_table = (uint8_t*) malloc( global_size * sizeof(uint8_t)); //
    yags_T_tag_table = (uint32_t*)malloc( global_size * sizeof(uint32_t));
    yags_NT_bht_table = (uint8_t*) malloc(global_size * sizeof(uint8_t)); //
    yags_NT_tag_table  = (uint32_t*)malloc( global_size * sizeof(uint32_t));
    yags_choice_table = (uint8_t*) malloc(chooser_size * sizeof(uint8_t)); //
  for(int i = 0; i < global_size; i++) {
    yags_T_bht_table[i] = WN; 
    yags_T_tag_table[i] = 255;
    yags_NT_bht_table[i] = WN;
    yags_NT_tag_table[i] = 255;
  }
  for(int i = 0; i < chooser_size; i++) {
    yags_choice_table[i] = WN;
  }
  yags_ghistory=0;
}


uint8_t 
gshare_predict(uint32_t pc) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;
  switch(bht_gshare[index]){
    case WN:
      return NOTTAKEN;
    case SN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
      return NOTTAKEN;
  }
}

uint8_t 
tourney_global_predict(uint32_t ghistory_lower) {

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
tourney_local_predict(uint32_t pc_lower_bits) {

  uint32_t local_history = tourney_localhist[pc_lower_bits] & ((1 << tourney_localhistBits) -1);

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
  uint32_t historyBits = 1 << tourney_ghistoryBits;
  uint32_t pc_lower_bits = pc & ((1 << tourney_pcBits) - 1);
  uint32_t ghistory_lower = tourney_gHistoryTable & (historyBits - 1);
  uint32_t chooser = ghistory_lower;

  switch(tourney_chooser[chooser]) {
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

uint8_t
yags_T_predict(uint32_t index) {
  switch(yags_T_bht_table[index]) {
    case SN:
      return NOTTAKEN;
    case WN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Warning: Undefined state of entry in yags true BHT!\n");
      return NOTTAKEN;
  }
}

uint8_t
yags_NT_predict(uint32_t index) {
  switch(yags_NT_bht_table[index]) {
    case SN:
      return NOTTAKEN;
    case WN:
      return NOTTAKEN;
    case WT:
      return TAKEN;
    case ST:
      return TAKEN;
    default:
      printf("Warning: Undefined state of entry in yags not true BHT!\n");
      return NOTTAKEN;
  }
}

uint8_t
yags_predict(uint32_t pc) {
  //printf("predicting \n");
  uint32_t historyBits = 1 << yags_histBits;
  uint32_t pc_lower_bits = pc & (historyBits - 1);
  uint32_t hist_chooser_mask = ((1 << yags_histBits_chooser) -1) << yags_histBits; 
  uint32_t pc_index_bits = ((yags_ghistory & hist_chooser_mask) >> (yags_histBits-yags_pcBits)) ^ (pc & ((1 << yags_pcBits) - 1)); //addressed only by PC
  uint32_t ghistory_lower = yags_ghistory & (historyBits - 1); 
  uint32_t index = pc_lower_bits ^ ghistory_lower; //cache index same as gshare

  uint32_t pc_tag_mask = (1 << yags_pcBits_tag) -1;
  uint32_t hist_tag_mask = ((1 << yags_histBits_tag) -1) << yags_histBits; //14th to 11th bit mask
  uint32_t tag = ((yags_ghistory & hist_tag_mask) >> (yags_histBits-yags_pcBits_tag)) | ((pc>>yags_pcBits) & pc_tag_mask);
  //higher 4 bits are from the 14th to 11th bits of global history, the lower 6 bits are the lower 6 bits of the PC.
  //efficient way to include correlation history? aliasing doesnt seem to be the problem, limited global history is.
  //printf("tag = %x\n",(uint32_t)(hist_tag_mask));

  switch(yags_choice_table[pc_index_bits]) {
    case SN:
      if(yags_T_tag_table[index] == tag)
        return yags_T_predict(index);
      else
        return NOTTAKEN;
    case WN:
      if(yags_T_tag_table[index] == tag)
        return yags_T_predict(index);
      else
        return NOTTAKEN;
    case WT:
      if(yags_NT_tag_table[index] == tag)
        return yags_NT_predict(index);
      else
        return TAKEN;
    case ST:
      if(yags_NT_tag_table[index] == tag)
        return yags_NT_predict(index);
      else
        return TAKEN;
    default:
      printf("Undefined state in chooser table");
      return NOTTAKEN;
  }
}


void
train_gshare(uint32_t pc, uint8_t outcome) {
  //get lower ghistoryBits of pc
  uint32_t bht_entries = 1 << ghistoryBits;
  uint32_t pc_lower_bits = pc & (bht_entries-1);
  uint32_t ghistory_lower_bits = ghistory & (bht_entries -1);
  uint32_t index = pc_lower_bits ^ ghistory_lower_bits;

  //Update state of entry in bht based on outcome
  switch(bht_gshare[index]){
    case WN:
      bht_gshare[index] = (outcome==TAKEN)?WT:SN;
      break;
    case SN:
      bht_gshare[index] = (outcome==TAKEN)?WN:SN;
      break;
    case WT:
      bht_gshare[index] = (outcome==TAKEN)?ST:WN;
      break;
    case ST:
      bht_gshare[index] = (outcome==TAKEN)?ST:WT;
      break;
    default:
      printf("Warning: Undefined state of entry in GSHARE BHT!\n");
  }

  //Update history register
  ghistory = ((ghistory << 1) | outcome); 
}

void
tourney_global_train(uint32_t ghistory_lower, uint8_t outcome){
     
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
tourney_local_train(uint32_t pc_lower_bits, uint8_t outcome){
   
   //printf("pc_lower_bits = %d \n", pc_lower_bits);
    uint32_t local_history = tourney_localhist[pc_lower_bits] & ((1 << tourney_localhistBits) -1);
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
  uint32_t historyBits = 1 << tourney_ghistoryBits;
  uint32_t pc_lower_bits = pc & ((1 << tourney_pcBits) - 1);
  uint32_t ghistory_lower = tourney_gHistoryTable & (historyBits - 1);
  uint32_t chooser = ghistory_lower;
  //printf("ghistory_lower = %d \n", ghistory_lower);

  switch(tourney_chooser[chooser]) {
    case SN:
      tourney_chooser[chooser] = (outcome == tourney_global_predict(ghistory_lower))? SN: (outcome == tourney_local_predict(pc_lower_bits)) ? WN : SN;
      break;
    case WN:
      tourney_chooser[chooser] = (outcome == tourney_global_predict(ghistory_lower))? SN: (outcome == tourney_local_predict(pc_lower_bits)) ? WT : WN;
      break;
    case WT:
      tourney_chooser[chooser] = (outcome == tourney_local_predict(pc_lower_bits))? ST: (outcome == tourney_global_predict(ghistory_lower)) ? WN : WT;
      break;
    case ST:
      tourney_chooser[chooser] = (outcome == tourney_local_predict(pc_lower_bits))? ST: (outcome == tourney_global_predict(ghistory_lower)) ? WT : ST;
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
yags_T_train(uint32_t index, uint8_t outcome){
     
switch(yags_T_bht_table[index]) {
    case SN:
      yags_T_bht_table[index] = (outcome==TAKEN) ? WN : SN;
      break;
    case WN:
      yags_T_bht_table[index] = (outcome==TAKEN) ? WT : SN;
      break;
    case WT:
      yags_T_bht_table[index] = (outcome==TAKEN) ? ST : WN;
      break;
    case ST:
      yags_T_bht_table[index] = (outcome==TAKEN) ? ST : WT;
      break;
    default:
      break;
  }
}

void
yags_NT_train(uint32_t index, uint8_t outcome){
     
switch(yags_NT_bht_table[index]) {
    case SN:
      yags_NT_bht_table[index] = (outcome==TAKEN) ? WN : SN;
      break;
    case WN:
      yags_NT_bht_table[index] = (outcome==TAKEN) ? WT : SN;
      break;
    case WT:
      yags_NT_bht_table[index] = (outcome==TAKEN) ? ST : WN;
      break;
    case ST:
      yags_NT_bht_table[index] = (outcome==TAKEN) ? ST : WT;
      break;
    default:
      break;
  }
}

void
yags_train(uint32_t pc, uint8_t outcome) {
  //printf("predicting \n");
  uint32_t historyBits = 1 << yags_histBits;
  uint32_t pc_lower_bits = pc & (historyBits - 1);
  uint32_t hist_chooser_mask = ((1 << yags_histBits_chooser) -1) << yags_histBits; 
  uint32_t pc_index_bits = ((yags_ghistory & hist_chooser_mask) >> (yags_histBits-yags_pcBits)) | (pc & ((1 << yags_pcBits) - 1)); //addressed only by PC //addressed only by PC
  uint32_t ghistory_lower = yags_ghistory & (historyBits - 1); 
  uint32_t index = pc_lower_bits ^ ghistory_lower; //cache index same as gshare

  uint32_t pc_tag_mask = (1 << yags_pcBits_tag) -1;
  uint32_t hist_tag_mask = ((1 << yags_histBits_tag) -1) << yags_histBits; //14th to 11th bit mask
  uint32_t tag = ((yags_ghistory & hist_tag_mask) >> (yags_histBits-yags_pcBits_tag)) | ((pc>>yags_pcBits) & pc_tag_mask);
  //higher 4 bits are from the 14th to 11th bits of global history, the lower 6 bits are 6 bits of pc after whatever chooser uses.
  //efficient way to include correlation history? aliasing doesnt seem to be the problem, limited global history is.
  //printf("tag = %x\n",(uint32_t)(tag));
  switch(yags_choice_table[pc_index_bits]) {
    case SN:
      if(yags_T_tag_table[index] != tag){ //cache miss
        if(outcome == TAKEN) { //chooser table wrong prediction, update cache with this outlier
          yags_T_train(index, outcome);
          yags_T_tag_table[index] = tag;
        }
        //if cache miss and chooser was correct, no need to update cache bht.
        yags_choice_table[pc_index_bits] == (outcome==TAKEN) ? WN : SN; //update chooser table always if cache miss
      }
      else { //cache hit
        if(yags_T_predict(index) != outcome) //cache bht incorrect
        { 
          yags_choice_table[pc_index_bits] = (outcome == TAKEN)? WN : SN; //do not update chooser when cache bht is correct
        }
        yags_T_train(index, outcome); //update the cache bht always if cache hit
        yags_T_tag_table[index] = tag;
      }
      break;
    case WN:
      if(yags_T_tag_table[index] != tag){ //cache miss
        if(outcome == TAKEN) { //chooser table wrong prediction, update cache with this outlier
          yags_T_train(index, outcome);
          yags_T_tag_table[index] = tag;
        }
        //if cache miss and chooser was correct, no need to update cache.
        yags_choice_table[pc_index_bits] == (outcome==TAKEN) ? WT : SN; //update chooser table always if cache miss
      }
      else { //cache hit
        if(yags_T_predict(index) != outcome) //cache bht incorrect
        { 
          yags_choice_table[pc_index_bits] = (outcome == TAKEN)? WT : SN; //do not update chooser when cache bht is correct
        }
        yags_T_train(index, outcome); //update the cache bht always if cache hit
        yags_T_tag_table[index] = tag;
      }
      break;
    case WT:
      if(yags_NT_tag_table[index] != tag){ //cache miss
        if(outcome == NOTTAKEN) { //chooser table wrong prediction, update cache with this outlier
          yags_NT_train(index, outcome);
          yags_NT_tag_table[index] = tag;
        }
        //if cache miss and chooser was correct, no need to update cache.
        yags_choice_table[pc_index_bits] == (outcome==TAKEN) ? ST : WN; //update chooser table always if cache miss
      }
      else { //cache hit
        if(yags_NT_predict(index) != outcome) //cache bht incorrect
        { 
          yags_choice_table[pc_index_bits] = (outcome == TAKEN)? ST : WN; //do not update chooser when cache bht is correct
        }
        yags_NT_train(index, outcome); //update the cache bht always if cache hit
        yags_NT_tag_table[index] = tag;
      }
      break;
    case ST:
      if(yags_NT_tag_table[index] != tag){ //cache miss
        if(outcome == NOTTAKEN) { //chooser table wrong prediction, update cache with this outlier
          yags_NT_train(index, outcome);
          //yags_NT_tag_table[index] = tag;
        }
        //if cache miss and chooser was correct, no need to update cache.
        yags_choice_table[pc_index_bits] == (outcome==TAKEN) ? ST : WT; //update chooser table always if cache miss
      }
      else { //cache hit
        if(yags_NT_predict(index) != outcome) //cache bht incorrect
        { 
          yags_choice_table[pc_index_bits] = (outcome == TAKEN)? ST : WT; //do not update chooser when cache bht is correct
        }
        yags_NT_train(index, outcome); //update the cache bht always if cache hit
        yags_NT_tag_table[index] = tag;
      }
      break;
    default:
      break;
  }
  yags_ghistory = ((yags_ghistory << 1 ) | outcome);
}


void
cleanup_gshare() {
  free(bht_gshare);
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
      init_yags();
      break;
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
      return yags_predict(pc);
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
      return yags_train(pc, outcome);
    default:
      break;
  }
  

}
