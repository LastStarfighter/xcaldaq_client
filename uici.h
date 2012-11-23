/*********************************** uici.h **************************/
/*   Prototypes for the three public UICI functions                  */
/*********************************************************************/
#ifdef __cplusplus
extern "C"
{
    #endif

    #define UPORT
    typedef unsigned short u_port_t;
    int u_open(u_port_t port);
    int u_accept(int fd, char *hostn, int hostnsize);
    int u_connect(u_port_t port, char *hostn);

    #ifdef __cplusplus
}
#endif
