#include "predictor.h"
#include <assert.h>
#include <stdio.h>

#define STRONGLY_TAKEN 3
#define WEAKLY_TAKEN 2
#define WEAKLY_NOT_TAKEN 1
#define STRONGLY_NOT_TAKEN 0

/////////////////////////////////////////////////////////////
// 2bitsat
/////////////////////////////////////////////////////////////

int two_bit_table[4096];
int mask, idx;

void InitPredictor_2bitsat() {
  int i;
  for (i = 0; i < 4096; i++)
    two_bit_table[i] = WEAKLY_NOT_TAKEN;
}

bool GetPrediction_2bitsat(UINT32 PC) {
  int ret;
  mask = 0xfff;
  idx = mask & PC;
  assert(idx >= 0 && idx <= 4095);

  if (two_bit_table[idx] == STRONGLY_TAKEN || two_bit_table[idx] == WEAKLY_TAKEN)
    ret = TAKEN;
  else if (two_bit_table[idx] == STRONGLY_NOT_TAKEN || two_bit_table[idx] == WEAKLY_NOT_TAKEN)
    ret = NOT_TAKEN;
  else
    assert(1 == 0);

  return ret;
}

void UpdatePredictor_2bitsat(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  if (resolveDir == TAKEN && two_bit_table[idx] != STRONGLY_TAKEN)
    two_bit_table[idx]++;
  else if (resolveDir == NOT_TAKEN && two_bit_table[idx] != STRONGLY_NOT_TAKEN)
    two_bit_table[idx]--;
}

/////////////////////////////////////////////////////////////
// 2level
/////////////////////////////////////////////////////////////

int bht[512];
int pht[64][8];

void InitPredictor_2level() {
  int i, j;
  for (i = 0; i < 512; i++)
    bht[i] = 0;
  for (i = 0; i < 64; i++)
    for (j = 0; j < 8; j++)
      pht[i][j] = WEAKLY_NOT_TAKEN;
}

bool GetPrediction_2level(UINT32 PC) {
  int mask_bht, mask_pht, idx_bht, idx_pht, ret;

  mask_bht = 0b111111111;
  mask_pht = 0b111;
  idx_bht = (PC >> 3) & mask_bht;
  idx_pht = PC & mask_pht;

  assert(idx_bht >= 0 && idx_bht <= 511);
  assert(idx_pht >= 0 && idx_pht <= 7);
  assert(bht[idx_bht] >= 0 && bht[idx_bht] <= 63);

  if (pht[bht[idx_bht]][idx_pht] == STRONGLY_TAKEN || pht[bht[idx_bht]][idx_pht] == WEAKLY_TAKEN)
    ret = TAKEN;
  else if (pht[bht[idx_bht]][idx_pht] == STRONGLY_NOT_TAKEN || pht[bht[idx_bht]][idx_pht] == WEAKLY_NOT_TAKEN)
    ret = NOT_TAKEN;
  else
    assert(1 == 0);

  return ret;
}

void UpdatePredictor_2level(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int mask_bht, mask_pht, idx_bht, idx_pht;

  mask_bht = 0b111111111;
  mask_pht = 0b111;
  idx_bht = (PC >> 3) & mask_bht;
  idx_pht = PC & mask_pht;

  assert(idx_bht >= 0 && idx_bht <= 511);
  assert(idx_pht >= 0 && idx_pht <= 7);
  assert(bht[idx_bht] >= 0 && bht[idx_bht] <= 63);

  if (resolveDir == TAKEN && pht[bht[idx_bht]][idx_pht] != STRONGLY_TAKEN) 
    pht[bht[idx_bht]][idx_pht]++;
  else if (resolveDir == NOT_TAKEN && pht[bht[idx_bht]][idx_pht] != STRONGLY_NOT_TAKEN) 
    pht[bht[idx_bht]][idx_pht]--;

  bht[idx_bht] <<= 1;
  if (resolveDir == TAKEN)
    bht[idx_bht]++;
  bht[idx_bht] &= 0b111111;

  assert(bht[idx_bht] >= 0 && bht[idx_bht] <= 63);
}

/////////////////////////////////////////////////////////////
// openend
/////////////////////////////////////////////////////////////

