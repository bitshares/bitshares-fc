#include <boost/test/unit_test.hpp>

#include <fc/network/ip.hpp>
#include <fc/network/tcp_socket.hpp>
#include <fc/variant.hpp>

BOOST_AUTO_TEST_SUITE(fc_network)

BOOST_AUTO_TEST_CASE(tcp_server_reuse_address_test)
{
   for( uint16_t i = 0; i <= 1; ++i )
   {
      for( uint16_t j = 0; j <= 1; ++j )
      {
         idump( (i)(j) );

         fc::ip::endpoint ep1 = fc::ip::endpoint::from_string( std::string( "0.0.0.0:0" ) );

         fc::tcp_server server1;
         if( 0 == i )
            server1.set_reuse_address();
         server1.listen( ep1 );

         auto port1 = server1.get_port();
         ilog( "Port = ${p}", ("p", port1) );
         fc::ip::endpoint ep2 = fc::ip::endpoint::from_string( std::string( "0.0.0.0:" ) + fc::to_string( port1 ) );
         fc::ip::endpoint ep3 = fc::ip::endpoint::from_string( std::string( "127.0.0.1:" ) + fc::to_string( port1 ) );

         fc::tcp_server server2a;
         if( 0 == j )
            server2a.set_reuse_address();
         BOOST_CHECK_THROW( server2a.listen( ep2 ), fc::exception );

         fc::tcp_server server3a;
         if( 0 == j )
            server3a.set_reuse_address();
         BOOST_CHECK_THROW( server3a.listen( ep3 ), fc::exception );
      }
   }
}

BOOST_AUTO_TEST_SUITE_END()
