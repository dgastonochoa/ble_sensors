import threading

class Atomic:
    def __init__(self, init_val):
        self._val = init_val
        self._lock = threading.Lock()

    def get(self):
        with self._lock:
            return self._val

    def set(self, val):
        with self._lock:
            self._val = val
