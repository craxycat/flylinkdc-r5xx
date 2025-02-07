/*
 * Copyright (C) 2009-2011 Big Muscle, http://strongdc.sf.net
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

#include "stdafx.h"

#ifdef STRONG_USE_DHT

#include "SearchManager.h"

#include "Constants.h"
#include "DHT.h"
#include "IndexManager.h"
#include "Utils.h"

#include "../client/ClientManager.h"
#include "../client/SearchManager.h"
#include "../client/SearchResult.h"
#include "../client/SimpleXML.h"
#include "../FlyFeatures/flyServer.h"
namespace dht
{

Search::~Search()
{
	switch (m_type)
	{
		case TYPE_NODE:
			IndexManager::setPublish(true);
			break;
		case TYPE_STOREFILE:
			IndexManager::decPublishing();
			break;
	}
}

/*
 * Process this search request
 */
void Search::process()
{
	if (stopping)
		return;
		
	// no node to search
	if (possibleNodes.empty()/* || respondedNodes.size() >= MAX_SEARCH_RESULTS*/)
	{
		stopping = true;
		m_lifeTime = GET_TICK() + SEARCH_STOPTIME; // wait before deleting not to lose so much delayed results
		return;
	}
	
	// send search request to the first ALPHA closest nodes
	const size_t l_nodesCount = min((size_t)SEARCH_ALPHA, possibleNodes.size());
	for (size_t i = 0; i < l_nodesCount; i++)
	{
		auto it = possibleNodes.begin();
		const Node::Ptr node = it->second;
		
		// move to tried and delete from possibles
		triedNodes[node->getUser()->getCID()] = node;
		possibleNodes.erase(it);
		
		// send SCH command
		AdcCommand cmd(AdcCommand::CMD_SCH, AdcCommand::TYPE_UDP);
		cmd.addParam("TR", m_term);
		cmd.addParam("TY", Util::toString(m_type));
		cmd.addParam("TO", Util::toString(m_token));
		
		//node->setTimeout();
		DHT::getInstance()->send(cmd, node->getIdentity().getIpAsString(), node->getIdentity().getUdpPort(), node->getUser()->getCID(), node->getUdpKey());
	}
}

SearchManager::SearchManager() : m_lastTimeSearchFile(0)
{
}

SearchManager::~SearchManager()
{
	for (auto i = m_searches.cbegin(); i != m_searches.cend(); ++i)
	{
		delete i->second;
	}
}

/*
 * Performs node lookup in the network
 */
void SearchManager::findNode(const CID& cid)
{
	dcassert(BOOLSETTING(USE_DHT));
	if (isAlreadySearchingFor(cid.toBase32()))
		return;
		
	Search* s = new Search(Search::TYPE_NODE, cid.toBase32(), Util::rand());
	
	search(*s);
}

/*
 * Performs value lookup in the network
 */
void SearchManager::findFile(const string& tth, uint32_t p_token)
{
	dcassert(BOOLSETTING(USE_DHT));
	// temporary fix to prevent UDP flood (search queue would be better here)
	if (GET_TICK() - m_lastTimeSearchFile < 10000)
		return;
		
	if (isAlreadySearchingFor(tth))
		return;
		
	// do I have requested TTH in my store?
	//IndexManager::SourceList sources;
	//if(IndexManager::getInstance()->findResult(TTHValue(tth), sources))
	//{
	//  for(auto i = sources.cbegin(); i != sources.cend(); ++i)
	//  {
	//      // create user as offline (only TCP connected users will be online)
	//      UserPtr u = ClientManager::getUser(i->getCID());
	//      u->setFlag(User::DHT);
	//
	//      // contact node that we are online and we want his info
	//      DHT::getInstance()->info(i->getIp(), i->getUdpPort(), true);
	//
	//      const SearchResult sr(u, SearchResult::TYPE_FILE, 0, 0, i->getSize(), tth, "DHT", Util::emptyString, i->getIp(), TTHValue(tth), token);
	//      dcpp::SearchManager::getInstance()->fly_fire1(SearchManagerListener::SR(), sr);
	//  }
	//
	//  return;
	//}
	
	Search* s = new Search(Search::TYPE_FILE, tth, p_token);
	
	search(*s);
	
	m_lastTimeSearchFile = GET_TICK();
}

/*
 * Performs node lookup to store key/value pair in the network
 */
