cmake_minimum_required(VERSION 3.20.0)

project(DiplomWEBCrawler)

configure_file(../Ini/config.ini
        ${CMAKE_CURRENT_BINARY_DIR} COPYONLY)

add_executable(DiplomWEBCrawler WebCrawler.cpp)

target_compile_features(DiplomWEBCrawler PRIVATE cxx_std_17)

include(../CMake/CPM.cmake)

CPMAddPackage(
  NAME Boost
  VERSION 1.86.0 
  URL https://github.com/boostorg/boost/releases/download/boost-1.86.0/boost-1.86.0-cmake.tar.xz
  URL_HASH SHA256=2c5ec5edcdff47ff55e27ed9560b0a0b94b07bd07ed9928b476150e16b0efc57
  OPTIONS "BOOST_ENABLE_CMAKE ON" "BOOST_SKIP_INSTALL_RULES ON" # Set `OFF` for installation
          "BUILD_SHARED_LIBS OFF" "BOOST_INCLUDE_LIBRARIES container\\\;asio\\\;locale\\\;beast" # Note the escapes!
)

CPMAddPackage(
  NAME pqxx
  VERSION 7.10.0
  URL https://github.com/jtv/libpqxx/archive/refs/tags/7.10.0.tar.gz
  OPTIONS "BUILD_SHARED_LIBS OFF"
)

set (OPENSSL_ROOT_DIR "c:\\Program Files\\OpenSSL-Win64")

find_package(OpenSSL REQUIRED COMPONENTS Crypto SSL)
if (OPENSSL_FOUND)
	message("!!OpenSSL include dir: ${OPENSSL_INCLUDE_DIR}")
	message("!!OpenSSL libraries: ${OPENSSL_LIBRARIES}")
endif()



target_link_libraries(DiplomWEBCrawler PRIVATE 
    Boost::asio 
    Boost::container 
    Boost::beast
    Boost::locale
    OpenSSL::Crypto
    OpenSSL::SSL
    pqxx)



