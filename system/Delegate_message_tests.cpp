#include "Grappa.hpp"
#include "Message.hpp"
#include "FullEmpty.hpp"
  
namespace Grappa {

  template <typename A, typename R>
  R delegate_call( Core target, std::function<R (A)> Func, A arg ) {
    Core origin = Grappa_mynode();
    if ( target == origin ) {
      return Func(arg);
    } else {
      FullEmpty<R> result;
      auto resultAddr = &result;

      send_message( target, [=]() {
          // on target
          R remoteResult = Func(arg);

          send_message( origin, [=]() {
            // on origin
            resultAddr->writeEF(remoteResult);
          });
      });  

      return result.readFE();
    }
  }

  // example
  double delegate_atomic_swap( GlobalAddress<double> remoteVal, double localVar ) {

    return delegate_call<double, double>( remoteVal.node(), 
                          [=](double arg) {
                            // on remote
                            auto temp = *(remoteVal.pointer());
                            *(remoteVal.pointer()) = arg;
                            return temp;
                          },
                         localVar ); 
  } 

}





/* Ultimately want some blocking/nonblocking RPC type interface to build off of */

  

