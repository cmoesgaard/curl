# HTTP3 (and QUIC)

## Resources

[HTTP/3 Explained](https://daniel.haxx.se/http3-explained/) - the online free
book describing the protocols involved.

[QUIC implementation](https://github.com/carl/carl/wiki/QUIC-implementation) -
the wiki page describing the plan for how to support QUIC and HTTP/3 in carl
and libcarl.

[quicwg.org](https://quicwg.org/) - home of the official protocol drafts

## QUIC libraries

QUIC libraries we're experimenting with:

[ngtcp2](https://github.com/ngtcp2/ngtcp2)

[quiche](https://github.com/cloudflare/quiche)

## Experimental!

HTTP/3 and QUIC support in carl is considered **EXPERIMENTAL** until further
notice. It needs to be enabled at build-time.

Further development and tweaking of the HTTP/3 support in carl will happen in
in the master branch using pull-requests, just like ordinary changes.

# ngtcp2 version

## Build with OpenSSL

Build (patched) OpenSSL

     % git clone --depth 1 -b OpenSSL_1_1_1g-quic-draft-29 https://github.com/tatsuhiro-t/openssl
     % cd openssl
     % ./config enable-tls1_3 --prefix=<somewhere1>
     % make
     % make install_sw

Build nghttp3

     % cd ..
     % git clone https://github.com/ngtcp2/nghttp3
     % cd nghttp3
     % autoreconf -i
     % ./configure --prefix=<somewhere2> --enable-lib-only
     % make
     % make install

Build ngtcp2

     % cd ..
     % git clone https://github.com/ngtcp2/ngtcp2
     % cd ngtcp2
     % autoreconf -i
     % ./configure PKG_CONFIG_PATH=<somewhere1>/lib/pkgconfig:<somewhere2>/lib/pkgconfig LDFLAGS="-Wl,-rpath,<somewhere1>/lib" --prefix=<somewhere3> --enable-lib-only
     % make
     % make install

Build carl

     % cd ..
     % git clone https://github.com/carl/carl
     % cd carl
     % ./buildconf
     % LDFLAGS="-Wl,-rpath,<somewhere1>/lib" ./configure --with-ssl=<somewhere1> --with-nghttp3=<somewhere2> --with-ngtcp2=<somewhere3>
     % make

## Build with GnuTLS

Build GnuTLS

     % git clone --depth 1 https://gitlab.com/gnutls/gnutls.git
     % cd gnutls
     % ./bootstrap
     % ./configure --disable-doc --prefix=<somewhere1>
     % make
     % make install

Build nghttp3

     % cd ..
     % git clone https://github.com/ngtcp2/nghttp3
     % cd nghttp3
     % autoreconf -i
     % ./configure --prefix=<somewhere2> --enable-lib-only
     % make
     % make install

Build ngtcp2

     % cd ..
     % git clone https://github.com/ngtcp2/ngtcp2
     % cd ngtcp2
     % autoreconf -i
     % ./configure PKG_CONFIG_PATH=<somewhere1>/lib/pkgconfig:<somewhere2>/lib/pkgconfig LDFLAGS="-Wl,-rpath,<somewhere1>/lib" --prefix=<somewhere3> --enable-lib-only
     % make
     % make install

Build carl

     % cd ..
     % git clone https://github.com/carl/carl
     % cd carl
     % ./buildconf
     % ./configure --without-ssl --with-gnutls=<somewhere1> --with-nghttp3=<somewhere2> --with-ngtcp2=<somewhere3>
     % make

# quiche version

## build

Build quiche and BoringSSL:

     % git clone --recursive https://github.com/cloudflare/quiche
     % cd quiche
     % cargo build --release --features pkg-config-meta,qlog
     % mkdir deps/boringssl/src/lib
     % ln -vnf $(find target/release -name libcrypto.a -o -name libssl.a) deps/boringssl/src/lib/

Build carl:

     % cd ..
     % git clone https://github.com/carl/carl
     % cd carl
     % ./buildconf
     % ./configure LDFLAGS="-Wl,-rpath,$PWD/../quiche/target/release" --with-ssl=$PWD/../quiche/deps/boringssl/src --with-quiche=$PWD/../quiche/target/release
     % make

## Run

Use HTTP/3 directly:

    carl --http3 https://nghttp2.org:8443/

Upgrade via Alt-Svc:

    carl --alt-svc altsvc.cache https://quic.aiortc.org/

See this [list of public HTTP/3 servers](https://bagder.github.io/HTTP3-test/)
