/*
 * Copyright (C) 2001-2016 Jacek Sieka, arnetheduck on gmail point com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "stdinc.h"
#include "SSLSocket.h"

#include "LogManager.h"
#include "SettingsManager.h"
#include "ResourceManager.h"
#include "format.h"
#include "StringTokenizer.h"

#include <openssl/err.h>

SSLSocket::SSLSocket(CryptoManager::SSLContext context, bool allowUntrusted, const string& expKP) : SSLSocket(context)
{
	verifyData.reset(new CryptoManager::SSLVerifyData(allowUntrusted, expKP));
}
SSLSocket::SSLSocket(CryptoManager::SSLContext context) : /*Socket(/*TYPE_TCP), */ctx(NULL), ssl(NULL), verifyData(nullptr)
{
	ctx = CryptoManager::getInstance()->getSSLContext(context);
}

void SSLSocket::connect(const string& aIp, uint16_t aPort)
{
	Socket::connect(aIp, aPort);
	
	waitConnected(0);
}

bool SSLSocket::waitConnected(uint64_t millis)
{
	if (!ssl)
	{
		if (!Socket::waitConnected(millis))
		{
			return false;
		}
		ssl.reset(SSL_new(ctx));
		if (!ssl)
			checkSSL(-1);
			
		if (!verifyData)
		{
			SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);
		}
		else SSL_set_ex_data(ssl, CryptoManager::idxVerifyData, verifyData.get());
		
		checkSSL(SSL_set_fd(ssl, static_cast<int>(getSock())));
	}
	
	if (SSL_is_init_finished(ssl))
	{
		return true;
	}
	
	while (true)
	{
		int ret = ssl->server ? SSL_accept(ssl) : SSL_connect(ssl);
		if (ret == 1)
		{
			dcdebug("Connected to SSL server using %s as %s\n", SSL_get_cipher(ssl), ssl->server ? "server" : "client");
			return true;
		}
		if (!waitWant(ret, millis))
		{
			return false;
		}
	}
}

uint16_t SSLSocket::accept(const Socket& listeningSocket)
{
	auto ret = Socket::accept(listeningSocket);
	
	waitAccepted(0);
	
	return ret;
}

bool SSLSocket::waitAccepted(uint64_t millis)
{
	if (!ssl)
	{
		if (!Socket::waitAccepted(millis))
		{
			return false;
		}
		ssl.reset(SSL_new(ctx));
		if (!ssl)
			checkSSL(-1);
			
		if (!verifyData)
		{
			SSL_set_verify(ssl, SSL_VERIFY_NONE, NULL);
		}
		else SSL_set_ex_data(ssl, CryptoManager::idxVerifyData, verifyData.get());
		
		checkSSL(SSL_set_fd(ssl, static_cast<int>(getSock())));
	}
	
	if (SSL_is_init_finished(ssl))
	{
		return true;
	}
	
	while (true)
	{
		int ret = SSL_accept(ssl);
		if (ret == 1)
		{
			dcdebug("Connected to SSL client using %s\n", SSL_get_cipher(ssl));
			return true;
		}
		if (!waitWant(ret, millis))
		{
			return false;
		}
	}
}

bool SSLSocket::waitWant(int ret, uint64_t millis)
{
	int err = SSL_get_error(ssl, ret);
	switch (err)
	{
		case SSL_ERROR_WANT_READ:
			return wait(millis, Socket::WAIT_READ) == WAIT_READ;
		case SSL_ERROR_WANT_WRITE:
			return wait(millis, Socket::WAIT_WRITE) == WAIT_WRITE;
			// Check if this is a fatal error...
		default:
			checkSSL(ret);
	}
	dcdebug("SSL: Unexpected fallthrough");
	dcassert(0);
	// There was no error?
	return true;
}

int SSLSocket::read(void* aBuffer, int aBufLen)
{
	if (!ssl)
	{
		return -1;
	}
	int len = checkSSL(SSL_read(ssl, aBuffer, aBufLen));
	
	if (len > 0)
	{
		g_stats.m_ssl.totalDown += len;
		//dcdebug("In(s): %.*s\n", len, (char*)aBuffer);
	}
	return len;
}

int SSLSocket::write(const void* aBuffer, int aLen)
{
	if (!ssl)
	{
		return -1;
	}
	int ret = checkSSL(SSL_write(ssl, aBuffer, aLen));
	if (ret > 0)
	{
		g_stats.m_ssl.totalUp += ret;
		//dcdebug("Out(s): %.*s\n", ret, (char*)aBuffer);
	}
	return ret;
}

