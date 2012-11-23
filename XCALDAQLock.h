////////////////////////////////////////////////////////////////////////////////
/*
 description:
    a simple OS independent thread class

 revision history:
    2007-02-02 - submitted to cvs
    2007-02-01 - first compiled version

 */

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#ifndef __XCALDAQLock_h
#define __XCALDAQLock_h
////////////////////////////////////////////////////////////////////////////////

#ifndef WIN32
#include <pthread.h>
#else
#include <windows.h>
#endif

/**
 description:
 a simple OS independent thread class
*/
class XCALDAQLock
{
    public:
        XCALDAQLock()
        {
        #ifdef WIN32
            //    m_Lock = CreateMutex(NULL,FALSE,NULL);
            InitializeCriticalSection(&m_Lock);
        #else
            pthread_mutex_init(&m_Lock,NULL);
        #endif
        }

        void getLock()
        {
        #ifdef WIN32
            //    if (WaitForSingleObject(m_Lock,INFINITE) != WAIT_OBJECT_0) {
            //		throw std::runtime_error("Mutex acquisition failure.");
            //	}
            EnterCriticalSection(&m_Lock);
        #else
            pthread_mutex_lock(&m_Lock);
        #endif
        }

        int tryLock()
        {
        #ifdef WIN32
        #else
            return pthread_mutex_trylock(&m_Lock);
        #endif
        }

        void releaseLock()
        {
        #ifdef WIN32
            //	  if (!ReleaseMutex(m_Lock)) {
            //		 // throw std::runtime_error("Mutex release failure.");
            //	  }
            LeaveCriticalSection(&m_Lock);
        #else
            pthread_mutex_unlock(&m_Lock);
        #endif
        }

    private:

    #ifdef WIN32
        //HANDLE m_Lock;
        CRITICAL_SECTION m_Lock;
    #else
        pthread_mutex_t m_Lock;
    #endif

};

////////////////////////////////////////////////////////////////////////////////
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
