/********************************************************************
 *
 *
 * Name: string_utils.h
 *
 *
 * Author: Joseph Adams
 *
 * Description:
 *   utils for handling stl strings (hopefully perl style)
 *
 * --------------------------------------------------------------
 *    $Revision: 1.38 $
 * ---------------------------------------------------------------
 *    $Log: string_utils.h,v $
 *    Revision 1.38  2006-06-01 16:22:04  jadams
 *    Added a singel character version of ascii dump control characters
 *
 *    Revision 1.37  2006-01-24 18:53:46  jadams
 *    *** empty log message ***
 *
 *    Revision 1.36  2005/11/08 23:46:27  jadams
 *    Added a remove excess whitespace function
 *
 *    Revision 1.35  2005/10/04 19:38:27  jadams
 *    *** empty log message ***
 *
 *    Revision 1.34  2005/05/26 14:10:45  jadams
 *    *** empty log message ***
 *
 *    Revision 1.33  2005/05/26 01:06:08  jadams
 *    *** empty log message ***
 *
 *    Revision 1.32  2005/04/21 15:54:50  jadams
 *    *** empty log message ***
 *
 *    Revision 1.31  2005/04/18 21:25:04  jadams
 *    *** empty log message ***
 *
 *    Revision 1.30  2004/12/21 20:48:10  jadams
 *    Numerous changes to support gcc 3.4
 *
 *    Revision 1.29  2004/12/17 14:52:20  jadams
 *    *** empty log message ***
 *
 *    Revision 1.28  2004/11/23 16:08:45  jadams
 *    gsd_endian routines added
 *
 *    Revision 1.27  2004/10/20 20:32:37  jadams
 *    renamed all JSA sentinels to GSD
 *
 *    Revision 1.26  2004/10/14 20:41:31  jadams
 *    *** empty log message ***
 *
 *    Revision 1.25  2004/10/14 17:45:57  jadams
 *    *** empty log message ***
 *
 *    Revision 1.24  2004/10/14 13:53:03  jadams
 *    Added explicit int cast for hex mode on string_to_val(T,fmt)
 *
 *    Revision 1.23  2004/10/13 18:34:38  jadams
 *    Support for format via a sprintf hack
 *
 *    Revision 1.22  2004/07/14 13:20:59  jadams
 *    Circular vector is now working
 *
 *    Revision 1.21  2004/06/29 19:37:57  jadams
 *    *** empty log message ***
 *
 *    Revision 1.20  2004/03/24 17:44:09  jadams
 *    Added conversion functions and uu_string
 *
 *    Revision 1.19  2004/03/16 22:32:53  jadams
 *    Added cubic root solver to gsd::math seems to work ok
 *
 *    Revision 1.18  2004/03/10 14:34:55  jadams
 *    Added hex dump for stl strings to gsd::str
 *
 *    Revision 1.17  2004/03/08 23:49:28  jadams
 *    *** empty log message ***
 *
 *    Revision 1.16  2004/02/20 15:21:16  jadams
 *    Added some non-null terminated functions to gsd
 *
 *    Revision 1.15  2004/02/19 17:15:23  jadams
 *    Added pretty print number to gsd::str for putting commas in numbers
 *
 *    Revision 1.14  2004/01/30 23:48:32  jadams
 *    *** empty log message ***
 *
 *    Revision 1.13  2004/01/09 09:20:40  jadams
 *    added binary string printer to string utils
 *
 *    Revision 1.12  2003/12/02 22:01:20  jadams
 *    added get_path etc
 *
 *    Revision 1.11  2003/11/21 19:58:53  jadams
 *    fixed bug in split with tokens on first char, and empty tokens
 *
 *    Revision 1.10  2003/11/20 23:07:14  jadams
 *    *** empty log message ***
 *
 *    Revision 1.9  2003/11/20 20:37:53  jadams
 *    *** empty log message ***
 *
 *    Revision 1.8  2003/11/20 19:10:32  jadams
 *    added convert_whitespace_to_delimeter, search_and_replace_all, and toupper and tolower methods
 *
 *    Revision 1.7  2003/11/20 17:19:51  jadams
 *    *** empty log message ***
 *
 *    Revision 1.6  2003/10/10 19:27:40  jadams
 *    *** empty log message ***
 *
 *    Revision 1.5  2003/10/10 18:40:50  jadams
 *    added precision to any_ref, to_string and labeled_array
 *
 *    Revision 1.4  2003/10/10 17:54:32  jadams
 *    increased default precsion on val to string
 *
 *    Revision 1.3  2003/10/10 14:56:10  jadams
 *    added sanitize, fixed a "bug" with whitespace on l_arrays
 *
 *    Revision 1.2  2003/10/07 18:08:12  jadams
 *    *** empty log message ***
 *
 *    Revision 1.1.1.1  2003/07/14 22:28:35  jadams
 *    start of gsd lib
 *
 *
 *    Revision 1.2  2003/07/01 15:38:35  jadams
 *    fixed sstreeam a.str()
 *
 *    Revision 1.1  2003/06/23 15:39:10  jadams
 *    *** empty log message ***
 *
 *    Revision 1.1.1.1  2003/05/08 21:10:09  jadams
 *
 *
 *
 *    Revision 1.3  2003/02/07 14:29:07  jadams
 *    Added escape backslash function as well as a double quote removal function
 *
 **********************************************************************/
