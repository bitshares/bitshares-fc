#include <boost/fiber/future/future.hpp>

#include <fc/thread/fibers.hpp>

namespace fc { namespace test {

void init_rr_scheduler();

class worker_thread
{
   boost::thread _thread;
   boost::fibers::condition_variable _cv;
   boost::fibers::mutex _mtx;
   bool _shutdown = false;

   public:
      worker_thread()
      {
         _thread = boost::thread( [this] () {
            init_rr_scheduler();
            std::unique_lock<boost::fibers::mutex> lock(_mtx);
            _cv.wait( lock, [this] () { return _shutdown; } );
         });
      }
      ~worker_thread()
      {
         std::unique_lock<boost::fibers::mutex> lock(_mtx);
         _shutdown = true;
         _cv.notify_all();
         lock.unlock();
         _thread.join();
      }
      boost::thread::id id() { return _thread.get_id(); }
};

struct worker_thread_config {
  worker_thread_config() {
     init_rr_scheduler();
  }
};

}} // fc::test