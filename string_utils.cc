/********************************************************************
 *
 *
 * Name: string_utilts.h
 *
 *
 * Author: Joseph Adams
 *
 * Description:
 *   utils for handling stl strings (hopefully perl style)
 *
 * --------------------------------------------------------------
 *    $Revision: 1.29 $
 * ---------------------------------------------------------------
 *
 *
 **********************************************************************/

#include "string_utils.h"
#include "gsd_error.h"
#include <algorithm>
#include <cctype>                                 // for toupper
#include <bitset>
#include <string>
#include <string.h>

namespace gsd
{
    // this is gsd::str
    namespace str
    {

        char * stl_string_to_c_style_string(const std::string& s)
        {
            // the +1 is for the null terminator
            char * c_style_string= new char[s.size()+1];

            strcpy(c_style_string,s.c_str());

            return c_style_string;

        }
        const std::string join(const char sep, const std::deque<std::string>& args)
        {

            std::string str;

            for (unsigned int i=0; i< args.size(); i++)
            {

                if (i>0)
                {
                    str += sep;
                }

                str += args[i];

            }

            return str;

        }

        const std::deque<std::string> multi_split(const std::deque<char>& seperators, const std::string& str)
        {
            std::deque<std::string> tokens;

            std::string new_string=str;

            // we will replace all the 'splitting' tokens to the first in the list
            // and then do a traditional split

            for (unsigned int i=1; i<seperators.size(); i++)
            {
                std::string seperator_0=gsd::str::to_s(seperators[0]);
                std::string seperator_i=gsd::str::to_s(seperators[i]);

                gsd::str::search_and_replace(new_string,seperator_0,seperator_i);

            }

            tokens=split(seperators[0], new_string);

            return tokens;

        }

        const std::deque<std::string> split(const char sep, const std::string& str)
        {
            std::deque<std::string> args;

            int result;

            result= split(str, args, sep);

            return args;

        }

        int split(const std::string &message_, std::deque<std::string>& args, const char separator)
        {

            std::string::size_type pos1;
            std::string::size_type pos2;

            std::string message;
            std::string token;

            args.clear();

            //	char separator=' ';

            pos1=0;
            pos2 = 0;

            bool done=false;

            message=message_;

            while (!done)
            {
                // this command returns a position?
                pos2 = message.find(separator);

                if (pos2 != std::string::npos)
                {

                    // check for invalid start point
                    if ((pos2) >= 0)
                    {
                        token = message.substr(0,pos2);

                        if (token.length())
                            args.push_back(token);
                    }
                    else
                    {
                        done=true;

                    }

                    // check for invalid end point
                    if (pos2+1 < message.size())
                    {
                        message=message.substr(pos2+1,message.size());

                    }
                    else
                    {

                        done=true;

                    }
                }
                else
                {
                    // make sure to add the remainder to the std::deque
                    args.push_back(message);
                    done = true;

                }

            }

            return args.size();
        }

        const std::string remove_quotes(const std::string &message)
        {

            std::string::size_type pos1=0;
            std::string::size_type pos2=0;

            std::string message2;

            //	char quote='\"';

            char quote=0x22;

            message2=message;

            // this command returns a position?
            pos1 = message.find(quote);
            pos2 = message.rfind(quote);

            if ( (pos1 != std::string::npos) &&  (pos2 != std::string::npos) )
            {
                // check for invalid start point
                if (pos1 != pos2)
                {
                    message2 = message.substr(pos1+1,pos2-1);
                }

            }

            return message2;
        }

        const std::string reverse(const std::string& str1)
        {

            std::string str2( str1 );
            std::copy( str1.begin(), str1.end(), str2.rbegin() );

            return str2;
        }

        const std::string remove_linefeeds_and_carriage_returns(const std::string &str)
        {

            const char LF=0xA;
            const char CR=0xD;
            char this_char;

            std::string str2;

            for (std::string::size_type i=0; i < str.size(); i++)
            {

                this_char=str[i];

                if ((this_char==LF) || (this_char==CR))
                {
                    ;                             // do nothing
                }
                else
                {
                    str2 += this_char;

                }

            }

            return str2;

        }

