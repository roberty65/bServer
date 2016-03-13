/* EventHandler.h
 *
**/
#ifndef __EVENT_HANDLER__H
#define __EVENT_HANDLER__H

namespace beyondy {
namespace Async {

class EventHandler {
public:
	EventHandler(int _fd) : fd(_fd) {}
	virtual ~EventHandler() {}
public:
	virtual int onReadable(int fd) { return 0; }
	virtual int onWritable(int fd) { return 0; }
	virtual int onError(int fd) { return 0; }
public: // getter???
	int fd;		// file-handle
	int events;	// events currently in mointoring
};

} /* Async */
} /* beyondy */

#endif /* __EVENT_HANDLER__H */
