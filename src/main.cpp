#ifdef NEED_SERVER
extern "C" int telnetd_start(void);
#else
extern "C" int run(void);
#endif

int main(int argc, const char **argv)
{
    #ifdef NEED_SERVER
    telnetd_start();
    #else
    run();
    #endif
}