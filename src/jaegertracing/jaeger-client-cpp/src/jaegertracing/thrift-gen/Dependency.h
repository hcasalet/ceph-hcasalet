/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
#ifndef Dependency_H
#define Dependency_H

#include <thrift/TDispatchProcessor.h>
#include <thrift/async/TConcurrentClientSyncInfo.h>
#include "dependency_types.h"

namespace jaegertracing { namespace thrift {

#ifdef _MSC_VER
  #pragma warning( push )
  #pragma warning (disable : 4250 ) //inheriting methods via dominance 
#endif

class DependencyIf {
 public:
  virtual ~DependencyIf() {}
  virtual void getDependenciesForTrace(Dependencies& _return, const std::string& traceId) = 0;
  virtual void saveDependencies(const Dependencies& dependencies) = 0;
};

class DependencyIfFactory {
 public:
  typedef DependencyIf Handler;

  virtual ~DependencyIfFactory() {}

  virtual DependencyIf* getHandler(const ::apache::thrift::TConnectionInfo& connInfo) = 0;
  virtual void releaseHandler(DependencyIf* /* handler */) = 0;
};

class DependencyIfSingletonFactory : virtual public DependencyIfFactory {
 public:
  DependencyIfSingletonFactory(const ::std::shared_ptr<DependencyIf>& iface) : iface_(iface) {}
  virtual ~DependencyIfSingletonFactory() {}

  virtual DependencyIf* getHandler(const ::apache::thrift::TConnectionInfo&) {
    return iface_.get();
  }
  virtual void releaseHandler(DependencyIf* /* handler */) {}

 protected:
  ::std::shared_ptr<DependencyIf> iface_;
};

class DependencyNull : virtual public DependencyIf {
 public:
  virtual ~DependencyNull() {}
  void getDependenciesForTrace(Dependencies& /* _return */, const std::string& /* traceId */) {
    return;
  }
  void saveDependencies(const Dependencies& /* dependencies */) {
    return;
  }
};


class Dependency_getDependenciesForTrace_args {
 public:

  Dependency_getDependenciesForTrace_args(const Dependency_getDependenciesForTrace_args&);
  Dependency_getDependenciesForTrace_args& operator=(const Dependency_getDependenciesForTrace_args&);
  Dependency_getDependenciesForTrace_args() : traceId() {
  }

  virtual ~Dependency_getDependenciesForTrace_args() throw();
  std::string traceId;

  void __set_traceId(const std::string& val);