/*! \file string_utils.h

  \brief String manipulation functions
*/

#ifndef _GSD_STRING_UTILS_
#define _GSD_STRING_UTILS_

#define GSD_STRING_UTILS_ 0.1

//#define _GSD_STRING_UTILS_DEBUG_
#ifdef _GSD_STRING_UTILS_DEBUG_
#define _GSD_STRING_UTILS_DEBUG_LEVEL_ 5
#endif

///////////////////////////////////////////// win32 specfic
#if defined(_WIN32)

// it seems this pragma has to be before *any* system include
//#pragma warning(disable : 4786)                   // disable identifier was truncated to '255' warning on STL
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <deque>
#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <bitset>

#include "data_types.h"

namespace gsd
{

    // this is gsd::str

    //! string handling routines
    namespace str
    {

        //! define the string list actually a string deque but thats too hard to spell
        typedef std::deque<std::string> slist;

        //! You need to call delete[] on the returned pointer!!!
        //!
        //! char * my_string=stl_string_to_c_style_string(str);
        //! printf("%s",my_string);
        //! delete[] my_string;
        char * stl_string_to_c_style_string(const std::string& s);

        // perl style functions

        //! Remove newlines from the end of a string
        int chomp(char msg[]);

        //! Split a string into tokens based on seperator.
        //!
        //! This is very close to it's perl counterpart split
        //! \sa join
        int split(const std::string &str, std::deque<std::string>& args, const char separator=' ' );

        //! Split a string into tokens based on seperator
        const std::deque<std::string> split(const char sep, const std::string& str);

        //! Split a string into tokens based on mulitple seperators
        const std::deque<std::string> multi_split(const std::deque<char>& seperators, const std::string& str);
        //! Join tokens into a single string using a seperator
        //!
        //! This is the reverse operation of split. It is very close
        //! to it's perl counterpart join
        //!
        //! \sa split
        const std::string join(const char sep, const std::deque<std::string>& args);

        // filters and cleanup
        //! Removes leading whitespace
        //!
        //!  eg "   Hello World!"  ---> "Hello World!"
        const std::string remove_leading_whitespace(const std::string& str);
        //! Removes trailing whitespace
        //!
        //!  eg "Hello World!   "  ---> "Hello World!"
        const std::string remove_trailing_whitespace(const std::string& str);
        //! Reduces repeated whitespace to a single whitespace
        //!
        //!  eg "Hello    World!"  ---> "Hello World!"
        const std::string squeeze_whitespace(const std::string &message);
        //! Remove all excess whitespace
        //!
        //!  eg "    Hello    World!    "  ---> "Hello World!"
        const std::string remove_excess_whitespace(const std::string &message);
        //! Converts whitespace to a new delimeter
        //!
        //! using _ as delimter:
        //!  eg "Hello World!"  ---> "Hello_World!"
        //! \sa convert_delimeter_to_whitespace
        const std::string convert_whitespace_to_delimeter(const std::string &str, const char delim='_');
        //! Converts  delimeter to whitespace
        //!
        //! using _ as delimter:
        //!  eg "Hello World!"  ---> "Hello_World!"
        //! \sa convert_delimeter_to_whitespace
        const std::string convert_delimeter_to_whitespace(const std::string &str, const char delim='_');
        //! Converts _ to \_
        //!
        const std::string escape_underscores(const std::string &message);
        const std::string remove_quotes(const std::string &message);

        const std::string remove_linefeeds_and_carriage_returns(const std::string &message);

        //! Remove all non printable characters
        //! 0-9,A-Z,a-z etc preserved
        //! \t --> space
        //! space --> space
        std::string sanitize(const std::string& message);

