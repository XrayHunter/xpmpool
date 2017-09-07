/*
 * pool.h
 *
 *  Created on: 18.04.2014
 *      Author: mad
 */

#ifndef POOL_H_
#define POOL_H_



#include "primeserver.h"
#include "wallet.h"
#include "prime.h"

#undef loop

#include <map>
#include <list>
#include <set>
#include <string>


#include <zmq.h>
#include <czmq.h>

#include "protocol.pb.h"

#include <utf8.h>

#include <dbobjects.h>

#include <Wt/Dbo/backend/Postgres>
#include <Wt/Dbo/Session>

using namespace pool;



inline bool isValidUTF8(const std::string& str) {
	
	return utf8::is_valid(str.begin(), str.end());
	
}

inline std::string repInvUTF8(const std::string& str) {
	
	std::string res;
	utf8::replace_invalid(str.begin(), str.end(), std::back_inserter(res));
	return res;
	
}




class PrimeWorker {
public:
	
	PrimeWorker(CWallet* pwallet, unsigned threadid, unsigned target);
	
	static void InvokeWork(zsock_t *pipe, void *args);
	
	static int InvokeInput(zloop_t *wloop, zmq_pollitem_t *item, void *arg);
	//static int InvokeBackend(zloop_t *wloop, zmq_pollitem_t *item, void *arg);
	static int InvokeRequest(zloop_t *wloop, zmq_pollitem_t *item, void *arg);
	static int InvokeTimerFunc(zloop_t *loop, int timer_id, void *arg);
	static int zactor_term(zloop_t *wloop, zsock_t *pipe, void *arg);
	
	zmsg_t* ReceiveRequest(proto::Request& req, void* socket);
	static void SendReply(const proto::Reply& rep, zmsg_t** msg, void* socket);
	static void SendData(const proto::Data& data, void* socket);
	
protected:
	
	void Work(zsock_t *pipe);
	
	int HandleInput(zmq_pollitem_t *item);
	int HandleBackend(zmq_pollitem_t *item);
	int HandleRequest(zmq_pollitem_t *item);
	
	int FlushStats();
	
	static int CheckVersion(unsigned version);
	static int CheckReqNonce(const uint256& nonce);
	
	
private:
	
	CWallet* mWallet;
	
	std::string mHost;
	std::string mName;
	unsigned mThreadID;
	
	zsock_t* mSignals;
	zsock_t* mServer;
	zsock_t* mBackend;
	
	int mServerPort;
	int mSignalPort;
	
	unsigned mCurrHeight;
	unsigned mExtraNonce;
	std::map<uint256, unsigned int> mNonceMap;
	CReserveKey mReserveKey;
	CBlockTemplate* mBlockTemplate;
	CBlockIndex* mIndexPrev;
	unsigned mWorkerCount;
	
	unsigned mReqDiff;
	unsigned mTarget;
	std::set<uint256> mReqNonces;
	std::set<uint256> mShares;
	std::map<std::pair<std::string,uint64>, proto::Data> mStats;
	std::map<std::pair<int,int>, int> mReqStats;
	uint64 mInvCount;
	
	proto::Signal mSignal;
	proto::Request mRequest;
	proto::Reply mReply;
	proto::Data mData;
	proto::ServerInfo mServerInfo;
	proto::Block mCurrBlock;
	proto::ServerStats mServerStats;
	
};



class PoolFrontend {
public:
	
	PoolFrontend(unsigned port);
	~PoolFrontend();
	
	static void InvokeProxy(void *arg, void *pipe);
	void ProxyLoop(void *pipe);
	
private:
	
	unsigned mPort;
	
	zsock_t* mRouter;
	zsock_t* mDealer;
	zactor_t* mPipe;
};













class PoolBackend {
public:
	
	PoolBackend(CWallet* pwallet);
	~PoolBackend();
	
	static void InvokeThread(void *arg, void *pipe);
	void ThreadLoop(void *pipe);
	
	static int InvokeInput(zloop_t *wloop, zmq_pollitem_t *item, void *arg);
	static int InvokeShutdown(zloop_t *wloop, zmq_pollitem_t *item, void *arg);
	static int InvokeAccounting(zloop_t *loop, int timer_id, void *arg);
	
	int HandleData(zmq_pollitem_t *item);
	
	dbo::ptr<db::Address> GetAddress(const std::string& str);
	
	int Accounting();
	
	const db::Settings& Settings() { return *mSettings; }
	
	
private:
	
	CWallet* mWallet;
	
	zactor_t* mPipe;
	zsock_t* mFrontend;
	
	proto::Data mData;
	
	dbo::backend::Postgres* mBackend;
	dbo::Session* mSession;
	
	dbo::ptr<db::Settings> mSettings;
	std::string mPoolAddress;
	
	unsigned mTickCounter;
	std::map<unsigned, int64> mShareValues;
	
	std::map<std::pair<std::string,int>, proto::ServerStats> mLastServerStats;
	
	
};



class PoolServer : public PrimeServer {
public:
	
	PoolServer(CWallet* pwallet);
	virtual ~PoolServer();
	
	virtual void NotifyNewBlock(CBlockIndex* pindex);
	
	static void SendSignal(proto::Signal& signal, zsock_t* socket);
	
	
private:
	
	PoolFrontend* mFrontend;
	PoolBackend* mBackend;
	
	CWallet* mWallet;
	
	std::vector<std::pair<PrimeWorker*, void*> > mWorkers;
	
	zsock_t* mWorkerSignals;
	zsock_t* mBackendDealer;

	int mMinShare;
	int mTarget;
	
	
};


#endif /* POOL_H_ */
