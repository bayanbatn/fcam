#include <pthread.h>

#include "FCam/Shot.h"
#include "FCam/Action.h"
#include "FCam/Sensor.h"

#include "Debug.h"


namespace FCam {

    pthread_mutex_t Shot::_idLock = PTHREAD_MUTEX_INITIALIZER;
    int Shot::_id = 0;

    Shot::Shot(): exposure(0), frameTime(0), gain(0), whiteBalance(5000) {
        pthread_mutex_lock(&_idLock);
        id = ++_id;
        pthread_mutex_unlock(&_idLock);

        // by default shots should result in frames
        wanted = true;
        
    };

    Shot::~Shot() {
        clearAllActions();
    }

    void Shot::clearActions(void *owner) {
        for (std::set<Action*>::iterator i = _actions.begin(); 
             i != _actions.end();) {

        	if ((*i)->owner == owner) {
        		delete *i;
        		_actions.erase(i++);
        	} else  {
        		i++;
        	}
        }
    }
    
    void Shot::clearAllActions() {
		for (std::set<Action*>::iterator i = _actions.begin();
			 i != _actions.end(); i++) {
				delete *i;
		}
		_actions.clear();
	}

    void Shot::addAction(const Action &action) {
        _actions.insert(action.copy());
    }

    Shot::Shot(const Shot &other) :
        image(other.image),
        exposure(other.exposure),
        frameTime(other.frameTime),
        gain(other.gain),
        whiteBalance(other.whiteBalance),
        histogram(other.histogram),
        sharpness(other.sharpness),
        wanted(other.wanted),
        _colorMatrix(other._colorMatrix) {

        pthread_mutex_lock(&_idLock);
        id = ++_id;
        pthread_mutex_unlock(&_idLock);

        clearAllActions();

        for (std::set<Action*>::const_iterator i = other._actions.begin(); 
             i != other._actions.end(); i++) {
            Action *a = (*i)->copy();
            _actions.insert(a);
        }        
    };


    void Shot::setColorMatrix(const float *m) {
        if (!m) {
            clearColorMatrix();
            return;
        }
        _colorMatrix.resize(12);
        for (int i = 0; i < 12; i++) _colorMatrix[i] = m[i];
    }

    void Shot::clearColorMatrix() {
        _colorMatrix.clear();
    }

    const Shot &Shot::operator=(const Shot &other) {
        clearAllActions();

        for (std::set<Action*>::const_iterator i = other._actions.begin(); 
             i != other._actions.end(); i++) {
            Action *a = (*i)->copy();
            _actions.insert(a);
        }        

        pthread_mutex_lock(&_idLock);
        id = ++_id;
        pthread_mutex_unlock(&_idLock);

        exposure = other.exposure;
        frameTime = other.frameTime;
        whiteBalance = other.whiteBalance;
        gain = other.gain;
        histogram = other.histogram;
        sharpness = other.sharpness;
        image = other.image;
        wanted = other.wanted;
        _colorMatrix = other._colorMatrix;

        return *this;
    }
};