        // searching, eventually needs some regexp here
        //! only gets the first occurence
        int search_and_replace(std::string &str, const std::string& old_sub,const std::string& new_sub);
        //! gets all occurences
        int search_and_replace_all(std::string &str, const std::string& old_sub,const std::string& new_sub);

        // returns 1 if sub_str exists in str, 0 otherwise
        int search(const std::string &str, const std::string& sub_str);

        // removes all characters from # up to end of string.
        std::string remove_sh_comments(const std::string& line,const std::string comment_str="#");

        // formatting

        //! enum for justify routines
        enum {LEFT_JUSTIFY,RIGHT_JUSTIFY};

        //! Either left pad or right pad all the strings in a deque.
        //!
        //! ensures equal string length
        int justify_column( std::deque<std::string>& col,int justify_type=LEFT_JUSTIFY);

        //! add whitespace to the end
        int right_pad_string( std::string& str, size_t new_length);

        //! add whitespace to the end
        int left_pad_string( std::string& str, size_t new_length);

        //   int print_sdeque(deque<std::string>& my_stack);

        //! print a horizontal line
        //!
        //! eg ______________________________________________
        int hline(int line_length=70);

        //! reverse the string
        //!
        //! eg.
        //! abc --> cba
        //! madam  --> madam
        const std::string reverse(const std::string& str);

        //! make string uppercase
        void toupper(std::string & str);

        //! make string lower case
        void tolower(std::string & str);

        // conversions

        std::string itos(int i);                  //!< integer to string
        std::string ftos(float x);                //!< float to string
        std::string dtos(double x);               //!< double to string

        //! pretty print a number in binary
        //!
        //! eg T=5  -> "101"
        template <typename T>
            std::string pretty_print_as_binary(const T value)
        {
            // this is exceptionally nasty notation
            std::bitset<sizeof(T)*8> my_bitset(value);
            return my_bitset.template to_string<char,
                std::char_traits<char>,
                std::allocator<char> >();
        }

        //! convert bool to a "true" or "false"
        std::string bool_to_tf_string(const bool& b);

        //! convert an arbitrary type to a string
        template <typename T>
            std::string val_to_string(const T& x)
        {
            std::string str;
            std::stringstream a;

            // append a to string stream
            a  << x;

            str=convert_whitespace_to_delimeter(a.str(),'_');

            #ifdef _GSD_STRING_UTILS_DEBUG_
            cout << endl << "val_to_string, val=" << x;
            cout << endl << "val_to_string, a.str()=" << a.str();
            cout << endl << "val_to_string, str=" << str;
            cout.flush();
            #endif
            return str;

            //return a.str();

        }

        //! convert a numeric type to a string with formatting
        /*!

         \parameter fmt
          normally a printf parameter like "%1.4f", however for binary
          output a series of "%b" formats are handled as well:
          - "%b" binary output
          - "%8b" 8 bit binary output
          - "%16b" 16 bit binary output
          - "%32b" 32 bit binary output
        \sa val_to_string
        */
        template <typename T>
            std::string val_to_string(const T& x,const std::string& fmt)
        {
            std::string str;
            char buffer[64];

            if (fmt=="%b")
            {
                str=gsd::str::pretty_print_as_binary(static_cast<guint16>(x));
            }
            else if (fmt=="%8b")
            {
                str=gsd::str::pretty_print_as_binary(static_cast<guchar8>(x));
            }
            else if (fmt=="%16b")
            {
                str=gsd::str::pretty_print_as_binary(static_cast<guint16>(x));
            }
            else if (fmt=="%32b")
            {
                str=gsd::str::pretty_print_as_binary(static_cast<guint32>(x));
            }
            else if ((gsd::str::search(fmt,"x") | gsd::str::search(fmt,"X") |
                gsd::str::search(fmt,"d")))
            {
                //printf("\n detected hex mode");
                sprintf(buffer, fmt.c_str(), static_cast<guint32>(x));
                str=std::string(buffer);
            }
            else
            {

                sprintf(buffer, fmt.c_str(), x);

                str=std::string(buffer);
            }

            str=convert_whitespace_to_delimeter(str,'_');

            return str;

            //return a.str();

        }

        /*
        template <typename T>
                string val_to_string(const T& x,const ios_base::fmtflags& fmt)
            {
                std::string str;
                stringstream a;

            // cout << "\n string val_to_string:[" << fmt << "]";

            // set fmt flags see stroustrup pg. 627
            //a.setf(fmt);
        // setf doesn't work with non bit flags like hex base
        a.flags(fmt);

        // append a to string stream
        a  << x;

        str=convert_whitespace_to_delimeter(a.str(),'_');

        return str;

        //return a.str();

        }
        */

