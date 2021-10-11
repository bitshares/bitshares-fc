#include <fc/crypto/openssl.hpp>

#include <fc/filesystem.hpp>

#include <boost/filesystem/path.hpp>

#include <cstdlib>
#include <string>
#include <stdlib.h>

namespace  fc 
{
    struct openssl_scope
    {
       static path & _configurationFilePath()
       {
          static path orderindependentstorage;
          return orderindependentstorage;
       }
       openssl_scope()
       {
          ERR_load_crypto_strings(); 
          OpenSSL_add_all_algorithms();

          const boost::filesystem::path& boostPath = _configurationFilePath();
          if(boostPath.empty() == false)
          {
            std::string varSetting("OPENSSL_CONF=");
            varSetting += _configurationFilePath().to_native_ansi_path();
#if defined(WIN32)
            _putenv((char*)varSetting.c_str());
#else
            putenv((char*)varSetting.c_str());
#endif
          }
#if OPENSSL_VERSION_NUMBER < 0x10100000L
         // no longer needed as of OpenSSL 1.1
         // if special initialization is necessary in versions 1.1 and above,
         // use OPENSSL_init_crypto
          OPENSSL_config(nullptr);
#endif
       }

       ~openssl_scope()
       {
#if not defined(LIBRESSL_VERSION_NUMBER)
          // No FIPS in LibreSSL.
          // https://marc.info/?l=openbsd-misc&m=139819485423701&w=2
#if OPENSSL_VERSION_NUMBER < 0x30000000L
          // FIPS_mode_set() is removed in OpenSSL v3
          FIPS_mode_set(0);
#endif
#endif
          CONF_modules_unload(1);
          EVP_cleanup();
          CRYPTO_cleanup_all_ex_data();
          ERR_free_strings();
       }
    };

    void store_configuration_path(const path& filePath)
    {
      openssl_scope::_configurationFilePath() = filePath;
    }
   
    int init_openssl()
    {
      static openssl_scope ossl;
      return 0;
    }

    #define SSL_TYPE_IMPL(name, ssl_type, free_func) \
            name::name( ssl_type* obj ) : ssl_wrapper(obj) {} \
            name::name( name&& move ) : ssl_wrapper( move.obj ) \
            { \
               move.obj = nullptr; \
            } \
            name::~name() \
            { \
               if( obj != nullptr ) \
                  free_func(obj); \
            } \
            name& name::operator=( name&& move ) \
            { \
               if( this != &move ) \
               { \
                  if( obj != nullptr ) \
                     free_func(obj); \
                  obj = move.obj; \
                  move.obj = nullptr; \
               } \
               return *this; \
            }

    SSL_TYPE_IMPL(ec_group,       EC_GROUP,       EC_GROUP_free)
    SSL_TYPE_IMPL(ec_point,       EC_POINT,       EC_POINT_free)
    SSL_TYPE_IMPL(ecdsa_sig,      ECDSA_SIG,      ECDSA_SIG_free)
    SSL_TYPE_IMPL(bn_ctx,         BN_CTX,         BN_CTX_free)
    SSL_TYPE_IMPL(evp_cipher_ctx, EVP_CIPHER_CTX, EVP_CIPHER_CTX_free )
    SSL_TYPE_IMPL(ssl_dh,         DH,             DH_free)
}