        const std::string escape_underscores(const std::string &str)
        {

            //const char space=' ';
            const char backslash='\\';
            char last_char='x';                   // this needs to be non space for the algorithm to work
            char this_char='x';

            std::string str2;

            //str2=str;

            str2.erase();

            char *buffer;

            buffer = new char[str.length()*2];

            strcpy(buffer,str.c_str());

            //char c;

            for (std::string::size_type i=0; i < str.size(); i++)
            {

                this_char=str[i];
                str2 += this_char;

                if (this_char==backslash)
                {
                    str2 += this_char;
                }

                last_char=this_char;

            }

            delete[] buffer;
            return str2;
        }

        const std::string squeeze_whitespace(const std::string &str)
        {

            const char space=' ';
            // this needs to be non space for the algorithm to work
            char last_char='x';
            char this_char='x';

            std::string str2;

            for (std::string::size_type i=0; i < str.size(); i++)
            {

                this_char=str[i];

                if (str[i]==space && last_char==space)
                {
                    ;                             // do nothing
                }
                else
                {
                    str2 += this_char;

                }

                last_char=this_char;

            }

            return str2;
        }

        const std::string remove_trailing_whitespace(const std::string& str)
        {
            std::string str2(str);

            str2=reverse(str2);

            str2=remove_leading_whitespace(str2);

            str2=reverse(str2);

            return str2;

        }

        const std::string remove_leading_whitespace(const std::string& str)
        {

            const char space=' ';

            // this needs to be non space for the algorithm to work
            char this_char;

            std::string str2;
            bool non_space_found=false;

            for (std::string::size_type i=0; i < str.size(); i++)
            {

                this_char=str[i];

                if (str[i]!=space) non_space_found=true;

                if (non_space_found)
                {
                    str2 += this_char;

                }

            }

            return str2;
        }

        const std::string remove_excess_whitespace(const std::string& str)
        {
            std::string str2;

            str2=remove_leading_whitespace(str);
            str2=remove_trailing_whitespace(str2);
            str2=squeeze_whitespace(str2);

            return str2;

        }

        // #define SPACES " \t\r\n"

        // inline string trim_right (const string & s, const string & t = SPACES)
        //   {
        //   string d (s);
        //   string::size_type i (d.find_last_not_of (t));
        //   if (i == string::npos)
        //     return "";
        //   else
        //    return d.erase (d.find_last_not_of (t) + 1) ;
        //   }  // end of trim_right

        // inline string trim_left (const string & s, const string & t = SPACES)
        //   {
        //   string d (s);
        //   return d.erase (0, s.find_first_not_of (t)) ;
        //   }  // end of trim_left

        // inline string trim (const string & s, const string & t = SPACES)
        //   {
        //   string d (s);
        //   return trim_left (trim_right (d, t), t) ;
        //   }  // end of trim

        const std::string convert_whitespace_to_delimeter(const std::string &str, const char delim)
        {

            const char space=' ';

            // this needs to be non space for the algorithm to work
            char this_char=!space;
            char last_char=!space;

            std::string str2;

            for (std::string::size_type i=0; i < str.size(); i++)
            {

                this_char=str[i];

                if (str[i]==space && last_char==space)
                {
                    ;                             // do nothing
                }
                else if (str[i]==space)
                {
                    str2 += delim;                // do nothing
                }
                else
                {
                    str2 += this_char;

                }

                last_char=this_char;

            }

            return str2;
        }

        const std::string convert_delimeter_to_whitespace(const std::string &str, const char delim)
        {

            const char space=' ';
            // this needs to be non space for the algorithm to work
            //    char last_char='x';
            char this_char;

            std::string str2;

            for (std::string::size_type i=0; i < str.size(); i++)
            {

                this_char=str[i];

                if (str[i]==delim)
                {
                    str2 += space;                // do nothing
                }
                else
                {
                    str2 += this_char;

                }

            }

            return str2;
        }

        // removes the ^M ^R from the end a dos style string
        // should work for the newline on unix too
        int chomp(char msg[])
        {

            //	printf("\n chomp");

            std::string::size_type last=strlen(msg)-1;

            //	printf("\t msg[%d]=%d",last,msg[last]);
            if (msg[last]==10)
            {

                msg[last]='\0';

            }

            last=strlen(msg)-1;
            //	printf("\t msg[%d]=%d",last,msg[last]);
            if (msg[last]==13)
            {

                msg[last]='\0';

            }

            return 0;

        }

        //! removes from #--->\n or end of string
        std::string remove_sh_comments(const std::string& line,const std::string comment_str)
        {

            std::string::size_type pos1;
            std::string::size_type pos2;
            // std::string::size_type len;

            std::string new_line=line;
            while((pos1=new_line.find(comment_str,0)) != std::string::npos)
            {

                pos2=new_line.find("\n",0);
                new_line.erase(pos1,pos2);

            }

            return new_line;
        }

