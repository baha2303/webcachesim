#ifndef LRU_VARIANTS_H
#define LRU_VARIANTS_H

#include <unordered_map>
#include <list>
#include <random>
#include "cache.h"
#include "cache_object.h"
#include "adaptsize_const.h" /* AdaptSize constants */
#include "caches/sketch/countmin.h"


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

class  TinyLFU : public LRUCache
{
protected:
    CM_type *cm_sketch;
    void update_tiny_lfu(long long id);

public:
    TinyLFU() : LRUCache() {}
    
    virtual ~TinyLFU()
    {
      CM_Destroy(cm_sketch);
    }

    virtual void setSize(uint64_t cs) {
        //std::cout << "CM_Init" << std::endl;
        cm_sketch = CM_Init(cs/2, 2, 1033096058);
        _cacheSize = cs;
  
    }

    bool lookup(SimpleRequest* req);
    virtual void admit(SimpleRequest* req);
    //virtual void evict(SimpleRequest* req); // maybe we don't need this
    virtual bool evict(int cand_id);
    virtual SimpleRequest* evict_return(int cand_id);
    //Need to be updated to support TinyLFU algorithm comparison
};


static Factory<TinyLFU> factoryTinyLFU("TinyLFU");

/*
    SLRU
*/
class SLRUCache : public Cache
{
protected:
    LRUCache segments[2];

public:
    CM_type *cm_sketch;
    Door_keeper * dk;
    SLRUCache()
        : Cache()   
    {
        segments[0] = LRUCache();
        segments[1] = LRUCache();

    }

    virtual ~SLRUCache()
    {
        Door_keeper_Destroy(dk);
        CM_Destroy(cm_sketch);
    }

    virtual void setSize(uint64_t cs);
    virtual bool lookup(SimpleRequest* req);
    virtual void admit(SimpleRequest* req);
    virtual void segment_admit(uint8_t idx, SimpleRequest* req);
    virtual void evict(SimpleRequest* req);
    virtual void evict();
    void admit_from_window(SimpleRequest* req);
    void update_cm_sketch(long long id);
    void update_door_keeper(long long id) ;
    int search_door_keeper(long long id);
    SimpleRequest* evict_return(int);
    int getCurrentSegmentSize(int seg);
    int getSegmentSize(int seg);
    void initDoor_initCM(uint64_t cs);
};

static Factory<SLRUCache> factorySLRU("SLRU");

/*
  W-TinyLFU 
 W-TinyLFU Cache Policy. uses SLRU for main cache and LRU window to maintain freshness

     
*/
class LRU : public LRUCache {

    public:
    LRU() : LRUCache() {

    }
    virtual ~LRU() {}
    //virtual void setSize(uint64_t cs);
     //virtual bool lookup(SimpleRequest* req) ;
    // virtual void admit(SimpleRequest* req);
    // virtual void evict(SimpleRequest* req) {}
    // virtual void evict() {}
    // virtual SimpleRequest* evict_return() {}
    std::list<SimpleRequest*> admit_with_return(SimpleRequest* req);

};

class  W_TinyLFU : public Cache
{
protected:

    SLRUCache main_cache;       // TinyLFU-CM-Sketch implemented in the SLRU
    LRU window;
    uint64_t window_size_p;     // the percentage of the window of all cache size [0-100]
    uint64_t reqs,hits;         // for the hillClimber algorithm
    double prev_hit_ratio;
public:
    W_TinyLFU() : window_size_p(0.01),Cache(),main_cache(),window()
    {
       // window=LRU();
       // =SLRUCache();
        //window_size_p=0.2;
        // We have this from Cache() 
         //: _cacheSize -> setSize(cacheSize) from the script
         //_currentSize
         reqs=0;
         hits=0;
         prev_hit_ratio=0;
    }
    
    virtual ~W_TinyLFU()
    {
    }

    bool lookup(SimpleRequest* req);
    virtual void admit(SimpleRequest* req);
    //virtual void evict(SimpleRequest* req); // maybe we don't need this
    //Need to be updated to support TinyLFU algorithm comparison
    virtual void setPar(std::string parName, std::string parValue);
    virtual void evict(SimpleRequest* req);
    virtual void evict();
    void hillClimber(int reqs, int hits );
    void increaseWindow();
    void increaseMainCache();
};

static Factory<W_TinyLFU> factoryW_TinyLFU("W_TinyLFU");

#endif
