#pragma once

#if defined __cplusplus

#ifdef WIN32
// Required due to --layout=tagged being added to install_boost.py
#define BOOST_AUTO_LINK_TAGGED 1
#endif

#if defined(Q_OS_MAC)
// Avoid warnings from boost libraries on Mac.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#pragma clang diagnostic ignored "-Wshorten-64-to-32"
#endif

#include <boost/asio.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/logic/tribool.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/serialization/vector.hpp>

typedef boost::shared_ptr<boost::asio::ip::tcp::socket> socket_ptr;

using namespace boost::placeholders;

#if defined(Q_OS_MAC)
#pragma clang diagnostic pop
#endif

#endif
