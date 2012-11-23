////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include "XOUT.h"
#include <string.h>
////////////////////////////////////////////////////////////////////////////////

namespace xdaq
{
    /////////////////////////////////////////////////////////////////////////////
    XOUT::XOUT()
    {
        m_fileName = std::string("");
        m_dataWriterFD = NULL;
        m_lock = XCALDAQLock();
        m_recordMax = 0;
        m_recordWritten = 0;

        m_writingFlag = false;

        m_headerFlag = false;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XOUT::closeXOUT()
    {
        m_writingFlag = false;

        if (m_dataWriterFD != NULL)
        {

            // get lock
            m_lock.getLock();

            bool removeFlag = false;

            if (ftell(m_dataWriterFD) == 0)
            {
                //They didn't actually write anything to the file.  So delete it.
                removeFlag = true;
                //std::cout << "this file is empty.. kill it" << std::endl;
            }

            std::cout << "Closing output file." << std::endl;
            fclose(m_dataWriterFD);
            m_dataWriterFD = NULL;

            if (removeFlag)
            {
                remove(m_filePath.c_str());
            }

            // release the lock
            m_lock.releaseLock();

        }

        m_headerFlag = false;
        m_fileName = "";
        m_filePath = "";
        return;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XOUT::openXOUT(char *filename,char *mode)
    {
        // copy the mode local
        memcpy (&m_mode[0], mode, sizeof(*mode));

        // set the full path name
        m_filePath = filename;

        printf ("full path: %s\n",filename);

        // get file name out of full path
        m_fileName = fl_filename_name(filename);

        // open the file
        m_dataWriterFD = fopen(filename,m_mode);

        // reset the recs number
        m_recordWritten = 0;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XOUT::headerFlag(bool b)
    {
        m_headerFlag = b;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool XOUT::headerFlag(void)
    {
        return m_headerFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XOUT::writingFlag(bool b)
    {
        m_writingFlag = b;
        std::cout << "File Writing turned ";
        if (b) std::cout << "on" << std::endl;
        else std::cout << "off" << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    bool XOUT::writingFlag(void)
    {
        return m_writingFlag;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XOUT::recordWritten(int d)
    {
        m_recordWritten = d;
        std::cout << "File Writing turned " << d << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    int XOUT::recordWritten(void)
    {
        return m_recordWritten;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XOUT::recordMax(int d)
    {
        if (d<0)
        {
            printf ("bad record max... setting to 0\n");
            d=0;
        }
        m_recordMax = d;
        std::cout << "Max records set to " << d << std::endl;
    }

    /////////////////////////////////////////////////////////////////////////////
    int XOUT::recordMax(void)
    {
        return m_recordMax;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XOUT::fileName(std::string s)
    {
        m_fileName = s;
    }

    /////////////////////////////////////////////////////////////////////////////
    std::string XOUT::fileName(void)
    {
        return m_fileName;
    }

    /////////////////////////////////////////////////////////////////////////////
    void XOUT::filePath(std::string s)
    {
        m_filePath = s;
    }

    /////////////////////////////////////////////////////////////////////////////
    std::string XOUT::filePath(void)
    {
        return m_filePath;
    }

    /////////////////////////////////////////////////////////////////////////////
    FILE* XOUT::dataWriterFD(void)
    {
        return m_dataWriterFD;
    }

}                                                 // end of the namespace


////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
