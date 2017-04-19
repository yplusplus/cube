#ifndef __CUBE_SHMEM_H__
#define __CUBE_SHMEM_H__

#include <sys/ipc.h>
#include <sys/shm.h>

namespace cube {

class Shmem {
    public:
        static const int RET_SHM_GET_FAIL = -1;
        static const int RET_SHM_AT_FAIL  = -2;

        Shmem();
        ~Shmem();

        int Init(key_t key, size_t size, bool owner = false);

        bool Owner() const { return m_ownner; }
        bool Create() const { return m_create; }
        size_t ShmSize() const { return m_shm_size; }
        key_t ShmKey() const { return m_shm_key; }
        void *ShmPtr() const { return m_shm_ptr; }

        int Detach();

    private:
        int Attach();

    private:
        bool m_initialized;

        void *m_shm_ptr;

        bool m_ownner;
        
        bool m_create;

        size_t m_shm_size;

        key_t m_shm_key;

        int m_shm_id;
};

}
#endif
