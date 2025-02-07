/*
 * Copyright (C) 2001-2013 Jacek Sieka, arnetheduck on gmail point com
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

#pragma once


#ifndef DCPLUSPLUS_DCPP_FORWARD_H_
#define DCPLUSPLUS_DCPP_FORWARD_H_

/** @file
 * This file contains forward declarations for the various DC++ classes
 */

#include <boost/intrusive_ptr.hpp>

class FavoriteHubEntry;
typedef std::vector<FavoriteHubEntry*> FavoriteHubEntryList;

class FinishedItem;
typedef std::shared_ptr<FinishedItem> FinishedItemPtr;
typedef std::deque<FinishedItemPtr> FinishedItemList;

class HubEntry;
typedef std::deque<HubEntry> HubEntryList; // [!] IRainman opt: change vector to deque

//class Identity;

class OnlineUser;
typedef boost::intrusive_ptr<OnlineUser> OnlineUserPtr;
typedef std::vector<OnlineUserPtr> OnlineUserList;

class QueueItem;
typedef std::shared_ptr<QueueItem> QueueItemPtr;
typedef std::list<QueueItemPtr> QueueItemList; // �� vector - ������ ������
//typedef unsigned QueueItemID;
//typedef std::deque<QueueItemID> QueueItemIDArray;

class UploadQueueItem;
typedef std::shared_ptr<UploadQueueItem> UploadQueueItemPtr;

// http://code.google.com/p/flylinkdc/issues/detail?id=1413
//class UploadQueueItemInfo;
//typedef UploadQueueItemInfo* UploadQueueItemInfoPtr;

class User;
typedef std::shared_ptr<User> UserPtr;
typedef std::vector<UserPtr> UserList;

class UserCommand;
class UserConnection;

#endif /*DCPLUSPLUS_CLIENT_FORWARD_H_*/
