#include <omp.h>
#include "linkedlist_parallel.h"

LinkedlistPara_t::LinkedlistPara_t(int ndiv, PositionData_t *data, HBTReal boxsize, bool periodic)
{
  #pragma omp parallel
  {
    int thread_id=omp_get_thread_num();
    int thread_num=omp_get_num_threads();
    #pragma omp single
    {
      LLs.resize(thread_num);
      Samples.resize(thread_num);
    }
    Samples[thread_id].init(thread_id, thread_num, data);
    LLs[thread_id].build(ndiv, &(Samples[thread_id]), boxsize, periodic);
  }
}
void LinkedlistPara_t::SearchSphere(HBTReal radius, const HBTxyz &searchcenter, vector <HBTInt> &found_ids, int nmax_guess, HBTReal rmin)
{//parallel version. not suitable for use inside another parallel region.
  found_ids.clear();
#pragma omp parallel for
  for(int thread_id=0;thread_id<LLs.size();thread_id++)
  {
    vector <HBTInt> thread_founds;
    LLs[thread_id].SearchSphere(radius, searchcenter, thread_founds, nmax_guess, rmin);
    #pragma omp critical(insert_linklist_founds) //this prevents nested parallelization
    {
      found_ids.insert(found_ids.end(), thread_founds.begin(), thread_founds.end());
    }
  }
}
void LinkedlistPara_t::SearchSphereSerial(HBTReal radius, const HBTxyz &searchcenter, vector <HBTInt> &found_ids, int nmax_guess, HBTReal rmin)
{//serial version, which can be safely run inside another parallel region
  found_ids.clear();
  for(int thread_id=0;thread_id<LLs.size();thread_id++)
  {
    vector <HBTInt> thread_founds;
    LLs[thread_id].SearchSphere(radius, searchcenter, thread_founds, nmax_guess, rmin);
    found_ids.insert(found_ids.end(), thread_founds.begin(), thread_founds.end());
  }
}
