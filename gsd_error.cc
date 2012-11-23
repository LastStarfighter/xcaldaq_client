/********************************************************************
 *
 *
 * Name: gsd_error.cc
 *
 *
 * Author: Joseph Adams
 *
 * Description:
 *  stupid error functions
 *
 * --------------------------------------------------------------
 *    $Revision: 1.13 $
 * ---------------------------------------------------------------
 *
 *
 *
 **********************************************************************/

#include "gsd_error.h"
#include "string_utils.h"

#include <string.h>
#include <errno.h>

#if defined(_MSC_VER)
//#define DllImport   __declspec( dllimport )
// #define DllExport   __declspec( dllexport )

//  DllImport extern int errno;
//#define errno 0
#else
extern int errno;
#endif

// gsd_error handling--crash&burn
namespace gsd
{

    // "private" methods
    const char * get_error_string(int error);
    int get_last_error(void);
    int info_base(const char preamble[], const char msg[],const char method[]="");
    /*
      void gsd_error(const char error_text[])
      {
          fprintf(stderr,"\nERROR: %s\n\n",error_text);
          fflush(stderr);
          exit(1);
      }

      void gsd_error(const char method_name[],const char error_text[])
      {
          fprintf(stderr,"\nERROR in %s",method_name);
    fprintf(stderr,"\n%s\n\n",error_text);
    fflush(stderr);
    exit(1);
    }

    void gsd_warning(const char error_text[])
    {
    fprintf(stderr,"\nWARNING: %s",error_text);
    fflush(stderr);
    return;
    }

    void gsd_warning(const char method_name[],const char error_text[])
    {
    fprintf(stderr,"\nWarning in %s",method_name);
    fprintf(stderr,"\n%s",error_text);
    fflush(stderr);
    exit(1);
    }

    */

    //! generic error handler, calls exit
    int error(const char method[],const char msg[])
    {
        int result;
        result=info_base("(gsd::error says)\nError",msg,method);

        fprintf(stdout,"\nExiting with error code %d\n\n",result);
        exit(result);
    }

    //! generic error handler, calls exit
    //!
    int error(const char msg[])
    {
        int result;
        result=info_base("(gsd::error says)\nError",msg);
        fprintf(stdout,"\nExiting with error code %d\n\n",result);
        exit(result);
    }

    int warning(const std::string& method,const std::string& msg)
    {
        return warning(method.c_str(),msg.c_str());

    }

    int warning(const std::string& msg)
    {
        return warning(msg.c_str());

    }

    //! generic warning handler
    int warning(const char method[],const char msg[])
    {
        return info_base("(gsd::warning says)\nWarning",msg,method);

    }

    //! generic warning handler
    int warning(const char msg[])
    {
        return info_base("(gsd::warning says)\nWarning",msg);

    }

    //! generic info handler
    int info(const char method[],const char msg[])
    {
        return info_base("(gsd::info says)\nInfo",msg,method);

    }

    //! generic info handler
    int info(const char msg[])
    {
        return info_base("(gsd::info says)\nInfo",msg);

    }

    //! generic info handler base class.
    //!
    //! Not for external use
    int info_base(const char preamble[], const char msg[],const char method[])
    {
        //fprintf(stderr,"\n%: %d\n", s, get_last_error())

        int err_no;
        char err_msg[2048];

        err_no=get_last_error();

        if (strlen(method))
        {
            sprintf(err_msg,"\n%s in method %s:",preamble,method);
        }

        else
        {
            sprintf(err_msg,"\n%s:",preamble);
        }

        if (strlen(msg))
        {
            sprintf(err_msg,"%s\n%s",err_msg,msg);
        }

        err_no=0;
        if (err_no)
        {
            sprintf(err_msg,"%s\nerr_no:%d: %s",err_msg,err_no,get_error_string(err_no));

        }

        gsd::str::hline();
        fprintf(stdout,"\n%s\n",err_msg);
        gsd::str::hline();
        fflush(stdout);

        return err_no;

    }

    const char * get_error_string(int error)
    {

        int err=get_last_error();
        return strerror(err);

        // return "This function needs work";

    }

    int get_last_error(void)
    {

        return errno;

    }

}