int global; // Global history
int tbs[4096]; // two bit saturating counter
int b[512]; // BHT
int p[4096][8]; // PHT
int selector[4096]; // Selector that selects whether to use gshare or two bit saturating counter
void InitPredictor_openend() {
  int i, j;
  global = 0;
  for (i = 0; i < 4096; i++)
    tbs[i] = 0;
  for (i = 0; i < 4096; i++)
    for (j = 0; j < 8; j++)
      p[i][j] = WEAKLY_NOT_TAKEN;
  for (i = 0; i < 4096; i++)
    selector[i] = 1;
}

bool GetPrediction_openend(UINT32 PC) {
  int tbsidx, tbspred;
  tbsidx = (PC & 0b111111111111) ^ global;
  if (tbs[tbsidx] == STRONGLY_TAKEN || tbs[tbsidx] == WEAKLY_TAKEN)
    tbspred = TAKEN;
  if (tbs[tbsidx] == STRONGLY_NOT_TAKEN || tbs[tbsidx] == WEAKLY_NOT_TAKEN)
    tbspred = NOT_TAKEN;

  int bhtidx, phtidx, pred;
  bhtidx = (PC >> 3) & 0b111111111;
  phtidx = PC & 0b111;
  if (p[b[bhtidx]][phtidx] == STRONGLY_TAKEN || p[b[bhtidx]][phtidx] == WEAKLY_TAKEN)
    pred = TAKEN;
  else if (p[b[bhtidx]][phtidx] == STRONGLY_NOT_TAKEN || p[b[bhtidx]][phtidx] == WEAKLY_NOT_TAKEN)
    pred = NOT_TAKEN;

  int ret;
  if (tbspred == pred)
    ret = pred;
  else {
    if (selector[tbsidx] == 0 || selector[tbsidx] == 1)
      ret = tbspred; // global
    else if (selector[tbsidx] == 2 || selector[tbsidx] == 3)
      ret = pred; // private
  }
  return ret;
}

void UpdatePredictor_openend(UINT32 PC, bool resolveDir, bool predDir, UINT32 branchTarget) {
  int tbsidx, tbspred;
  tbsidx = (PC & 0b111111111111) ^ global;
  if (tbs[tbsidx] == STRONGLY_TAKEN || tbs[tbsidx] == WEAKLY_TAKEN)
    tbspred = TAKEN;
  if (tbs[tbsidx] == STRONGLY_NOT_TAKEN || tbs[tbsidx] == WEAKLY_NOT_TAKEN)
    tbspred = NOT_TAKEN;

  int bhtidx, phtidx, pred;
  bhtidx = (PC >> 3) & 0b111111111;
  phtidx = PC & 0b111;
  if (p[b[bhtidx]][phtidx] == STRONGLY_TAKEN || p[b[bhtidx]][phtidx] == WEAKLY_TAKEN)
    pred = TAKEN;
  else if (p[b[bhtidx]][phtidx] == STRONGLY_NOT_TAKEN || p[b[bhtidx]][phtidx] == WEAKLY_NOT_TAKEN)
    pred = NOT_TAKEN;

  if (tbspred != pred) {
    if (tbspred == resolveDir && selector[tbsidx] != 0)
      selector[tbsidx]--;
    if (pred == resolveDir && selector[tbsidx] != 3)
      selector[tbsidx]++;
  }

  if (resolveDir == TAKEN && tbs[tbsidx] != STRONGLY_TAKEN)
    tbs[tbsidx]++;
  else if (resolveDir == NOT_TAKEN && tbs[tbsidx] != STRONGLY_NOT_TAKEN)
    tbs[tbsidx]--;

  global <<= 1;
  global &= 0b111111111111;
  if (resolveDir == TAKEN)
    global++;

  if (resolveDir == TAKEN && p[b[bhtidx]][phtidx] != STRONGLY_TAKEN)
    p[b[bhtidx]][phtidx]++;
  if (resolveDir == NOT_TAKEN && p[b[bhtidx]][phtidx] != STRONGLY_NOT_TAKEN)
    p[b[bhtidx]][phtidx]--;

  b[bhtidx] <<= 1;
  b[bhtidx] &= 0b111111111111;
  if (resolveDir == TAKEN)
    b[bhtidx]++;


}
