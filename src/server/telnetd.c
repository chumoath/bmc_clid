#define _XOPEN_SOURCE 600
#include <stdlib.h>
#define __USE_MISC
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <pty.h>
#include <signal.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <arpa/telnet.h>
#include <arpa/inet.h>

extern int run(void);
extern const char *greeting;

struct tsession {
    struct tsession *next;
    pid_t shell_pid;
    int sockfd_read;
    int sockfd_write;
    int ptyfd;
    signed char buffered_IAC_for_pty;
    /* two circular buffers */
    /*char *buf1, *buf2;*/
/*#define TS_BUF1(ts) ts->buf1*/
/*#define TS_BUF2(ts) TS_BUF2(ts)*/
#define TS_BUF1(ts) ((unsigned char*)(ts + 1))
#define TS_BUF2(ts) (((unsigned char*)(ts + 1)) + BUFSIZE)
    int rdidx1, wridx1, size1;
    int rdidx2, wridx2, size2;
};

/* Two buffers are directly after tsession in malloced memory.
 * Make whole thing fit in 4k */
enum { BUFSIZE = (4 * 1024 - sizeof(struct tsession)) / 2 };
enum { GETPTY_BUFSIZE = 16 };

/* Globals */
struct globals {
    struct tsession *sessions;
    int maxfd;
};

static char global_buffer[1024];

#define G (*(struct globals*)global_buffer)
#define ALIGN1 __attribute__((aligned(1)))
#define MIN(a,b) (((a)<(b))?(a):(b))

static ssize_t safe_write(int fd, const void *buf, size_t count)
{
    ssize_t n;

    for (;;) {
        n = write(fd, buf, count);
        if (n >= 0 || errno != EINTR)
            break;
        /* Some callers set errno=0, are upset when they see EINTR.
         * Returning EINTR is wrong since we retry write(),
         * the "error" was transient.
         */
        errno = 0;
        /* repeat the write() */
    }

    return n;
}

static ssize_t safe_read(int fd, void *buf, size_t count)
{
    ssize_t n;

    for (;;) {
        n = read(fd, buf, count);
        if (n >= 0 || errno != EINTR)
            break;
        /* Some callers set errno=0, are upset when they see EINTR.
         * Returning EINTR is wrong since we retry read(),
         * the "error" was transient.
         */
        errno = 0;
        /* repeat the read() */
    }

    return n;
}

static void* xmalloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL && size != 0) {
        perror("memory_exhausted");
        exit (-1);
    }
    return ptr;
}

static void* xzalloc(size_t size)
{
    void *ptr = xmalloc(size);
    memset(ptr, 0, size);
    return ptr;
}

static int xgetpty(char *line)
{
    int p;

    p = open("/dev/ptmx", O_RDWR);
    if (p >= 0) {
        grantpt(p); /* chmod+chown corresponding slave pty */
        unlockpt(p); /* (what does this do?) */

        if (ptsname_r(p, line, GETPTY_BUFSIZE-1) != 0) {
            perror("pstname_r");
            exit (-1);
        }
        line[GETPTY_BUFSIZE-1] = '\0';
        return p;
    }

    perror ("can't find free pty");
    exit (-1);
}

static int ndelay_on(int fd)
{
    int flags = fcntl(fd, F_GETFL);
    if (flags & O_NONBLOCK)
        return flags;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    return flags;
}

static void close_on_exec_on(int fd)
{
    fcntl(fd, F_SETFD, FD_CLOEXEC);
}

static int setsockopt_keepalive(int fd)
{
    int optval = 1;
    return setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(int));
}

static int fflush_all(void)
{
    return fflush(NULL);
}

static void bb_signals(int sigs, void (*f)(int))
{
    int sig_no = 0;
    int bit = 1;

    while (sigs) {
        if (sigs & bit) {
            sigs -= bit;
            signal(sig_no, f);
        }
        sig_no++;
        bit <<= 1;
    }
}

static int xopen(const char *pathname, int flags)
{
    int ret;

    ret = open(pathname, flags, 0666);
    if (ret < 0) {
        char buf[64];
        sprintf(buf, "can't open '%s'", pathname);
        perror(buf);
        exit(-1);
    }
    return ret;
}

static void xdup2(int from, int to)
{
    if (dup2(from, to) != to) {
        perror("can't duplicate file descriptor");
        exit (-1);
    }
}

static void xlisten(int s, int backlog)
{
    if (listen(s, backlog)) {
        perror("listen");
        exit (-1);
    }
}

