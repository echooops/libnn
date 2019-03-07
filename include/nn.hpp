#pragma once
#include <nanomsg/nn.h>
#include <nanomsg/pair.h>
#include <nanomsg/bus.h>
#include <nanomsg/pipeline.h>
#include <nanomsg/reqrep.h>
#include <nanomsg/pubsub.h>
#include <nanomsg/survey.h>

#include <cassert>
#include <cstring>
#include <algorithm>
#include <exception>

#if defined __GNUC__
#define nn_slow(x) __builtin_expect ((x), 0)
#else
#define nn_slow(x) (x)
#endif

// 为 nanomsg 提供一个 C++ 封装
namespace nn
{
    // nanomsg 自定义异常
    class exception : public std::exception
    {
    public:

        exception () : err (nn_errno ()) {}

        virtual const char *what () const throw ()
        {
            return nn_strerror (err);
        }

        int num () const
        {
            return err;
        }

    private:

        int err;
    };
    // 获取 nanomsg 定义的宏符号和对应的值
    inline const char *symbol (int i, int *value)
    {
        return nn_symbol (i, value);
    }

    inline void *allocmsg (size_t size, int type)
    {
        void *msg = nn_allocmsg (size, type);
        if (nn_slow (!msg))
            throw nn::exception ();
        return msg;
    }
    
    inline void *reallocmsg (void * msg, size_t size)
    {
        void *newmsg = nn_reallocmsg(msg, size);
        if (nn_slow (!msg))
            throw  nn::exception ();
        return newmsg;
    }
    
    inline int freemsg (void *msg)
    {
        int rc = nn_freemsg (msg);
        if (nn_slow (rc != 0))
            throw nn::exception ();
        return rc;
    }
    // 宕机
    inline void term ()
    {
        nn_term ();
    }
    

    // TODO: bind connect 对返回值 endpoint 仍需考虑
    // TODO: 移动构造移动复制，需考虑
    class socket
    {
        // 友元函数声明
        friend int device(socket &s1, socket &s2);
    public:
        // domain AF_SP AF_SP_RAW
        socket (int domain, int protocol)
        {
            sock_ = nn_socket (domain, protocol);
            if (nn_slow (sock_ < 0))
                throw nn::exception ();
        }
        ~socket ()
        {
            int rc = nn_close (sock_);
            assert (rc == 0);
        }

        //! 移动构造
        socket (socket &&other) noexcept : sock_(other.sock_) { other.sock_ = 0; }
        //! 移动赋值
        socket& operator=(socket &&other) noexcept
        {
            sock_ = other.sock_;
            other.sock_ = 0;
            return *this;
        }

        /*  Prevent making copies of the socket by accident. */
        socket (const socket&) = delete;
        void operator = (const socket&) = delete;

    public:                     // 功能函数
        inline void setsockopt (int level, int option, const void *optval,
                                size_t optvallen)
        {
            int rc = nn_setsockopt (sock_, level, option, optval, optvallen);
            if (nn_slow (rc != 0))
                throw nn::exception ();
        }

        inline void getsockopt (int level, int option, void *optval,
                                size_t *optvallen)
        {
            int rc = nn_getsockopt (sock_, level, option, optval, optvallen);
            if (nn_slow (rc != 0))
                throw nn::exception ();
        }

        inline int bind (const char *addr)
        {
            int rc = nn_bind (sock_, addr);
            if (nn_slow (rc < 0))
                throw nn::exception ();
            return rc;
        }

        inline int connect (const char *addr)
        {
            int rc = nn_connect (sock_, addr);
            if (nn_slow (rc < 0))
                throw nn::exception ();
            return rc;
        }

        inline void shutdown (int how)
        {
            int rc = nn_shutdown (sock_, how);
            if (nn_slow (rc != 0))
                throw nn::exception ();
        }

        inline int send (const void *buf, size_t len, int flags)
        {
            int rc = nn_send (sock_, buf, len, flags);
            if (nn_slow (rc < 0)) {
                if (nn_slow (nn_errno () != EAGAIN))
                    throw nn::exception ();
                return -1;
            }
            return rc;
        }
        
        inline int recv (void *buf, size_t len, int flags)
        {
            int rc = nn_recv (sock_, buf, len, flags);
            if (nn_slow (rc < 0)) {
                if (nn_slow (nn_errno () != EAGAIN))
                    throw nn::exception ();
                return -1;
            }
            return rc;
        }

        inline int sendmsg (const struct nn_msghdr *msghdr, int flags)
        {
            int rc = nn_sendmsg (sock_, msghdr, flags);
            if (nn_slow (rc < 0)) {
                if (nn_slow (nn_errno () != EAGAIN))
                    throw nn::exception ();
                return -1;
            }
            return rc;
        }

        inline int recvmsg (struct nn_msghdr *msghdr, int flags)
        {
            int rc = nn_recvmsg (sock_, msghdr, flags);
            if (nn_slow (rc < 0)) {
                if (nn_slow (nn_errno () != EAGAIN))
                    throw nn::exception ();
                return -1;
            }
            return rc;
        }
        
        inline int device (const socket &s)
        {
            int rc = nn_device (sock_, s.sock_);
            if (nn_slow (rc < 0)) {
                if (nn_slow (nn_errno () != EAGAIN))
                    throw nn::exception ();
                return -1;
            }
            return rc;
        }

    private:
        int sock_ = 0;
    };

    inline int device (socket &s1, socket&s2)
    {
        int rc = nn_device (s1.sock_, s2.sock_);
        if (nn_slow (rc < 0)) {
            if (nn_slow (nn_errno () != EAGAIN))
                throw nn::exception ();
            return -1;
        }
        return rc;
    }
}

#undef nn_slow
