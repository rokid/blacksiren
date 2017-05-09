#include "legacy/AELearning.h"
#include <assert.h>
#include "legacy/r2math.h"

AELearning::AELearning(bool bD, unsigned int dMax, time_t dTimeOut, time_t tTimeout){
  
  sectors = R2_SAFE_NEW_AR1(sectors, AudioInputSector, SECTOR_NUM) ;
  
  this->bAffect = bD ;
  init(dMax, dTimeOut, tTimeout);
  
}

AELearning::~AELearning(void){
  
  R2_SAFE_DEL_AR1(sectors) ;
    
}


void AELearning::setFree(int index) {
  assert(index >= 0 && index < SECTOR_NUM) ;
  R2_MEM_ASSERT(this,0);
  
	sectors[index].dirtyTime = 0;
	sectors[index].dirty = 0;
	sectors[index].tryTime = 0;
}

void AELearning::setFree2(int index){
  
  assert(index >= 0 && index < SECTOR_NUM) ;
  R2_MEM_ASSERT(this,0);
  
  for (int i = - SECTOR_TOR ; i <= SECTOR_TOR; i ++) {
    int subindex = index + i ;
    if (subindex < 0) {
      subindex += SECTOR_NUM ;
    }
    if (subindex >= SECTOR_NUM) {
      subindex -= SECTOR_NUM ;
    }
    setFree(subindex) ;
  }
  
}

void AELearning::init(unsigned int dMax, time_t dTimeOut, time_t tTimeOut) {
  
	dirtyMax = dMax;
	dirtyTimeOut = dTimeOut;
	tryTimeOut = tTimeOut;
}

int AELearning::getIndexByDegree(int degree) {
  
  assert(degree >= 0 && degree <= 360);
  
	int index = degree / (360 / SECTOR_NUM);

	if(index >= SECTOR_NUM)
		index = 0;

	return index;
}

bool AELearning::check(int index) {
  
  if (!bAffect) {
    return  true ;
  }
  
  R2_MEM_ASSERT(this,0);
  assert(index >= 0 && index < SECTOR_NUM) ;
  
	if(index >= SECTOR_NUM) {
		return false; // invalid sector index
	}

	//check guity or not
	if(sectors[index].dirty < dirtyMax) { // not dirty enough to be guity
		setFree2(index);
		return true;
	}

	time_t now = time(NULL);
	if((now - sectors[index].dirtyTime) >= dirtyTimeOut) { //dirty time out
		setFree2(index);
		return true;
	}

	if((now - sectors[index].tryTime) < tryTimeOut) { //second time try to set free.
		setFree2(index);
		return true;
	}

	//retry time out
  for (int i = - SECTOR_TOR ; i <= SECTOR_TOR; i ++) {
    int subindex = index + i ;
    if (subindex < 0) {
      subindex += SECTOR_NUM ;
    }
    if (subindex >= SECTOR_NUM) {
      subindex -= SECTOR_NUM ;
    }
    sectors[subindex].tryTime = now;
  }
	
	return false;
}


void AELearning::dirty(int index) {
  
  
  R2_MEM_ASSERT(this,0);
  assert(index >= 0 && index < SECTOR_NUM) ;
  
	if(index >= SECTOR_NUM) {
		return; // invalid sector index
	}

	time_t now = time(NULL);

	if((now - sectors[index].dirtyTime) >= dirtyTimeOut) { //dirty time out
		setFree(index);
	}

	sectors[index].dirty ++;
	sectors[index].dirtyTime = now;
}

void AELearning::dirty2(int index){
  
  assert(index >= 0 && index < SECTOR_NUM) ;
  R2_MEM_ASSERT(this,0);
  
  for (int i = -SECTOR_TOR ; i <= SECTOR_TOR; i ++) {
    int subindex = index + i ;
    if (subindex < 0) {
      subindex += SECTOR_NUM ;
    }
    if (subindex >= SECTOR_NUM) {
      subindex -= SECTOR_NUM ;
    }
    dirty(subindex) ;
  }
  
}

void AELearning::reset() {
	for(int i=0; i<SECTOR_NUM; ++i) {
		setFree(i);
	}
}