static void xbind(int sockfd, struct sockaddr *my_addr, socklen_t addrlen)
{
    if (bind(sockfd, my_addr, addrlen)) {
        perror("bind");
        exit (-1);
    }
}

static int xsocket(int domain, int type, int protocol)
{
    int r = socket(domain, type, protocol);

    if (r < 0) {
        perror("socket");
        exit(-1);
    }

    return r;
}

static void setsockopt_reuseaddr(int fd)
{
    int optval = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));
}

static int create_and_bind_stream(uint16_t port)
{
    int fd;
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    fd = xsocket(PF_INET, SOCK_STREAM, 0);
    setsockopt_reuseaddr(fd);
    xbind(fd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    return fd;
}

static int tcsetattr_stdin_TCSANOW(const struct termios *tp)
{
    return tcsetattr(STDIN_FILENO, TCSANOW, tp);
}

/* Write some buf1 data to pty, processing IACs.
 * Update wridx1 and size1. Return < 0 on error.
 * Buggy if IAC is present but incomplete: skips them.
 */
static ssize_t safe_write_to_pty_decode_iac(struct tsession *ts)
{
    unsigned wr;
    ssize_t rc;
    unsigned char *buf;
    unsigned char *found;

    buf = TS_BUF1(ts) + ts->wridx1;
    wr = MIN(BUFSIZE - ts->wridx1, ts->size1);
    /* wr is at least 1 here */

    if (ts->buffered_IAC_for_pty) {
        /* Last time we stopped on a "dangling" IAC byte.
         * We removed it from the buffer back then.
         * Now pretend it's still there, and jump to IAC processing.
         */
        ts->buffered_IAC_for_pty = 0;
        wr++;
        ts->size1++;
        buf--; /* Yes, this can point before the buffer. It's ok */
        ts->wridx1--;
        goto handle_iac;
    }

    found = memchr(buf, IAC, wr);
    if (found != buf) {
        /* There is a "prefix" of non-IAC chars.
         * Write only them, and return.
         */
        if (found)
            wr = found - buf;

        /* We map \r\n ==> \r for pragmatic reasons:
         * many client implementations send \r\n when
         * the user hits the CarriageReturn key.
         * See RFC 1123 3.3.1 Telnet End-of-Line Convention.
         */
        rc = wr;
        found = memchr(buf, '\r', wr);
        if (found)
            rc = found - buf + 1;
        rc = safe_write(ts->ptyfd, buf, rc);
        if (rc <= 0)
            return rc;
        if (rc < wr /* don't look past available data */
            && buf[rc-1] == '\r' /* need this: imagine that write was _short_ */
            && (buf[rc] == '\n' || buf[rc] == '\0')
                ) {
            rc++;
        }
        goto update_and_return;
    }

    /* buf starts with IAC char. Process that sequence.
     * Example: we get this from our own (bbox) telnet client:
     * read(5, "\377\374\1""\377\373\37""\377\372\37\0\262\0@\377\360""\377\375\1""\377\375\3"):
     * IAC WONT ECHO, IAC WILL NAWS, IAC SB NAWS <cols> <rows> IAC SE, IAC DO SGA
     * Another example (telnet-0.17 from old-netkit):
     * read(4, "\377\375\3""\377\373\30""\377\373\37""\377\373 ""\377\373!""\377\373\"""\377\373'"
     * "\377\375\5""\377\373#""\377\374\1""\377\372\37\0\257\0I\377\360""\377\375\1"):
     * IAC DO SGA, IAC WILL TTYPE, IAC WILL NAWS, IAC WILL TSPEED, IAC WILL LFLOW, IAC WILL LINEMODE, IAC WILL NEW_ENVIRON,
     * IAC DO STATUS, IAC WILL XDISPLOC, IAC WONT ECHO, IAC SB NAWS <cols> <rows> IAC SE, IAC DO ECHO
     */
    if (wr <= 1) {
        /* Only the single IAC byte is in the buffer, eat it
         * and set a flag "process the rest of the sequence
         * next time we are here".
         */
        //bb_error_msg("dangling IAC!");
        ts->buffered_IAC_for_pty = 1;
        rc = 1;
        goto update_and_return;
    }

    handle_iac:
    /* 2-byte commands (240..250 and 255):
     * IAC IAC (255) Literal 255. Supported.
     * IAC SE  (240) End of subnegotiation. Treated as NOP.
     * IAC NOP (241) NOP. Supported.
     * IAC BRK (243) Break. Like serial line break. TODO via tcsendbreak()?
     * IAC AYT (246) Are you there.
     *  These don't look useful:
     * IAC DM  (242) Data mark. What is this?
     * IAC IP  (244) Suspend, interrupt or abort the process. (Ancient cousin of ^C).
     * IAC AO  (245) Abort output. "You can continue running, but do not send me the output".
     * IAC EC  (247) Erase character. The receiver should delete the last received char.
     * IAC EL  (248) Erase line. The receiver should delete everything up tp last newline.
     * IAC GA  (249) Go ahead. For half-duplex lines: "now you talk".
     *  Implemented only as part of NAWS:
     * IAC SB  (250) Subnegotiation of an option follows.
     */
    if (buf[1] == IAC) {
        /* Literal 255 (emacs M-DEL) */
        //bb_error_msg("255!");
        rc = safe_write(ts->ptyfd, &buf[1], 1);
        /*
         * If we went through buffered_IAC_for_pty==1 path,
         * bailing out on error like below messes up the buffer.
         * EAGAIN is highly unlikely here, other errors will be
         * repeated on next write, let's just skip error check.
         */
        rc = 2;
        goto update_and_return;
    }
    if (buf[1] == AYT) {
        if (ts->size2 == 0) { /* if nothing buffered yet... */
            /* Send back evidence that AYT was seen */
            unsigned char *buf2 = TS_BUF2(ts);
            buf2[0] = IAC;
            buf2[1] = NOP;
            ts->wridx2 = 0;
            ts->rdidx2 = ts->size2 = 2;
        }
        rc = 2;
        goto update_and_return;
    }
    if (buf[1] >= 240 && buf[1] <= 249) {
        /* NOP (241). Ignore (putty keepalive, etc) */
        /* All other 2-byte commands also treated as NOPs here */
        rc = 2;
        goto update_and_return;
    }

    if (wr <= 2) {
/* BUG: only 2 bytes of the IAC is in the buffer, we just eat them.
 * This is not a practical problem since >2 byte IACs are seen only
 * in initial negotiation, when buffer is empty
 */
        rc = 2;
        goto update_and_return;
    }

    if (buf[1] == SB) {
        if (buf[2] == TELOPT_NAWS) {
            /* IAC SB, TELOPT_NAWS, 4-byte, IAC SE */
            struct winsize ws;
            if (wr <= 6) {
/* BUG: incomplete, can't process */
                rc = wr;
                goto update_and_return;
            }
            memset(&ws, 0, sizeof(ws)); /* pixel sizes are set to 0 */
            ws.ws_col = (buf[3] << 8) | buf[4];
            ws.ws_row = (buf[5] << 8) | buf[6];
            ioctl(ts->ptyfd, TIOCSWINSZ, (char *)&ws);
            rc = 7;
            /* trailing IAC SE will be eaten separately, as 2-byte NOP */
            goto update_and_return;
        }
        /* else: other subnegs not supported yet */
    }

    /* Assume it is a 3-byte WILL/WONT/DO/DONT 251..254 command and skip it */
    rc = 3;

    update_and_return:
    ts->wridx1 += rc;
    if (ts->wridx1 >= BUFSIZE) /* actually == BUFSIZE */
        ts->wridx1 = 0;
    ts->size1 -= rc;
    /*
     * Hack. We cannot process IACs which wrap around buffer's end.
     * Since properly fixing it requires writing bigger code,
     * we rely instead on this code making it virtually impossible
     * to have wrapped IAC (people don't type at 2k/second).
     * It also allows for bigger reads in common case.
     */
    if (ts->size1 == 0) { /* very typical */
        //bb_error_msg("zero size1");
        ts->rdidx1 = 0;
        ts->wridx1 = 0;
        return rc;
    }
    wr = ts->wridx1;
    if (wr != 0 && wr < ts->rdidx1) {
        /* Buffer is not wrapped yet.
         * We can easily move it to the beginning.
         */
        //bb_error_msg("moved %d", wr);
        memmove(TS_BUF1(ts), TS_BUF1(ts) + wr, ts->size1);
        ts->rdidx1 -= wr;
        ts->wridx1 = 0;
    }
    return rc;
}

/*
 * Converting single IAC into double on output
 */
static size_t safe_write_double_iac(int fd, const char *buf, size_t count)
{
    const char *IACptr;
    size_t wr, rc, total;

    total = 0;
    while (1) {
        if (count == 0)
            return total;
        if (*buf == (char)IAC) {
            static const char IACIAC[] ALIGN1 = { IAC, IAC };
            rc = safe_write(fd, IACIAC, 2);
/* BUG: if partial write was only 1 byte long, we end up emitting just one IAC */
            if (rc != 2)
                break;
            buf++;
            total++;
            count--;
            continue;
        }
        /* count != 0, *buf != IAC */
        IACptr = memchr(buf, IAC, count);
        wr = count;
        if (IACptr)
            wr = IACptr - buf;
        rc = safe_write(fd, buf, wr);
        if (rc != wr)
            break;
        buf += rc;
        total += rc;
        count -= rc;
    }
    /* here: rc - result of last short write */
    if ((ssize_t)rc < 0) { /* error? */
        if (total == 0)
            return rc;
        rc = 0;
    }
    return total + rc;
}

static struct tsession *make_new_session(int sock)
{
    struct termios termbuf;
    int fd, pid;
    char tty_name[GETPTY_BUFSIZE];
    struct tsession *ts = xzalloc(sizeof(struct tsession) + BUFSIZE * 2);

    /*ts->buf1 = (char *)(ts + 1);*/
    /*ts->buf2 = ts->buf1 + BUFSIZE;*/

    /* Got a new connection, set up a tty */
    fd = xgetpty(tty_name);
    if (fd > G.maxfd)
        G.maxfd = fd;
    ts->ptyfd = fd;
    ndelay_on(fd);
    close_on_exec_on(fd);

    /* SO_KEEPALIVE by popular demand */
    setsockopt_keepalive(sock);

    ts->sockfd_read = sock;
	ndelay_on(sock);
	if (sock == 0) { /* We are called with fd 0 - we are in inetd mode */
		sock++; /* so use fd 1 for output */
		ndelay_on(sock);
	}
	ts->sockfd_write = sock;
	if (sock > G.maxfd)
		G.maxfd = sock;

    /* Make the telnet client understand we will echo characters so it
     * should not do it locally. We don't tell the client to run linemode,
     * because we want to handle line editing and tab completion and other
     * stuff that requires char-by-char support. */
    {
        static const char iacs_to_send[] ALIGN1 = {
                IAC, DO, TELOPT_ECHO,
                IAC, DO, TELOPT_NAWS,
                /* This requires telnetd.ctrlSQ.patch (incomplete) */
                /*IAC, DO, TELOPT_LFLOW,*/
                IAC, WILL, TELOPT_ECHO,
                IAC, WILL, TELOPT_SGA
        };
        /* This confuses safe_write_double_iac(), it will try to duplicate
         * each IAC... */
        //memcpy(TS_BUF2(ts), iacs_to_send, sizeof(iacs_to_send));
        //ts->rdidx2 = sizeof(iacs_to_send);
        //ts->size2 = sizeof(iacs_to_send);
        /* So just stuff it into TCP stream! (no error check...) */
        safe_write(sock, iacs_to_send, sizeof(iacs_to_send));

        /*ts->rdidx2 = 0; - xzalloc did it */
        /*ts->size2 = 0;*/
    }

    fflush_all();
    pid = fork(); /* NOMMU-friendly */
    if (pid < 0) {
        free(ts);
        close(fd);
        /* sock will be closed by caller */
        perror("vfork");
        return NULL;
    }
    if (pid > 0) {
        /* Parent */
        ts->shell_pid = pid;
        return ts;
    }

    /* Child */
    /* Careful - we are after vfork! */

    /* Restore default signal handling ASAP */
    bb_signals((1 << SIGCHLD) + (1 << SIGPIPE), SIG_DFL);

    pid = getpid();

    /* Make new session and process group */
    setsid();

    /* Open the child's side of the tty */
    /* NB: setsid() disconnects from any previous ctty's. Therefore
     * we must open child's side of the tty AFTER setsid! */
    close(0);
    xopen(tty_name, O_RDWR); /* becomes our ctty */
    xdup2(0, 1);
    xdup2(0, 2);
    tcsetpgrp(0, pid); /* switch this tty's process group to us */

    /* The pseudo-terminal allocated to the client is configured to operate
     * in cooked mode, and with XTABS CRMOD enabled (see tty(4)) */
    tcgetattr(0, &termbuf);
    termbuf.c_lflag |= ECHO; /* if we use readline we dont want this */
    termbuf.c_oflag |= ONLCR | XTABS;
    termbuf.c_iflag |= ICRNL;
    termbuf.c_iflag &= ~IXOFF;
    /*termbuf.c_lflag &= ~ICANON;*/
    tcsetattr_stdin_TCSANOW(&termbuf);

    /* Uses FILE-based I/O to stdout, but does fflush_all(),
     * so should be safe with vfork.
     * I fear, though, that some users will have ridiculously big
     * issue files, and they may block writing to fd 1,
     * (parent is supposed to read it, but parent waits
     * for vforked child to exec!) */
    run();
    /* _exit is safer with vfork, and we shouldn't send message
     * to remote clients anyway */
    _exit(EXIT_SUCCESS); /*bb_perror_msg_and_die("execv %s", G.loginpath);*/
}

static void free_session(struct tsession *ts)
{
	struct tsession *t;

	/* Unlink this telnet session from the session list */
	t = G.sessions;
	if (t == ts)
		G.sessions = ts->next;
	else {
		while (t->next != ts)
			t = t->next;
		t->next = ts->next;
	}

	close(ts->ptyfd);
	close(ts->sockfd_read);
	/* We do not need to close(ts->sockfd_write), it's the same
	 * as sockfd_read unless we are in inetd mode. But in inetd mode
	 * we do not reach this */
	free(ts);

	/* Scan all sessions and find new maxfd */
	G.maxfd = 0;
	ts = G.sessions;
	while (ts) {
		if (G.maxfd < ts->ptyfd)
			G.maxfd = ts->ptyfd;
		if (G.maxfd < ts->sockfd_read)
			G.maxfd = ts->sockfd_read;

		ts = ts->next;
	}
}

int telnetd_start(void)
{
    fd_set rdfdset, wrfdset;
    int count;
    struct tsession *ts;
	int master_fd; /* for compiler */
    uint16_t portnbr;
    struct sockaddr_in client_addr;
    char client_ip[INET_ADDRSTRLEN];
    int addrlen = sizeof(client_addr);

    portnbr = CLID_PORT;
    master_fd = create_and_bind_stream(portnbr);

    xlisten(master_fd, 1);
    close_on_exec_on(master_fd);

    /* We don't want to die if just one session is broken */
    signal(SIGPIPE, SIG_IGN);
    signal(SIGCHLD, SIG_IGN);

/*
   This is how the buffers are used. The arrows indicate data flow.

   +-------+     wridx1++     +------+     rdidx1++     +----------+
   |       | <--------------  | buf1 | <--------------  |          |
   |       |     size1--      +------+     size1++      |          |
   |  pty  |                                            |  socket  |
   |       |     rdidx2++     +------+     wridx2++     |          |
   |       |  --------------> | buf2 |  --------------> |          |
   +-------+     size2++      +------+     size2--      +----------+

   size1: "how many bytes are buffered for pty between rdidx1 and wridx1?"
   size2: "how many bytes are buffered for socket between rdidx2 and wridx2?"

   Each session has got two buffers. Buffers are circular. If sizeN == 0,
   buffer is empty. If sizeN == BUFSIZE, buffer is full. In both these cases
   rdidxN == wridxN.
*/
    again:
    FD_ZERO(&rdfdset);
    FD_ZERO(&wrfdset);

    /* Select on the master socket, all telnet sockets and their
     * ptys if there is room in their session buffers.
     * NB: scalability problem: we recalculate entire bitmap
     * before each select. Can be a problem with 500+ connections. */
    ts = G.sessions;
    while (ts) {
        struct tsession *next = ts->next; /* in case we free ts */
        if (ts->shell_pid == -1) {
            /* Child died and we detected that */
            free_session(ts);
        } else {
            if (ts->size1 > 0)       /* can write to pty */
                FD_SET(ts->ptyfd, &wrfdset);
            if (ts->size1 < BUFSIZE) /* can read from socket */
                FD_SET(ts->sockfd_read, &rdfdset);
            if (ts->size2 > 0)       /* can write to socket */
                FD_SET(ts->sockfd_write, &wrfdset);
            if (ts->size2 < BUFSIZE) /* can read from pty */
                FD_SET(ts->ptyfd, &rdfdset);
        }
        ts = next;
    }

    FD_SET(master_fd, &rdfdset);
    /* This is needed because free_session() does not
     * take master_fd into account when it finds new
     * maxfd among remaining fd's */
    if (master_fd > G.maxfd)
        G.maxfd = master_fd;

    struct timeval *tv_ptr = NULL;
    count = select(G.maxfd + 1, &rdfdset, &wrfdset, NULL, tv_ptr);

    if (count == 0) /* "telnetd -w SEC" timed out */
        return 0;
    if (count < 0)
        goto again; /* EINTR or ENOMEM */

    /* Check for and accept new sessions */
	if (FD_ISSET(master_fd, &rdfdset)) {
		int fd;
		struct tsession *new_ts;

		fd = accept(master_fd, (struct sockaddr *)&client_addr, (socklen_t *)&addrlen);
		if (fd < 0)
			goto again;
		close_on_exec_on(fd);

        inet_ntop (AF_INET, &(client_addr.sin_addr), client_ip, INET_ADDRSTRLEN);

        if (strcmp(client_ip, "127.0.0.1") == 0) greeting = "BMC> ";
        else if (strcmp(client_ip, "192.168.3.10") == 0) greeting = "CPU> ";

		/* Create a new session and link it into active list */
		new_ts = make_new_session(fd);
		if (new_ts) {
			new_ts->next = G.sessions;
			G.sessions = new_ts;
		} else {
			close(fd);
		}
	}

    /* Then check for data tunneling */
    ts = G.sessions;
    while (ts) { /* For all sessions... */
        struct tsession *next = ts->next; /* in case we free ts */

        if (/*ts->size1 &&*/ FD_ISSET(ts->ptyfd, &wrfdset)) {
            /* Write to pty from buffer 1 */
            count = safe_write_to_pty_decode_iac(ts);
            if (count < 0) {
                if (errno == EAGAIN)
                    goto skip1;
                goto kill_session;
            }
        }
        skip1:
        if (/*ts->size2 &&*/ FD_ISSET(ts->sockfd_write, &wrfdset)) {
            /* Write to socket from buffer 2 */
            count = MIN(BUFSIZE - ts->wridx2, ts->size2);
            count = safe_write_double_iac(ts->sockfd_write, (void*)(TS_BUF2(ts) + ts->wridx2), count);
            if (count < 0) {
                if (errno == EAGAIN)
                    goto skip2;
                goto kill_session;
            }
            ts->wridx2 += count;
            if (ts->wridx2 >= BUFSIZE) /* actually == BUFSIZE */
                ts->wridx2 = 0;
            ts->size2 -= count;
            if (ts->size2 == 0) {
                ts->rdidx2 = 0;
                ts->wridx2 = 0;
            }
        }
        skip2:

        if (/*ts->size1 < BUFSIZE &&*/ FD_ISSET(ts->sockfd_read, &rdfdset)) {
            /* Read from socket to buffer 1 */
            count = MIN(BUFSIZE - ts->rdidx1, BUFSIZE - ts->size1);
            count = safe_read(ts->sockfd_read, TS_BUF1(ts) + ts->rdidx1, count);
            if (count <= 0) {
                if (count < 0 && errno == EAGAIN)
                    goto skip3;
                goto kill_session;
            }
            /* Ignore trailing NUL if it is there */
            if (!TS_BUF1(ts)[ts->rdidx1 + count - 1]) {
                --count;
            }
            ts->size1 += count;
            ts->rdidx1 += count;
            if (ts->rdidx1 >= BUFSIZE) /* actually == BUFSIZE */
                ts->rdidx1 = 0;
        }
        skip3:
        if (/*ts->size2 < BUFSIZE &&*/ FD_ISSET(ts->ptyfd, &rdfdset)) {
            /* Read from pty to buffer 2 */
            int eio = 0;
            read_pty:
            count = MIN(BUFSIZE - ts->rdidx2, BUFSIZE - ts->size2);
            count = safe_read(ts->ptyfd, TS_BUF2(ts) + ts->rdidx2, count);
            if (count <= 0) {
                if (count < 0) {
                    if (errno == EAGAIN)
                        goto skip4;
                    /* login process might call vhangup(),
                     * which causes intermittent EIOs on read above
                     * (observed on kernel 4.12.0). Try up to 10 ms.
                     */
                    if (errno == EIO && eio < 10) {
                        eio++;
                        //bb_error_msg("EIO pty %u", eio);
                        usleep(1000);
                        goto read_pty;
                    }
                }
                goto kill_session;
            }
            ts->size2 += count;
            ts->rdidx2 += count;
            if (ts->rdidx2 >= BUFSIZE) /* actually == BUFSIZE */
                ts->rdidx2 = 0;
        }
        skip4:
        ts = next;
        continue;
        kill_session:
        free_session(ts);
        ts = next;
    }

    goto again;
}