        int search(const std::string &str, const std::string& sub_str)
        {

            std::string::size_type pos1;
            // std::string::size_type len;

            pos1=str.find(sub_str,0);

            if (pos1 == std::string::npos)
                return 0;
            else
                return 1;

        }

        int search_and_replace(std::string &str, const std::string& old_sub, const std::string& new_sub)
        {

            std::string::size_type pos1;
            std::string::size_type len;

            pos1=str.find(old_sub,0);

            if (pos1 == std::string::npos)
                return -1;

            len=old_sub.length();

            str.replace(pos1,len,new_sub);

            return 0;

        }

        int search_and_replace_all(std::string &str, const std::string& old_sub, const std::string& new_sub)
        {

            // int done=0;
            //int result=0;

            while (search_and_replace(str, old_sub, new_sub) != -1)
            {

                ;

            }

            return 0;

        }

        void toupper(std::string & str)
        {
            transform(str.begin(), str.end(), str.begin(), ::toupper);
        }

        void tolower(std::string & str)
        {
            transform(str.begin(), str.end(), str.begin(), ::tolower);
        }

        std::string itos(int i)                   // convert int to std::string
        {
            std::stringstream s;
            s << i;
            return s.str();
        }

        std::string dtos(double x)
        {
            std::stringstream s;
            s << x;
            return s.str();
        }

        std::string itos(float x)                 // convert int to string
        {
            std::stringstream s;
            s << x;
            return s.str();
        }

        std::string bool_to_tf_string(const bool& b)
        {

            std::string str;

            if (b)
            {
                str="true";
            }
            else
            {
                str="false";

            }

            return str;
        }

        int justify_column( std::deque<std::string>& col,int justify_type)
        {
            size_t max_length=0;

            std::deque<std::string>::iterator iter;

            // print out the menu
            iter = col.begin();

            while(iter != col.end())
            {
                if (max_length < iter->size())
                {
                    max_length = iter->size();

                }

                iter++;
            }

            // print out the menu
            iter = col.begin();

            while(iter != col.end())
            {
                if (justify_type==LEFT_JUSTIFY)
                    right_pad_string(*iter,max_length);
                else if (justify_type==RIGHT_JUSTIFY)
                    left_pad_string(*iter,max_length);
                else
                {
                    std::cout << "\n\nError:unknown justification\n\n";
                    exit(1);

                }

                iter++;
            }

            return 0;

        }

        int right_pad_string( std::string& str, size_t new_length)
        {

            int count=0;
            while (str.size() < new_length)
            {

                str += " ";
                count++;
            }

            return count;
        }

        int left_pad_string( std::string& str, size_t new_length)
        {

            int count=0;
            while (str.size() < new_length)
            {

                str = " " + str;
                count++;
            }

            return count;
        }

        int hline(int line_length)
        {
            std::cout << std::endl;
            for (int i=0; i<line_length; i++)
                std::cout << "_";
            std::cout.flush();

            return 0;

        }

        std::string sanitize(const std::string& old_str)
        {

            const char min_valid_ascii_value=0x20;//' ';
            const char max_valid_ascii_value=0x7E;// '~';
            char this_char;
            std::string str2;

            for (std::string::size_type i=0; i < old_str.size(); i++)
            {

                this_char=old_str[i];

                if ( (this_char >= min_valid_ascii_value) &&
                    ( this_char <= max_valid_ascii_value) )
                {
                    str2 += this_char;
                }
                else if (this_char=='\t')
                {
                    str2 += " ";
                }
                else if (this_char==' ')
                {
                    str2 += " ";
                }

            }

            return str2;

        }

        std::string  pretty_print_guint16_as_binary(const guint16& x)
        {

            std::bitset<16> bit16=x;
            std::string bit_str;

            // for gcc >= 3.4
            #ifdef __GNUC_MINOR__
            #if (__GNUC__>=4) || (( __GNUC__==3) && (__GNUC_MINOR__ >= 4))
            bit_str=bit16.to_string<char, std::char_traits<char>, std::allocator<char> >();
            #else
            bit_str=bit16.template to_string<char, std::char_traits<char>, std::allocator<char> >();
            #endif

            #else
            bit_str=bit16.template to_string<char, std::char_traits<char>, std::allocator<char> >();
            #endif

            return bit_str;

        }

