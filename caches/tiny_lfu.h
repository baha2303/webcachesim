#include "lru_variants.h"
#include "countmin.h"


/*
  TinyLFU: Uses LRU for main cache and TinyLFU algorithm for victim handling
*/
class  TinyLFU : public LRUCache
{
protected:
    // TODO

    // virtual void hit(lruCacheMapType::const_iterator it, uint64_t size);
    // do we need a simillar function to update our statistics on hit ?
    void update_tiny_lfu(long long id);




    // add data structures for the TinyLFU algorithm 



    // add the cm sketch
    CM_type* cm_sketch;


    /*
    Available CMSketch functions
    extern void CM_Update(CM_type*, unsigned int, int);
    extern int CM_PointEst(CM_type*, unsigned int);
    extern int CM_PointMed(CM_type*, unsigned int);
    extern int CM_InnerProd(CM_type*, CM_type*);
    extern int CM_Residue(CM_type*, unsigned int*);
    */


public:
    TinyLFU()
        : LRUCache()
    {
        cm_sketch = CM_Init(_cacheSize/4, 4, 319062105);

           // We have this from Cache() 
            //: _cacheSize -> setSize(cacheSize) from the script
            //_currentSize
    }
    virtual ~TinyLFU()    
    {
    }

     bool lookup(SimpleRequest* req);
    // The one from LRU is enough
    
    virtual void admit(SimpleRequest* req);
    //


    virtual void evict(SimpleRequest* req); // maybe we don't need this
    virtual bool evict(int cand_id);
    virtual SimpleRequest* evict_return(int cand_id);
    //Need to be updated to support TinyLFU algorithm comparison


};


static Factory<TinyLFU> factoryTinyLFU("TinyLFU");