void SearchManager::findStore(const string& tth, int64_t size, bool partial)
{
	dcassert(BOOLSETTING(USE_DHT));
	if (isAlreadySearchingFor(tth))
	{
		IndexManager::decPublishing();
		return;
	}
	
	Search* s = new Search(Search::TYPE_STOREFILE, tth, Util::rand());
	s->m_filesize = size;
	s->partial    = partial;
	
	search(*s);
}

/*
 * Performs general search operation in the network
 */
void SearchManager::search(Search& s)
{
	dcassert(BOOLSETTING(USE_DHT));
	// set search lifetime
	s.m_lifeTime = GET_TICK();
	switch (s.m_type)
	{
		case Search::TYPE_FILE:
			s.m_lifeTime += SEARCHFILE_LIFETIME;
			break;
		case Search::TYPE_NODE:
			s.m_lifeTime += SEARCHNODE_LIFETIME;
			break;
		case Search::TYPE_STOREFILE:
			s.m_lifeTime += SEARCHSTOREFILE_LIFETIME;
			break;
	}
	
	// get nodes closest to requested ID
	DHT::getInstance()->getClosestNodes(CID(s.m_term), s.possibleNodes, 50, 3);
	
	if (s.possibleNodes.empty())
	{
		delete &s;
		return;
	}
	
	CFlyFastLock(cs);
	// store search
	m_searches[s.m_token] = &s;
	
	s.process();
}

/*
 * Process incoming search request
 */
void SearchManager::processSearchRequest(const string& ip, uint16_t port, const UDPKey& udpKey, const AdcCommand& cmd)
{
	dcassert(BOOLSETTING(USE_DHT));
	string token;
	if (!cmd.getParam("TO", 1, token))
		return; // missing search token?
		
	string term;
	if (!cmd.getParam("TR", 1, term))
		return; // nothing to search?
		
	string type;
	if (!cmd.getParam("TY", 1, type))
		return; // type not specified?
		
	AdcCommand res(AdcCommand::CMD_RES, AdcCommand::TYPE_UDP);
	res.addParam("TO", token);
	
	SimpleXML xml;
	xml.addTag("Nodes");
	xml.stepIn();
	
	bool empty = true;
	unsigned int searchType = Util::toInt(type);
	switch (searchType)
	{
		case Search::TYPE_FILE:
		{
			// check file hash in our database
			// if it's there, then select sources else do the same as node search
			dht::SourceList sources;
			if (IndexManager::getInstance()->findResult(TTHValue(term), sources))
			{
				// yes, we got sources for this file
				unsigned int n = MAX_SEARCH_RESULTS;
				for (auto i = sources.cbegin(); i != sources.cend() && n > 0; ++i, n--)
				{
					xml.addTag("Source");
					xml.addChildAttrib("CID", i->getCID().toBase32());
					xml.addChildAttrib("I4", i->getIp());
					xml.addChildAttrib("U4", i->getUdpPort());
					xml.addChildAttrib("SI", i->getSize());
					xml.addChildAttrib("PF", i->getPartial());
					
					empty = false;
				}
				break;
			}
		}
		default:
		{
			// maximum nodes in response is based on search type
			unsigned int count;
			switch (searchType)
			{
				case Search::TYPE_FILE:
					count = 2;
					break;
				case Search::TYPE_NODE:
					count = 10;
					break;
				case Search::TYPE_STOREFILE:
					count = 4;
					break;
				default:
					return; // unknown type
			}
			
			// get nodes closest to requested ID
			Node::Map nodes;
			if (term.size() != 39)
			{
				LogManager::dht_message("DHT - SearchManager::processSearchRequest Error term.size() != 39. term = " + term);
			}
			else
			{
				const CID l_CID_term(term);
				DHT::getInstance()->getClosestNodes(l_CID_term, nodes, count, 2);
				
				// add nodelist in XML format
				for (auto i = nodes.cbegin(); i != nodes.cend(); ++i)
				{
					xml.addTag("Node");
					xml.addChildAttrib("CID", i->second->getUser()->getCID().toBase32());
					xml.addChildAttrib("I4", i->second->getIdentity().getIpAsString());
					xml.addChildAttrib("U4", i->second->getIdentity().getUdpPort());
					
					empty = false;
				}
			}
			break;
		}
	}
	
	xml.stepOut();
	
	if (empty)
		return; // no requested nodes found, don't send empty list
		
	string nodes;
	StringOutputStream sos(nodes);
	//sos.write(SimpleXML::utf8Header); // don't write header to save some bytes
	xml.toXML(&sos);
	
	res.addParam("NX", Utils::compressXML(nodes));
	
	// send search result
	DHT::getInstance()->send(res, ip, port, CID(cmd.getParam(0)), udpKey);
}

