/********************************************************************
 *
 *
 * Name: gsd_error.h
 *
 *
 * Author: Joseph Adams
 *
 * Description:
 *  stupid error functions
 *
 * --------------------------------------------------------------
 *    $Revision: 1.7 $
 * ---------------------------------------------------------------
 *
 *
 *
 **********************************************************************/
/*! \file gsd_error.h

\brief Error handling routines

\defgroup gsd_error
*/

#ifndef _GSD_ERROR_
#define _GSD_ERROR_

#include <stdlib.h>
#include <stdio.h>
#include <string>
namespace gsd
{
    /*
      void gsd_error(const char error_text[]);
      void gsd_error(const char method_name[],const char error_text[]);
      void gsd_warning(const char error_text[]);
      void gsd_warning(const char method_name[],const char error_text[]);
    */

    /**  @name Error Reoprting
     *  Functions for error reporting
     */
    //@{

    //    int error(const char method[],const char msg[]);
    //    int warning(const std::string& method,const std::string& msg)
    // { return warning(method.c_str(), msg.c_str()); }
    //  int info(const char method[],const char msg[]);

    int warning(const std::string& method,const std::string& msg);
    int warning(const std::string& msg);

    int error(const char method[],const char msg[]);
    int warning(const char method[],const char msg[]);
    int info(const char method[],const char msg[]);

    int error(const char msg[]);
    int warning(const char msg[]);
    int info(const char msg[]);
    //@}
}
#endif
