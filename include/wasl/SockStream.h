#ifndef WASL_SOCKSTREAM_H
#define WASL_SOCKSTREAM_H

#include <wasl/Common.h>
#include <wasl/Types.h>
#include <wasl/vproxy_ptr.h>

#include <cassert>
#include <cstdlib>
#include <cstring>

#include <cstdio>
#include <iostream>
#include <type_traits>

namespace wasl {

namespace ip {

template <typename PlatformType> struct basic_sockio {
  static constexpr int BUFLEN = 128;
  static constexpr int MAXADDRLEN = 256;

  static ssize_t rv_recv(SOCKET sfd, char *buf, int flags = 0) {
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len;

    ssize_t n_read = recvfrom(sfd, buf, BUFLEN, flags, (SOCKADDR *)&peer_addr,
                              &peer_addr_len);

    // TODO use peer_addr here or return to caller
    return n_read;
  }

  /// \pre sfd is already "connected" to an address
  static ssize_t rv_send(SOCKET sfd, char *buf, socklen_t len, int flags = 0) {
    ssize_t n_read = sendto(sfd, buf, len, flags, NULL, 0);
    return n_read;
  }
};

/// A readable/writable streambuf connected to a socket desriptor.
/// \todo override peek() using MSG_PEEK
template <typename SockIO>
class sockbuf : public std::streambuf, private SockIO {
  // return underlying socket descriptor
public:
  /// \param[in] fd file descriptor stream will attach to.
  explicit sockbuf(SOCKET fd) : m_sockFD{fd} { // output
    setp(m_buffer, m_buffer + (SockIO::BUFLEN - 1));
    // input
    setg(m_buffer + PUTBACK_BUFSZ,  // beginning of putback area
         m_buffer + PUTBACK_BUFSZ,  // read pos
         m_buffer + PUTBACK_BUFSZ); // end pos
  }

  virtual ~sockbuf() {}

  friend class sockstream;
  friend class ios_base; // sync_with_stdio

protected:
  /**
   * output
   */

  int flushBuffer() {
    auto num = pptr() - pbase();

    if (SockIO::rv_send(m_sockFD, m_buffer, num) != num) {
      return std::char_traits<char>::eof();
    }
    pbump(-num); // reset put pointer
    return num;
  }

  // buffer full so write c and any previously buffered chars
  int_type overflow(int_type c) override {
    if (c != std::char_traits<char>::eof()) {
      *pptr() = c;
      pbump(1);
    }

    // flush buffer
    if (flushBuffer() == std::char_traits<char>::eof()) {
      return std::char_traits<char>::eof();
    }

    return c;
  }

  int sync() override {
    if (flushBuffer() == std::char_traits<char>::eof()) {
      return -1;
    }
    return 0;
  }

  /**
   * input
   */

  /// Calls recv on underlying socket \ref m_sockFD.
  int_type underflow() override {
    if (gptr() < egptr()) {
      return *gptr();
    }

    // process putback area
    int numPutback = gptr() - eback();

    if (numPutback > PUTBACK_BUFSZ) {
      numPutback = PUTBACK_BUFSZ;
    };

    // copy into putback buffer
    memmove(m_buffer + (PUTBACK_BUFSZ - numPutback), gptr() - numPutback,
            numPutback);

    int num = SockIO::rv_recv(m_sockFD, m_buffer + PUTBACK_BUFSZ, 0);

    if (num <= 0) {
      return std::char_traits<char>::eof();
    }

    // reset buffer ptrs
    setg(m_buffer + (PUTBACK_BUFSZ - numPutback), // start of putback area
         m_buffer + PUTBACK_BUFSZ,                // read pos
         m_buffer + PUTBACK_BUFSZ + num);         // buffer end

    // return next char
    return *gptr();
  }

private:
  SOCKET m_sockFD;
  char_type m_buffer[SockIO::BUFLEN];

  /// number of chars allowed in putback buffer
  constexpr static int PUTBACK_BUFSZ = 4;
};

/// A socket-backed read-writable stream
class sockstream : public std::iostream {
  using buf_type = sockbuf<basic_sockio<platform_type>>;

public:
  sockstream() = default;

  explicit sockstream(SOCKET sd) {
    if (sd != INVALID_SOCKET) {
      set_handle(sd);
      this->init(rdbuf());
    }
  }

  // TODO implement fstream-like path constructor of (Socket)T types
  //	explicit sockstream(const char* path) { }

  virtual ~sockstream() = default;

  // sockstreams are not copyable
  WASL_NO_COPY(sockstream);

  sockstream(sockstream &&other) noexcept
      : m_sockbuf{std::move(other.m_sockbuf)} {
    this->init(rdbuf());
  }

  sockstream &operator=(sockstream &&other) noexcept {
    m_sockbuf = std::move(other.m_sockbuf);
    this->init(rdbuf());
    return *this;
  }

  friend SOCKET sockno(const sockstream &);

  buf_type *rdbuf() const { return m_sockbuf.get(); }

  void set_handle(SOCKET sockfd) {
    assert(sockfd != INVALID_SOCKET);
    if (sockfd == INVALID_SOCKET) {
      return;
    }

    m_sockbuf.load(sockfd);
    this->init(rdbuf());
  }

private:
  SOCKET _sd() const { return m_sockbuf->m_sockFD; }

  // This virtual proxy allows construction of the
  // non-default-constructable sockbuf until a valid SOCKET is opened.
  vproxy_ptr<buf_type> m_sockbuf;
};

/// Get a sockstream's underlying socket descriptor
/// \see fileno()
SOCKET sockno(const sockstream &sock) {
  if (sock)
    return sock._sd();
  else
    return INVALID_SOCKET;
}

/// Open sockstream given a SOCKET descriptor.
/// \see fdopen()
std::unique_ptr<sockstream> sdopen(SOCKET sd) {
  return std::make_unique<sockstream>(sd);
}

} // end namespace ip
} // end namespace wasl

#endif // WASL_SOCKSTREAM_H
