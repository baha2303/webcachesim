#include "tiny_lfu.h"


/*
  TinyLFU
*/

/*****HELPER FUNCTIONS******/

void TinyLFU::update_tiny_lfu(long long id) {
    CM_Update(cm_sketch, id, 1);

}



/*****CACHE FUNCTIONS******/
bool TinyLFU::lookup(SimpleRequest* req)
{
    // CacheObject: defined in cache_object.h 
    CacheObject obj(req);
    // _cacheMap defined in class LRUCache in lru_variants.h 
    auto it = _cacheMap.find(obj);
    if (it != _cacheMap.end()) {
        // log hit
        LOG("h", 0, obj.id, obj.size);
        update_tiny_lfu(obj.id);
        return true;
    }
    return false;
}

void TinyLFU::admit(SimpleRequest* req)
{
    const uint64_t size = req->getSize();
    // object feasible to store?
    if (size > _cacheSize) {
        LOG("L", _cacheSize, req->getId(), size);
        return;
    }
    // check eviction needed
    bool evicted;
    while (_currentSize + size > _cacheSize) {
        evicted=evict(req->getId());
        // TODO
        //which to evict ? how to evict ? how to compare between victim and candidate
    }

    // admit new object
    if (evicted) {
        CacheObject obj(req);
        _cacheList.push_front(obj);
        _cacheMap[obj] = _cacheList.begin();
        _currentSize += size;
        LOG("a", _currentSize, obj.id, obj.size);
        // TODO
        // Update the TinyLFU with the new object
        update_tiny_lfu(obj.id);

    }

}
/*
void TinyLFU::evict(SimpleRequest* req)
{
    CacheObject obj(req);
    auto it = _cacheMap.find(obj);
    if (it != _cacheMap.end()) {
        ListIteratorType lit = it->second;
        LOG("e", _currentSize, obj.id, obj.size);
        _currentSize -= obj.size;
        _cacheMap.erase(obj);
        _cacheList.erase(lit);
    }
}
*/

SimpleRequest* TinyLFU::evict_return(int cand_id)
{
    // evict least popular (i.e. last element)
    if (_cacheList.size() > 0) {
        ListIteratorType lit = _cacheList.end();
        lit--;
        CacheObject obj = *lit;
        LOG("e", _currentSize, obj.id, obj.size);
        SimpleRequest* req = new SimpleRequest(obj.id, obj.size);


        // TODO
        // should this by evicted ? use the CM Sketch to decide : compare the victim with the 

        int victim_freq_est = CM_PointEst(cm_sketch, obj.id);
        int candidate_freq_est = CM_PointEst(cm_sketch, obj.id);

        if (victim_freq_est > candidate_freq_est) {
            _currentSize -= obj.size;
            _cacheMap.erase(obj);
            _cacheList.erase(lit);
        }
        else {

            return NULL;
        }



        return req;
    }
    return NULL;
}

bool TinyLFU::evict(int cand_id)
{
    return (evict_return(cand_id) == NULL ? false : true);
}