        //! convert a numeric type to a string with precision specified

        template <typename T>
            std::string val_to_string(const T& x,const int p)
        {
            std::string str;
            std::stringstream a;

            // append a to string stream
            a << std::setprecision(p)  << std::fixed << std::setw(0)<< x;
            //a  << x;

            str=convert_whitespace_to_delimeter(a.str(),'_');

            return str;
            // return a.str();

        }

        //! specialization to convert a string to a string
        //!
        //! avoid problem with genric routine which uses
        //! stringstream splits on whitespace
        inline std::string val_to_string(const std::string& x)
        {

            return x;

        }

        inline void  string_to_val(const std::string& str, std::string& x)
        {
            // output val as string
            x=str;

            return;

        }

        //! convert a string to a value
        //!
        template <typename T>
            void  string_to_val(const std::string& str, T& x)
        {
            std::stringstream a;
            std::string str2;

            str2=convert_delimeter_to_whitespace(str,'_');

            // append a to string stream
            a << str2;
            // output val as string
            a >> x;

            return;

        }

        //! convert a string to a value
        //!
        //! I'm not sure this form actually works
        template <typename T>
            T  string_to_val(const std::string& str)
        {
            T x;
            string_to_val(str, x);
            return x;

        }
        //! convert a string to a string (specialization)
        inline std::string  string_to_val(const std::string& str)
        {

            return str;

        }

        //! alias for val_to_string which uses ruby convention
        //! cause I can never remember the function name
        template <typename T>
            inline std::string to_s(const T& x)
        {
            return val_to_string(x);
        }

        //! alias for val_to_string which uses ruby convention
        //! cause I can never remember the function name
        inline double to_f(const std::string& str)
        {
            double x;

            string_to_val(str,x);

            return x;
        }

        inline double to_d(const std::string& str)
        {
            double x;

            string_to_val(str,x);

            return x;
        }

        inline int to_i(const std::string& str)
        {
            int x;

            string_to_val(str,x);

            return x;
        }

        //! deprecated \sa pretty_print_as_binary
        std::string pretty_print_guint16_as_binary(const guint16& x);

        //! Prints number with commas 10000 --> "10,000"
        std::string pretty_print_number(int x);

        //! prints ascii as ascii and others as numeric
        void ascii_dump_control_characters(const std::string& str);
        std::string pretty_print_control_characters(const std::string& str);
        std::string pretty_print_hex_dump(const std::string& str);
        //! prints ascii as ascii and others as numeric
        void ascii_dump_control_characters(char c);

        //! Print a string in hexadecimal
        void hex_dump(const std::string& str);

        // functions to deal with non null terminated string

        //! helper function for null-termination issues
        std::string cp_non_terminated_c_string(const char *src, int L);
        //! helper function for null-termination issues
        char* cp_and_null_terminate_c_string(char *dest_buffer, int L_buffer, const char *src, int L_src);

        inline bool is_printable(char c)
        {

            const char min_valid_ascii_value=0x20;//' ';
            const char max_valid_ascii_value=0x7E;// '~';

            return ( (c >= min_valid_ascii_value) &&
                ( c <= max_valid_ascii_value) );
        }

        inline bool is_alpha_upper(char c)
        {

            const char min_valid_ascii_value=0x41;//'A';
            const char max_valid_ascii_value=0x90;// 'Z';

            return ( (c >= min_valid_ascii_value) &&
                ( c <= max_valid_ascii_value) );
        }

        inline bool is_alpha_lower(char c)
        {
            const char min_valid_ascii_value=0x61;//'A';
            const char max_valid_ascii_value=0x7A;// '~';
            return ( (c >= min_valid_ascii_value) &&
                ( c <= max_valid_ascii_value) );
        }

        inline bool is_alpha(char c)
        {

            return is_alpha_lower(c) || is_alpha_upper(c);

        }

        inline bool is_numeric(char c)
        {

            const char min_valid_ascii_value=0x30;//'0';
            const char max_valid_ascii_value=0x39;// '9';
            return ( (c >= min_valid_ascii_value) &&
                ( c <= max_valid_ascii_value) );

        }

    }
    //end of gsd::str
    //! alias for val_to_string which uses ruby convention
    //! cause I can never remember the function name
    template <typename T>
        inline std::string to_s(const T& x)
    {
        return gsd::str::val_to_string(x);
    }

}                                                 //end of gsd
#endif
