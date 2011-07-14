#ifndef __tascel_UniformTaskCollectionShared_h__
#define __tascel_UniformTaskCollectionShared_h__

#include "Comm.h"
#include "FuncReg.h"
#include "SharedQueue.h"
#include "UniformTaskCollection.h"

namespace tascel {

  /**
   * Implementation of the UniformTaskCollection using a SharedQueue.
   *
   * This is a thin wrapper around the SharedQueue.
   */
  class UniformTaskCollectionShared : public UniformTaskCollection {
    private:
      SharedQueue sq; /**< the SharedQueue instance */

    public:
      /**
       * Constructs the UniformTaskCollectionShared.
       *
       * @copydetails UniformTaskCollection::UniformTaskCollection(TslFunc,const TslFuncRegTbl&,int,int,void*,int)
       */
      UniformTaskCollectionShared(TslFunc _fn, const TslFuncRegTbl &frt,
                                  int tsk_size, int ntsks,
                                  void *pldata = 0, int pldata_len = 0);

      /**
       * Destroys the UniformTaskCollectionShared.
       */
      virtual ~UniformTaskCollectionShared();

      /**
       * @copybrief   UniformTaskCollection::process()
       * @copydetails UniformTaskCollection::process()
       */
      virtual void process();

      /**
       * @copybrief   UniformTaskCollection::addTask()
       * @copydetails UniformTaskCollection::addTask()
       */
      virtual void addTask(void *data, int dlen);

  }; /*UniformTaskCollectionShared*/

}; /*tascel*/

#endif /*__tascel_UniformTaskCollectionShared_h__*/