int SSLSocket::checkSSL(int ret)
{
	if (!ssl)
	{
		return -1;
	}
	if (ret <= 0)
	{
		/* inspired by boost.asio (asio/ssl/detail/impl/engine.ipp, function engine::perform) and
		the SSL_get_error doc at <https://www.openssl.org/docs/ssl/SSL_get_error.html>. */
		auto err = SSL_get_error(ssl, ret);
		switch (err)
		{
			case SSL_ERROR_NONE:        // Fallthrough - YaSSL doesn't for example return an openssl compatible error on recv fail
			case SSL_ERROR_WANT_READ:   // Fallthrough
			case SSL_ERROR_WANT_WRITE:
				return -1;
			case SSL_ERROR_ZERO_RETURN:
				throw SocketException(STRING(CONNECTION_CLOSED));
			case SSL_ERROR_SYSCALL:
			{
				auto sys_err = ERR_get_error();
				if (sys_err == 0)
				{
					if (ret == 0)
					{
						dcdebug("TLS error: call ret = %d, SSL_get_error = %d, ERR_get_error = " U64_FMT "\n", ret, err, sys_err);
						throw SSLSocketException(STRING(CONNECTION_CLOSED));
					}
					sys_err = getLastError();
				}
				throw SSLSocketException(sys_err);
			}
			default:
				//display the cert errors as first choice, if the error is not the certs display the error from the ssl.
				auto sys_err = ERR_get_error();
				string _error;
				int v_err = SSL_get_verify_result(ssl);
				if (v_err == X509_V_ERR_APPLICATION_VERIFICATION)
				{
					_error = "Keyprint mismatch";
				}
				else if (v_err != X509_V_OK)
				{
					_error = X509_verify_cert_error_string(v_err);
				}
				else
				{
					_error = ERR_error_string(sys_err, NULL);
				}
				dcdebug("TLS error: call ret = %d, SSL_get_error = %d, ERR_get_error = " U64_FMT ",ERROR string: %s \n", ret, err, sys_err, ERR_error_string(sys_err, NULL));
				throw SSLSocketException(STRING(TLS_ERROR) + (_error.empty() ? "" : + ": " + _error));
		}
	}
	return ret;
}

int SSLSocket::wait(uint64_t millis, int waitFor)
{
#ifdef HEADER_OPENSSLV_H
	if (ssl && (waitFor & Socket::WAIT_READ))
	{
		/** @todo Take writing into account as well if reading is possible? */
		char c;
		if (SSL_peek(ssl, &c, 1) > 0)
			return WAIT_READ;
	}
#endif
	return Socket::wait(millis, waitFor);
}

bool SSLSocket::isTrusted() const noexcept
{
    if (!ssl)
{
return false;
}

if (SSL_get_verify_result(ssl) != X509_V_OK)
{
return false;
}

X509* cert = SSL_get_peer_certificate(ssl);
if (!cert)
{
return false;
}

X509_free(cert);

return true;
}

bool SSLSocket::isKeyprintMatch() const noexcept
{
    if (ssl)
    return SSL_get_verify_result(ssl) != X509_V_ERR_APPLICATION_VERIFICATION;
    
    return true;
}

std::string SSLSocket::getEncryptionInfo() const noexcept
	{
	    if (!ssl)
	    return Util::emptyString;
	    
	    string cipher = SSL_get_cipher_name(ssl);
	    string protocol = SSL_get_version(ssl);
	    return protocol + " / " + cipher;
	}
	
	ByteVector SSLSocket::getKeyprint() const noexcept
		{
		    if (!ssl)
		    return ByteVector();
		    X509* x509 = SSL_get_peer_certificate(ssl);
		    
		    if (!x509)
			    return ByteVector();
			    
			    ByteVector res = ssl::X509_digest(x509, EVP_sha256());
			    
			    X509_free(x509);
			    return res;
			}
			
			bool SSLSocket::verifyKeyprint(const string& expKP, bool allowUntrusted) noexcept
			{
				if (!ssl)
					return true;
					
				if (expKP.empty() || expKP.find("/") == string::npos)
					return allowUntrusted;
					
				verifyData.reset(new CryptoManager::SSLVerifyData(allowUntrusted, expKP));
				SSL_set_ex_data(ssl, CryptoManager::idxVerifyData, verifyData.get());
				
				SSL_CTX* ssl_ctx = SSL_get_SSL_CTX(ssl);
				X509_STORE* store = SSL_CTX_get_cert_store(ctx);
				
				bool result = false;
				int err = SSL_get_verify_result(ssl);
				if (ssl_ctx && store)
				{
					X509_STORE_CTX* vrfy_ctx = X509_STORE_CTX_new();
					X509* cert = SSL_get_peer_certificate(ssl);
					if (vrfy_ctx && cert && X509_STORE_CTX_init(vrfy_ctx, store, cert, SSL_get_peer_cert_chain(ssl)))
					{
						auto vrfy_cb = SSL_CTX_get_verify_callback(ssl_ctx);
						
						X509_STORE_CTX_set_ex_data(vrfy_ctx, SSL_get_ex_data_X509_STORE_CTX_idx(), ssl);
						X509_STORE_CTX_set_verify_cb(vrfy_ctx, vrfy_cb);
						
						if (X509_verify_cert(vrfy_ctx) >= 0)
						{
							err = X509_STORE_CTX_get_error(vrfy_ctx);
							// This is for people who don't restart their clients and have low expiration time on their cert
							//result = (err == X509_V_OK) || (err == X509_V_ERR_CERT_HAS_EXPIRED);
							result = (err == X509_V_OK) || (err == X509_V_ERR_CERT_HAS_EXPIRED) || (allowUntrusted && err != X509_V_ERR_APPLICATION_VERIFICATION);
						}
					}
					
					if (cert) X509_free(cert);
					if (vrfy_ctx) X509_STORE_CTX_free(vrfy_ctx);
				}
				
				// KeyPrint is a strong indicator of trust (TODO: check that this KeyPrint is mediated by a trusted hub)
				SSL_set_verify_result(ssl, err);
				
				return result;
			}

void SSLSocket::shutdown() noexcept
{
	if (ssl)
		SSL_shutdown(ssl);
}

void SSLSocket::close() noexcept
{
	if (ssl)
	{
		ssl.reset();
	}
	Socket::shutdown();
	Socket::close();
}

