#include <algorithm>

#include "Handler.h"

void Handler::add(Handleable* handleable) {
    _handleables.push_back(handleable);
}

void Handler::remove(Handleable* handleable) {
    _handleables.erase(std::remove(_handleables.begin(), _handleables.end(), handleable), _handleables.end());
}

void Handler::begin() {
    for(Handleable *h : _handleables) {
        h->begin();
    }
}

void Handler::handle() {
    for(Handleable *h : _handleables) {
        h->handle();
    }
}

    