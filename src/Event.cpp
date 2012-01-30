#include <sstream>
#include <stdarg.h>

#include "FCam/Event.h"
#include "Debug.h"

namespace FCam {

    // Gets the next pending event. Returns false if there are no
    // outstanding events. 
    bool getNextEvent(Event *e) {        
        return _eventQueue.tryPull(e);
    }

    // Filter the event queue for specific types of events, several variants
    bool getNextEvent(Event *e, int type) {
        TSQueue<Event>::locking_iterator i = _eventQueue.begin();
        for (; i != _eventQueue.end(); i++) {
            if (i->type == type) {
                Event eTemp = *i;
                if (!_eventQueue.erase(i)) {
                    dprintf(4, "getNextEvent: Skipping 'reserved' event.\n");
                    return false;
                }
                *e = eTemp;
                return true;
            }
        }
        return false;
    }

    bool getNextEvent(Event *e, int type, int data) {
        TSQueue<Event>::locking_iterator i = _eventQueue.begin();
        for (; i != _eventQueue.end(); i++) {
            if (i->type == type && i->data == data) {
                Event eTemp = *i;
                if (!_eventQueue.erase(i)) {
                    dprintf(4, "getNextEvent: Skipping 'reserved' event.\n");
                    return false; // Can't erase == 'doesn't actually exist'
                }
                *e = eTemp;
                return true;
            }
        }
        return false;
    }

    bool getNextEvent(Event *e, int type, EventGenerator *creator) {
        TSQueue<Event>::locking_iterator i = _eventQueue.begin();
        for (; i != _eventQueue.end(); i++) {
            if (i->type == type && i->creator == creator) {
                Event eTemp = *i;
                if (!_eventQueue.erase(i)) {
                    dprintf(4, "getNextEvent: Skipping 'reserved' event.\n");
                    return false;
                }
                *e = eTemp;
                return true;
            }
        }
        return false;
    }

    bool getNextEvent(Event *e, int type, int data, EventGenerator *creator) {
        TSQueue<Event>::locking_iterator i = _eventQueue.begin();
        for (; i != _eventQueue.end(); i++) {
            if (i->type == type && i->data == data && i->creator == creator) {
                Event eTemp = *i;
                if (!_eventQueue.erase(i)) {
                    dprintf(4, "getNextEvent: Skipping 'reserved' event.\n");
                    return false; // Can't erase == 'doesn't actually exist'
                }
                *e = eTemp;
                return true;
            }
        }
        return false;
    }

    bool getNextEvent(Event *e, EventGenerator *creator) {
        TSQueue<Event>::locking_iterator i = _eventQueue.begin();
        for (; i != _eventQueue.end(); i++) {
            if (i->creator == creator) {
                Event eTemp = *i;
                if (!_eventQueue.erase(i)) {
                    dprintf(4, "getNextEvent: Skipping 'reserved' event.\n");
                    return false; // Can't erase == 'doesn't actually exist'
                }
                *e = eTemp;
                return true;
            }
        }
        return false;
    }

    void postEvent(Event e) {        
        if (e.type == Event::Error) _dprintf(DBG_ERROR, "Error (Event)", "%s\n", e.description.c_str());
        else if (e.type == Event::Warning) _dprintf(DBG_WARN, "Warning (Event)", "%s\n", e.description.c_str());
        else _dprintf(DBG_MINOR, "Event", "%s\n", e.description.c_str());
        _eventQueue.push(e);
    }

    void postEvent(int type, int data, const std::string &msg, EventGenerator *creator) {
        Event e;
        e.creator = creator;
        e.type = type;
        e.description = msg;
        e.data = data;
        e.time = Time::now();
        postEvent(e);
    }

    void error(int code, const char *fmt, ...) {
        char buf[256];
        va_list arglist;
        va_start(arglist, fmt);
        vsnprintf(buf, 256, fmt, arglist);
        va_end(arglist);
        postEvent(Event::Error, code, buf);
    }

    void warning(int code, const char *fmt, ...) {
        char buf[256];
        va_list arglist;
        va_start(arglist, fmt);
        vsnprintf(buf, 256, fmt, arglist);
        va_end(arglist);
        postEvent(Event::Warning, code, buf);
    }

    void error(int code, EventGenerator *creator, const char *fmt, ...) {
        char buf[256];
        va_list arglist;
        va_start(arglist, fmt);
        vsnprintf(buf, 256, fmt, arglist);
        va_end(arglist);
        postEvent(Event::Error, code, buf, creator);        
    }

    void warning(int code, EventGenerator *creator, const char *fmt, ...) {
        char buf[256];
        va_list arglist;
        va_start(arglist, fmt);
        vsnprintf(buf, 256, fmt, arglist);
        va_end(arglist);
        postEvent(Event::Warning, code, buf, creator);        
    }

    TSQueue<Event> _eventQueue;
}


