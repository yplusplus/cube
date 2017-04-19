#include <assert.h>
#include <stddef.h>

#include "shm_util.h"

namespace cube {

Shmem::Shmem() 
    : m_initialized(false),
    m_shm_ptr((void *)-1),
    m_ownner(false),
    m_create(false),
    m_shm_size(0),
    m_shm_key(0),
    m_shm_id(-1) {
}

Shmem::~Shmem() {
    if (m_initialized && m_ownner) {
        Detach();
    }
}

int Shmem::Init(key_t key, size_t size, bool owner /* = false */) {
    // make sure not initialize twice
    assert(!m_initialized);

    // IPC_CREAT   to  create a new segment.  If this flag is not used, then shmget() will
    //             find the segment associated with key and check to see if the  user  has
    //             permission to access the segment.
    //
    // IPC_EXCL    used with IPC_CREAT to ensure failure if the segment already exists.
    
    m_shm_id = shmget(key, size, IPC_CREAT | IPC_EXCL | 0644); 
    if (m_shm_id < 0) {
        m_create = false;

        m_shm_id = shmget(key, size, 0644);
        if (m_shm_id < 0)
            return RET_SHM_GET_FAIL;
    } else {
        m_create = true;
    }

    m_shm_ptr = shmat(m_shm_id, NULL, 0);
    if (m_shm_ptr == (void *)-1) {
        return RET_SHM_AT_FAIL;
    }

    m_initialized = true;
    m_ownner = owner;
    m_shm_key = key;
    m_shm_size = size;
    return 0;
}

int Shmem::Detach() {

    int ret = 0;
    if (m_shm_ptr != (void *)-1) {
        ret = shmdt(m_shm_ptr);
        m_shm_ptr = (void *)-1;
    }

    return ret;
}

}
