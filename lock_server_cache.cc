#include "lock_server_cache.h"
#include <sstream>
#include <stdio.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "lang/verify.h"
#include "handle.h"
#include "tprintf.h"
#include "jsl_log.h"
#include "rpc.h"

lock_server_cache::lock_server_cache() { }

int lock_server_cache::acquire(lock_protocol::lockid_t lid, std::string id, 
	int &) {
	jsl_log(JSL_DBG_ME, "%llu acquire from %s\n", lid, id.c_str());
	bool revok = false;
	std::string client_to_be_revoked = "";
	{
		ScopedLock ml(&mutex);
		lock_info &li = locks[lid];
		if (!li.is_locked) {
    // easy case, lock is not held
			li.is_locked = true;
			li.id = id;
			jsl_log(JSL_DBG_ME, "%llu lock granted immediately to %s\n", lid, id.c_str());
			return lock_protocol :: OK;
		}
    // a client shouldn't ask twice for the lock
		VERIFY(li.waiting.find(id) == li.waiting.end());

		// If this is the first time a client ask for this lock when it's held		
		// We revoke it
		revok = li.waiting.size() == 0;
		li.waiting.insert(id);
		client_to_be_revoked = li.id;
		// REPLY could be sent right away, without waiting for revoke to
		// complete. To investigate further... It's correct either way.
	}
	// possibly, the revoke isn't needed anymore because the client released
	// the lock without being asked for.
	// an extra-revoke doesn't impact correctness, so this is not a problem.
	// TODO: we don't consider the case where a client terminates 
	if (revok) {
		jsl_log(JSL_DBG_ME, "%llu couldn't grant lock to %s, revoke current owner %s\n",
			lid, id.c_str(), client_to_be_revoked.c_str());
		revoke(client_to_be_revoked, lid);
	} else {
		jsl_log(JSL_DBG_ME, "%llu revoked already sent to current owner %s\n", lid,
			client_to_be_revoked.c_str());
	}
	return lock_protocol::RETRY;
}

void lock_server_cache :: revoke(const std::string &client_to_be_revoked, 
																 const lock_protocol::lockid_t &lid) { 
	jsl_log(JSL_DBG_ME, "%llu client.revoke() %s\n", lid, client_to_be_revoked.c_str());
	sockaddr_in dstsock;
	make_sockaddr(client_to_be_revoked.c_str(), &dstsock);
	rpcc cl(dstsock);
	VERIFY (cl.bind() >= 0);
	int r;
	VERIFY(cl.call(rlock_protocol::revoke, lid, r) == rlock_protocol::OK);
	jsl_log(JSL_DBG_ME, "%llu client.revoke() - OK %s\n", lid, client_to_be_revoked.c_str());
	return;
}

void lock_server_cache :: retry(const std::string &client_to_be_retried, 
																const lock_protocol::lockid_t &lid) {
	jsl_log(JSL_DBG_ME, "%llu client.retry() %s\n", lid, client_to_be_retried.c_str());
	sockaddr_in dstsock;
	make_sockaddr(client_to_be_retried.c_str(), &dstsock);
	rpcc cl(dstsock);
	VERIFY (cl.bind() >= 0);
	int r;
	VERIFY(cl.call(rlock_protocol::retry, lid, r) == rlock_protocol::OK);
	jsl_log(JSL_DBG_ME, "%llu client.retry() - OK %s\n", lid, client_to_be_retried.c_str());
}

int lock_server_cache::release(lock_protocol::lockid_t lid, std::string id, int &r) {
	bool need_retry = false; 
	bool need_revoke = false;
	std::string new_owner;
	{
		ScopedLock ml(&mutex);
		jsl_log(JSL_DBG_ME, "%llu release from %s\n", lid, id.c_str());

		lock_info &li = locks[lid];

		VERIFY(li.is_locked);

		need_retry = li.waiting.size() != 0;
		need_revoke = li.waiting.size() >= 2;

		if (!need_retry) {
			li.is_locked = false;
			return lock_protocol::OK;
		}

		auto some = li.waiting.begin();
		new_owner = *some;
		li.id = new_owner;
		li.waiting.erase(some);
	}

	retry(new_owner, lid);
	if (need_revoke) {
		revoke(new_owner, lid);
	}
	return lock_protocol::OK;
}

lock_protocol::status lock_server_cache::stat(lock_protocol::lockid_t lid, 
																							int &r) {
	r = nacquire;
	return lock_protocol::OK;
}