  bool operator == (const Dependency_getDependenciesForTrace_args & rhs) const
  {
    if (!(traceId == rhs.traceId))
      return false;
    return true;
  }
  bool operator != (const Dependency_getDependenciesForTrace_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Dependency_getDependenciesForTrace_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Dependency_getDependenciesForTrace_pargs {
 public:


  virtual ~Dependency_getDependenciesForTrace_pargs() throw();
  const std::string* traceId;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Dependency_getDependenciesForTrace_result__isset {
  _Dependency_getDependenciesForTrace_result__isset() : success(false) {}
  bool success :1;
} _Dependency_getDependenciesForTrace_result__isset;

class Dependency_getDependenciesForTrace_result {
 public:

  Dependency_getDependenciesForTrace_result(const Dependency_getDependenciesForTrace_result&);
  Dependency_getDependenciesForTrace_result& operator=(const Dependency_getDependenciesForTrace_result&);
  Dependency_getDependenciesForTrace_result() {
  }

  virtual ~Dependency_getDependenciesForTrace_result() throw();
  Dependencies success;

  _Dependency_getDependenciesForTrace_result__isset __isset;

  void __set_success(const Dependencies& val);

  bool operator == (const Dependency_getDependenciesForTrace_result & rhs) const
  {
    if (!(success == rhs.success))
      return false;
    return true;
  }
  bool operator != (const Dependency_getDependenciesForTrace_result &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Dependency_getDependenciesForTrace_result & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

typedef struct _Dependency_getDependenciesForTrace_presult__isset {
  _Dependency_getDependenciesForTrace_presult__isset() : success(false) {}
  bool success :1;
} _Dependency_getDependenciesForTrace_presult__isset;

class Dependency_getDependenciesForTrace_presult {
 public:


  virtual ~Dependency_getDependenciesForTrace_presult() throw();
  Dependencies* success;

  _Dependency_getDependenciesForTrace_presult__isset __isset;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);

};

typedef struct _Dependency_saveDependencies_args__isset {
  _Dependency_saveDependencies_args__isset() : dependencies(false) {}
  bool dependencies :1;
} _Dependency_saveDependencies_args__isset;

class Dependency_saveDependencies_args {
 public:

  Dependency_saveDependencies_args(const Dependency_saveDependencies_args&);
  Dependency_saveDependencies_args& operator=(const Dependency_saveDependencies_args&);
  Dependency_saveDependencies_args() {
  }

  virtual ~Dependency_saveDependencies_args() throw();
  Dependencies dependencies;

  _Dependency_saveDependencies_args__isset __isset;

  void __set_dependencies(const Dependencies& val);

  bool operator == (const Dependency_saveDependencies_args & rhs) const
  {
    if (!(dependencies == rhs.dependencies))
      return false;
    return true;
  }
  bool operator != (const Dependency_saveDependencies_args &rhs) const {
    return !(*this == rhs);
  }

  bool operator < (const Dependency_saveDependencies_args & ) const;

  uint32_t read(::apache::thrift::protocol::TProtocol* iprot);
  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};


class Dependency_saveDependencies_pargs {
 public:


  virtual ~Dependency_saveDependencies_pargs() throw();
  const Dependencies* dependencies;

  uint32_t write(::apache::thrift::protocol::TProtocol* oprot) const;

};

class DependencyClient : virtual public DependencyIf {
 public:
  DependencyClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  DependencyClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void getDependenciesForTrace(Dependencies& _return, const std::string& traceId);
  void send_getDependenciesForTrace(const std::string& traceId);
  void recv_getDependenciesForTrace(Dependencies& _return);
  void saveDependencies(const Dependencies& dependencies);
  void send_saveDependencies(const Dependencies& dependencies);
 protected:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
};

class DependencyProcessor : public ::apache::thrift::TDispatchProcessor {
 protected:
  ::std::shared_ptr<DependencyIf> iface_;
  virtual bool dispatchCall(::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, const std::string& fname, int32_t seqid, void* callContext);
 private:
  typedef  void (DependencyProcessor::*ProcessFunction)(int32_t, ::apache::thrift::protocol::TProtocol*, ::apache::thrift::protocol::TProtocol*, void*);
  typedef std::map<std::string, ProcessFunction> ProcessMap;
  ProcessMap processMap_;
  void process_getDependenciesForTrace(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
  void process_saveDependencies(int32_t seqid, ::apache::thrift::protocol::TProtocol* iprot, ::apache::thrift::protocol::TProtocol* oprot, void* callContext);
 public:
  DependencyProcessor(::std::shared_ptr<DependencyIf> iface) :
    iface_(iface) {
    processMap_["getDependenciesForTrace"] = &DependencyProcessor::process_getDependenciesForTrace;
    processMap_["saveDependencies"] = &DependencyProcessor::process_saveDependencies;
  }

  virtual ~DependencyProcessor() {}
};

class DependencyProcessorFactory : public ::apache::thrift::TProcessorFactory {
 public:
  DependencyProcessorFactory(const ::std::shared_ptr< DependencyIfFactory >& handlerFactory) :
      handlerFactory_(handlerFactory) {}

  ::std::shared_ptr< ::apache::thrift::TProcessor > getProcessor(const ::apache::thrift::TConnectionInfo& connInfo);

 protected:
  ::std::shared_ptr< DependencyIfFactory > handlerFactory_;
};

class DependencyMultiface : virtual public DependencyIf {
 public:
  DependencyMultiface(std::vector<std::shared_ptr<DependencyIf> >& ifaces) : ifaces_(ifaces) {
  }
  virtual ~DependencyMultiface() {}
 protected:
  std::vector<std::shared_ptr<DependencyIf> > ifaces_;
  DependencyMultiface() {}
  void add(::std::shared_ptr<DependencyIf> iface) {
    ifaces_.push_back(iface);
  }
 public:
  void getDependenciesForTrace(Dependencies& _return, const std::string& traceId) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->getDependenciesForTrace(_return, traceId);
    }
    ifaces_[i]->getDependenciesForTrace(_return, traceId);
    return;
  }

  void saveDependencies(const Dependencies& dependencies) {
    size_t sz = ifaces_.size();
    size_t i = 0;
    for (; i < (sz - 1); ++i) {
      ifaces_[i]->saveDependencies(dependencies);
    }
    ifaces_[i]->saveDependencies(dependencies);
  }

};

// The 'concurrent' client is a thread safe client that correctly handles
// out of order responses.  It is slower than the regular client, so should
// only be used when you need to share a connection among multiple threads
class DependencyConcurrentClient : virtual public DependencyIf {
 public:
  DependencyConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
    setProtocol(prot);
  }
  DependencyConcurrentClient(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    setProtocol(iprot,oprot);
  }
 private:
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> prot) {
  setProtocol(prot,prot);
  }
  void setProtocol(std::shared_ptr< ::apache::thrift::protocol::TProtocol> iprot, std::shared_ptr< ::apache::thrift::protocol::TProtocol> oprot) {
    piprot_=iprot;
    poprot_=oprot;
    iprot_ = iprot.get();
    oprot_ = oprot.get();
  }
 public:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getInputProtocol() {
    return piprot_;
  }
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> getOutputProtocol() {
    return poprot_;
  }
  void getDependenciesForTrace(Dependencies& _return, const std::string& traceId);
  int32_t send_getDependenciesForTrace(const std::string& traceId);
  void recv_getDependenciesForTrace(Dependencies& _return, const int32_t seqid);
  void saveDependencies(const Dependencies& dependencies);
  void send_saveDependencies(const Dependencies& dependencies);
 protected:
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> piprot_;
  std::shared_ptr< ::apache::thrift::protocol::TProtocol> poprot_;
  ::apache::thrift::protocol::TProtocol* iprot_;
  ::apache::thrift::protocol::TProtocol* oprot_;
  ::apache::thrift::async::TConcurrentClientSyncInfo sync_;
};

#ifdef _MSC_VER
  #pragma warning( pop )
#endif

}} // namespace

#endif
