#pragma once
#include <regex>
#include <iostream>

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <openssl/ssl.h>

#include "Utils.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;

using tcp = boost::asio::ip::tcp;



std::string getContent(const URL& url)
{

	std::string result;

	try
	{
		std::string host = url.host;
		std::string query = url.query;

		// The io_context is required for all I/O
		net::io_context ioc;

		if (url.protocol == TProtocol::HTTPS)
		{

			ssl::context ctx(ssl::context::tlsv13_client);
			ctx.set_default_verify_paths();

			beast::ssl_stream<beast::tcp_stream> stream(ioc, ctx);
			stream.set_verify_mode(ssl::verify_none);

			stream.set_verify_callback([](bool preverified, ssl::verify_context& ctx) {
				return true; // Accept any certificate
				});


			if (!SSL_set_tlsext_host_name(stream.native_handle(), host.c_str())) {
				beast::error_code ec{ static_cast<int>(::ERR_get_error()), net::error::get_ssl_category() };
				throw beast::system_error{ ec };
			}

			ip::tcp::resolver resolver(ioc);
			get_lowest_layer(stream).connect(resolver.resolve({ host, "https" }));
			get_lowest_layer(stream).expires_after(std::chrono::seconds(30));


			http::request<http::empty_body> req{ http::verb::get, query, 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			stream.handshake(ssl::stream_base::client);
			http::write(stream, req);

			beast::flat_buffer buffer;
			http::response<http::dynamic_body> res;
			http::read(stream, buffer, res);

			result = buffers_to_string(res.body().data());

			beast::error_code ec;
			stream.shutdown(ec);
			if (ec == net::error::eof) {
				ec = {};
			}

			if (ec) {
				throw beast::system_error{ ec };
			}
		}
		else
		{
			// These objects perform our I/O
			tcp::resolver resolver(ioc);
			beast::tcp_stream stream(ioc);

			// Look up the domain name
			auto const results = resolver.resolve(host, "http");

			// Make the connection on the IP address we get from a lookup
			stream.connect(results);

			// Set up an HTTP GET request message
			http::request<http::string_body> req{ http::verb::get, query, 11 };
			req.set(http::field::host, host);
			req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

			// Send the HTTP request to the remote host
			http::write(stream, req);

			// This buffer is used for reading and must be persisted
			beast::flat_buffer buffer;

			// Declare a container to hold the response
			http::response<http::dynamic_body> res;

			// Receive the HTTP response
			http::read(stream, buffer, res);

			result = buffers_to_string(res.body().data());

			// Gracefully close the socket
			beast::error_code ec;
			stream.socket().shutdown(tcp::socket::shutdown_both, ec);

			if (ec && ec != beast::errc::not_connected)
				throw beast::system_error{ ec };

		}
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}

	return result;
}