/*
 * Process incoming search result
 */
void SearchManager::processSearchResult(const AdcCommand& cmd)
{
	dcassert(BOOLSETTING(USE_DHT));
	string l_token_str;
	if (!cmd.getParam("TO", 1, l_token_str))
		return; // missing search token?
	uint32_t l_token = Util::toUInt32(l_token_str);
	dcassert(l_token);
	
	string nodes;
	if (!cmd.getParam("NX", 1, nodes))
		return; // missing search token?
		
	CFlyFastLock(cs);
	auto i = m_searches.find(l_token);
	if (i == m_searches.end())
	{
		// we didn't search for this
		return;
	}
	
	Search* s = i->second;
	
	// store this node
	const CID l_cid = CID(cmd.getParam(0));
	auto t = s->triedNodes.find(l_cid);
	if (t == s->triedNodes.end())
		return; // we did not contact this node so why response from him???
		
	s->respondedNodes.insert(std::make_pair(Utils::getDistance(CID(cmd.getParam(0)), CID(s->m_term)), t->second));
	
	try
	{
		SimpleXML xml;
		xml.fromXML(nodes);
		xml.stepIn();
		
		if (s->m_type == Search::TYPE_FILE) // this is response to TYPE_FILE, check sources first
		{
			// extract file sources
			while (xml.findChild("Source"))
			{
				const CID cid       = CID(xml.getChildAttrib("CID"));
				const string i4     = xml.getChildAttrib("I4");
				uint16_t u4         = static_cast<uint16_t>(xml.getIntChildAttrib("U4"));
				int64_t size        = xml.getLongLongChildAttrib("SI");
				bool partial        = xml.getBoolChildAttrib("PF");
				
				// don't bother with invalid sources and private IPs
				if (cid.isZero() || ClientManager::getMyCID() == cid || !Utils::isGoodIPPort(i4, u4)) // [!] IRainman fix.
					continue;
					
				// create user as offline (only TCP connected users will be online)
				Node::Ptr source = DHT::getInstance()->addDHTNode(cid, i4, u4, UDPKey(), false, false);
				if (!source)
					return;
				if (partial)
				{
					if (!source->isOnline())
					{
						// node is not online, try to contact him
						DHT::getInstance()->info(i4, u4, DHT::PING | DHT::CONNECTION, cid, source->getUdpKey());
					}
					
					// ask for partial file
					AdcCommand l_cmd(AdcCommand::CMD_PSR, AdcCommand::TYPE_UDP);
					l_cmd.addParam("U4", ::SearchManager::getSearchPort());
					l_cmd.addParam("TR", s->m_term);
					
					DHT::getInstance()->send(l_cmd, i4, u4, cid, source->getUdpKey());
				}
				else
				{
					// create search result: hub name+ip => "DHT", file name => TTH
					boost::system::error_code l_ec;
					const auto l_ip4 = boost::asio::ip::address_v4::from_string(i4, l_ec);
					dcassert(!l_ec);
					if (!l_ec)
					{
						const SearchResult sr(source->getUser(), SearchResult::TYPE_FILE, !source->isOnline() ? 0 : source->getIdentity().getSlots(), 0, size, s->m_term, DHT::getInstance()->getHubName(), DHT::getInstance()->getHubUrl(), l_ip4, TTHValue(s->m_term), l_token);
						if (!source->isOnline())
						{
							// node is not online, try to contact him if we didn't contact him recently
							if (m_searchResults.find(source->getUser()->getCID()) != m_searchResults.end())
								DHT::getInstance()->info(i4, u4, DHT::PING | DHT::CONNECTION, cid, source->getUdpKey());
								
							m_searchResults.insert(std::make_pair(source->getUser()->getCID(), std::make_pair(GET_TICK(), sr)));
						}
						else
						{
							::SearchManager::getInstance()->fly_fire1(::SearchManagerListener::SR(), sr);
						}
					}
					else
					{
#ifdef FLYLINKDC_BETA
						const string l_message = "SearchManager::processSearchResult Error IP = " + i4;
						LogManager::message(l_message);
						CFlyServerJSON::pushError(27, l_message);
#endif
					}
				}
			}
			
			xml.resetCurrentChild();
		}
		
		// extract possible nodes
		unsigned int n = DHT_K;
		while (!ClientManager::isShutdown() && xml.findChild("Node") && n-- > 0)
		{
			const CID cid = CID(xml.getChildAttrib("CID"));
			const CID distance = Utils::getDistance(cid, CID(s->m_term));
			
			// don't bother with myself and nodes we've already tried or queued
			if (ClientManager::getMyCID() == cid || // [!] IRainman fix.
			        s->possibleNodes.find(distance) != s->possibleNodes.end() ||
			        s->triedNodes.find(cid) != s->triedNodes.end())
			{
				continue;
			}
			
			const string i4 = xml.getChildAttrib("I4");
			uint16_t u4 = static_cast<uint16_t>(xml.getIntChildAttrib("U4"));
			
			// don't bother with private IPs
			if (!Utils::isGoodIPPort(i4, u4))
				continue;
				
			// create unverified node
			// if this node already exists in our routing table, don't update its ip/port for security reasons
			// node won't be accepted for several reasons (invalid IP etc.)
			// if routing table is full, node can be accepted
			bool isAcceptable = true;// TODO
			Node::Ptr node = DHT::getInstance()->addDHTNode(cid, i4, u4, UDPKey(), false, false);
			if (isAcceptable && node)
			{
				// update our list of possible nodes
				s->possibleNodes[distance] = node;
			}
		}
		xml.stepOut();
	}
	catch (const SimpleXMLException&)
	{
		// malformed node list
	}
}

