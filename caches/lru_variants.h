#ifndef LRU_VARIANTS_H
#define LRU_VARIANTS_H

#include <unordered_map>
#include <list>
#include <random>
#include "cache.h"
#include "cache_object.h"
#include "adaptsize_const.h" /* AdaptSize constants */



typedef std::list<CacheObject>::iterator ListIteratorType;
typedef std::unordered_map<CacheObject, ListIteratorType> lruCacheMapType;

/*
  LRU: Least Recently Used eviction
*/
class LRUCache : public Cache
{
protected:
    // list for recency order
    // std::list is a container, usually, implemented as a doubly-linked list 
    std::list<CacheObject> _cacheList;
    // map to find objects in list
    lruCacheMapType _cacheMap;

    virtual void hit(lruCacheMapType::const_iterator it, uint64_t size);

public:
    LRUCache()
        : Cache()
    {
    }
    virtual ~LRUCache()
    {
    }

    virtual bool lookup(SimpleRequest* req);
    virtual void admit(SimpleRequest* req);
    virtual void evict(SimpleRequest* req);
    virtual void evict();
    virtual SimpleRequest* evict_return();

};

static Factory<LRUCache> factoryLRU("LRU");


/*
  FIFO: First-In First-Out eviction
*/
class FIFOCache : public LRUCache
{
protected:
    virtual void hit(lruCacheMapType::const_iterator it, uint64_t size);

public:
    FIFOCache()
        : LRUCache()
    {
    }
    virtual ~FIFOCache()
    {
    }
};

static Factory<FIFOCache> factoryFIFO("FIFO");

/*
  FilterCache (admit only after N requests)
*/
class FilterCache : public LRUCache
{
protected:
    uint64_t _nParam;
    std::unordered_map<CacheObject, uint64_t> _filter;

public:
    FilterCache();
    virtual ~FilterCache()
    {
    }

    virtual void setPar(std::string parName, std::string parValue);
    virtual bool lookup(SimpleRequest* req);
    virtual void admit(SimpleRequest* req);
};

static Factory<FilterCache> factoryFilter("Filter");

/*
  ThLRU: LRU eviction with a size admission threshold
*/
class ThLRUCache : public LRUCache
{
protected:
    uint64_t _sizeThreshold;

public:
    ThLRUCache();
    virtual ~ThLRUCache()
    {
    }

    virtual void setPar(std::string parName, std::string parValue);
    virtual void admit(SimpleRequest* req);
};

static Factory<ThLRUCache> factoryThLRU("ThLRU");

/*
  ExpLRU: LRU eviction with size-aware probabilistic cache admission
*/
class ExpLRUCache : public LRUCache
{
protected:
    double _cParam;

public:
    ExpLRUCache();
    virtual ~ExpLRUCache()
    {
    }

    virtual void setPar(std::string parName, std::string parValue);
    virtual void admit(SimpleRequest* req);
};

static Factory<ExpLRUCache> factoryExpLRU("ExpLRU");

/*
  AdaptSize: ExpLRU with automatic adaption of the _cParam
*/
class AdaptSizeCache : public LRUCache
{
public: 
    AdaptSizeCache();
    virtual ~AdaptSizeCache()
    {
    }

    virtual void setPar(std::string parName, std::string parValue);
    virtual bool lookup(SimpleRequest*);
    virtual void admit(SimpleRequest*);

private: 
    double _cParam; //
    uint64_t statSize;
    uint64_t _maxIterations;
    uint64_t _reconfiguration_interval;
    uint64_t _nextReconfiguration;
    double _gss_v;  // golden section search book parameters
    // for random number generation 
    std::uniform_real_distribution<double> _uniform_real_distribution = 
        std::uniform_real_distribution<double>(0.0, 1.0); 

    struct ObjInfo {
        double requestCount; // requestRate in adaptsize_stub.h
        uint64_t objSize;

        ObjInfo() : requestCount(0.0), objSize(0) { }
    };
    std::unordered_map<CacheObject, ObjInfo> _longTermMetadata;
    std::unordered_map<CacheObject, ObjInfo> _intervalMetadata;

    void reconfigure();
    double modelHitRate(double c);

    // align data for vectorization
    std::vector<double> _alignedReqCount;
    std::vector<double> _alignedObjSize;
    std::vector<double> _alignedAdmProb;
};

static Factory<AdaptSizeCache> factoryAdaptSize("AdaptSize");

/*
  S4LRU

  enter at segment 0
  if hit on segment i, segment i+1
  if evicted on segment i, segment i-1

*/
class S4LRUCache : public Cache
{
protected:
    LRUCache segments[4];

public:
    S4LRUCache()
        : Cache()
    {
        segments[0] = LRUCache();
        segments[1] = LRUCache();
        segments[2] = LRUCache();
        segments[3] = LRUCache();
    }
    virtual ~S4LRUCache()
    {
    }

    virtual void setSize(uint64_t cs);
    virtual bool lookup(SimpleRequest* req);
    virtual void admit(SimpleRequest* req);
    virtual void segment_admit(uint8_t idx, SimpleRequest* req);
    virtual void evict(SimpleRequest* req);
    virtual void evict();
};

static Factory<S4LRUCache> factoryS4LRU("S4LRU");





/*
  TinyLFU (Basic LRU-based version) -> "Uses LRU for main cache and TinyLFU algorithm for victim handling"
*/
#include "countmin.h"
class  TinyLFU : public LRUCache
{
protected:
    // TODO

    // virtual void hit(lruCacheMapType::const_iterator it, uint64_t size);
    // do we need a simillar function to update our statistics on hit ?
    void update_tiny_lfu(long long id);




    // add data structures for the TinyLFU algorithm 



    // add the cm sketch
    CM_type *cm_sketch;


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

        // We have this from Cache() 
         //: _cacheSize -> setSize(cacheSize) from the script
         //_currentSize
    }
    
    virtual ~TinyLFU()
    {
      delete cm_sketch;
    }

    
    virtual void setSize(uint64_t cs) {
    
    
        //std::cout << "CM_Init" << std::endl;
        cm_sketch = CM_Init(cs / 4, 4, 319062105);
        //std::cout << "CM_init  " << cm_sketch->depth <<" , " <<   cm_sketch->width << std::endl;
       // if (!cm_sketch)
       // std::cout << "CM fails" << std::endl;

    
        _cacheSize = cs;
        
        
    }
    bool lookup(SimpleRequest* req);
    // The one from LRU is enough

    virtual void admit(SimpleRequest* req);
    //


    //virtual void evict(SimpleRequest* req); // maybe we don't need this
    virtual bool evict(int cand_id);
    virtual SimpleRequest* evict_return(int cand_id);
    //Need to be updated to support TinyLFU algorithm comparison
};


static Factory<TinyLFU> factoryTinyLFU("TinyLFU");



#endif