        std::string pretty_print_number(int x)
        {
            // this should be replaced by num_put if I ever learn it....
            std::string str;
            std::string str2;

            str=val_to_string(x);

            str2.erase();
            //		string::size_type pos1;
            //        string::size_type pos2;

            //char separator='.';
            int count=0;
            for (int i=str.size()-1; i>=0; i--)
            {

                str2 += str[i];
                count++;

                if (count==3)
                {
                    if (i>0) str2 += ',';
                    count=0;

                }

            }

            //	str2=str1;

            // Call reverse algorithm to reverse the numbers
            std::reverse( str2.begin(), str2.end() );

            return str2;

        }
        std::string pretty_print_hex_dump(const std::string& str)
        {

            char c;
            //            int d;
            //printf("\n Ascii dump: size=%d:\t""",str.size());
            //printf("size()=%d:\t\"",str.size());

            std::stringstream a;

            a << "size()=" << str.size() << "\t\"";
            for (unsigned int j=0; j< str.size(); j++)
            {
                c=str[j];
                a.precision(2);
                //                  a << "[" <<  std::hex << static_cast<int>(c) << "]";

                a << "[" <<  static_cast<int>(c) << "]";

            }

            a << "\"";
            return a.str();

        }
        std::string pretty_print_control_characters(const std::string& str)
        {

            char c;
            //            int d;
            //printf("\n Ascii dump: size=%d:\t""",str.size());
            //printf("size()=%d:\t\"",str.size());

            std::stringstream a;

            a << "size()=" << str.size() << "\t\"";
            for (unsigned int j=0; j< str.size(); j++)
            {
                c=str[j];
                if (gsd::str::is_printable(c))
                {
                    a << c;
                }
                else
                {
                    a << "[" << static_cast<int>(c) << "]";
                }
            }

            a << "\"";
            return a.str();

        }
        void ascii_dump_control_characters(const std::string& str)
        {
            std::string new_str;
            char c;
            //            int d;
            //printf("\n Ascii dump: size=%d:\t""",str.size());
            printf("size()=%d:\t\"",static_cast<int>(str.size()));
            for (unsigned int j=0; j< str.size(); j++)
            {
                c=str[j];
                if (gsd::str::is_printable(c))
                {
                    printf("%c",c);
                }
                else
                {
                    printf("[%d]",c);
                }
            }
            printf("\"");
        }

        void ascii_dump_control_characters(char c)
        {

            if (gsd::str::is_printable(c))
            {
                printf("%c",c);
            }
            else
            {
                printf("[%d]",c);
            }
        }

        /* 
        void hex_dump(const std::string& str)
        {
        const char min_valid_ascii_value=0x21;   //0x20=' '; 21
            const char max_valid_ascii_value=0x7E;   // '~';

        fprintf(stdout,"\nstr.size()=<%d>, {",str.size());
        char c;

                  for (unsigned int j=0; j< str.size(); j++)
                  {
        c=str[j];

        if ((c >= min_valid_ascii_value) && (c <= max_valid_ascii_value))
        fprintf(stdout,"%c",str[j]);
        else
        fprintf(stdout,"[%02X]",str[j]);
        }
        fprintf(stdout,"}");

        }
        */

        void hex_dump(const std::string& str)
        {

            fprintf(stdout,"\nstr.size()=<%d>, {",static_cast<int>(str.size()));
            unsigned char uc;
            for (unsigned int j=0; j< str.size(); j++)
            {

                uc=str[j];

                fprintf(stdout,"[%02X]",uc);
            }
            fprintf(stdout,"}");

        }

        char* cp_and_null_terminate_c_string(char *dest_buffer, int L_buffer, const char *src, int L_src)
        {

            if (L_src+1 > L_buffer)
                warning("Buffer not allocated large enough","cp_and_null_terminate_c_string");

            for (int i=0; i < L_src;  i++)
            {
                dest_buffer[i]=src[i];

            }

            //  strncpy(buffer,src,L_src);
            dest_buffer[L_src]='\0';

            return dest_buffer;

        }

        std::string cp_non_terminated_c_string(const char *src, int L)
        {

            //char tmp_buffer[L+1];
            char *tmp_buffer;

            tmp_buffer =new char[L+1];
            strncpy(tmp_buffer,src,L);
            tmp_buffer[L+1]='\0';

            delete[] tmp_buffer;

            return std::string(tmp_buffer);

        }

    }                                             // end of namespace gsd::str

}                                                 // end of namespace gsd