/*
 * Sends publishing request
 */
void SearchManager::publishFile(const Node::Map& nodes, const string& tth, int64_t size, bool partial)
{
	dcassert(BOOLSETTING(USE_DHT));
	// send PUB command to K nodes
	int n = DHT_K;
	for (auto i = nodes.cbegin(); i != nodes.cend() && n > 0; ++i, n--)
	{
		const Node::Ptr& node = i->second;
		
		AdcCommand cmd(AdcCommand::CMD_PUB, AdcCommand::TYPE_UDP);
		cmd.addParam("TR", tth);
		cmd.addParam("SI", Util::toString(size));
		
		if (partial)
			cmd.addParam("PF", "1");
			
		//i->second->setTimeout();
		DHT::getInstance()->send(cmd, node->getIdentity().getIpAsString(), node->getIdentity().getUdpPort(), node->getUser()->getCID(), node->getUdpKey());
	}
}

/*
 * Processes all running searches and removes long-time ones
 */
void SearchManager::processSearches()
{
	dcassert(BOOLSETTING(USE_DHT));
	CFlyFastLock(cs);
	
	auto it = m_searches.begin();
	while (it != m_searches.end())
	{
		Search* s = it->second;
		
		// process active search
		s->process();
		
		// remove long search
		if (s->m_lifeTime < GET_TICK())
		{
			// search timed out, stop it
			m_searches.erase(it++);
			
			if (s->m_type == Search::TYPE_STOREFILE)
			{
				publishFile(s->respondedNodes, s->m_term, s->m_filesize, s->partial);
			}
			
			delete s;
		}
		else
		{
			++it;
		}
	}
}

/*
 * Processes incoming search results
 */
bool SearchManager::processSearchResults(const UserPtr& user, size_t slots)
{
	dcassert(BOOLSETTING(USE_DHT));
	bool ok = false;
	uint64_t tick = GET_TICK();
	
	ResultsMap::iterator it = m_searchResults.begin();
	while (it != m_searchResults.end())
	{
		if (it->first == user->getCID())
		{
			// user is online, process his result
			SearchResult sr = it->second.second;
			sr.setSlots(uint8_t(slots)); // slot count should be known now
			
			::SearchManager::getInstance()->fly_fire1(::SearchManagerListener::SR(), sr);
			m_searchResults.erase(it++);
			
			ok = true;
		}
		else if (it->second.first + 60 * 1000 <= tick)
		{
			// delete result from possibly offline users
			m_searchResults.erase(it++);
		}
		else
		{
			++it;
		}
	}
	
	return ok;
}

/*
 * Checks whether we are alreading searching for a term
 */
bool SearchManager::isAlreadySearchingFor(const string& p_term)
{
	dcassert(BOOLSETTING(USE_DHT));
	CFlyFastLock(cs);
	for (auto i = m_searches.cbegin(); i != m_searches.cend(); ++i)
	{
		if (i->second->m_term == p_term)
		{
			return true;
		}
	}
	
	return false;
}

}

#endif // STRONG_USE_DHT