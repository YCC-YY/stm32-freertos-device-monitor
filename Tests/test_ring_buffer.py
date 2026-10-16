"""SPSC ring buffer host-side tests.

Mirrors Middleware/RingBuffer/ring_buffer.c (head/tail only, no shared count).
Usable capacity is capacity-1.
"""


class RingBuffer:
    def __init__(self, capacity: int):
        assert capacity >= 2
        self.capacity = capacity
        self.buf = [0] * capacity
        self.head = 0
        self.tail = 0

    def _next(self, idx: int) -> int:
        idx += 1
        return 0 if idx >= self.capacity else idx

    def is_empty(self) -> bool:
        return self.head == self.tail

    def is_full(self) -> bool:
        return self._next(self.head) == self.tail

    def size(self) -> int:
        if self.head >= self.tail:
            return self.head - self.tail
        return self.capacity - self.tail + self.head

    def free(self) -> int:
        return self.capacity - 1 - self.size()

    def push(self, data: int) -> bool:
        nxt = self._next(self.head)
        if nxt == self.tail:
            return False
        self.buf[self.head] = data & 0xFF
        self.head = nxt
        return True

    def pop(self):
        if self.head == self.tail:
            return None
        val = self.buf[self.tail]
        self.tail = self._next(self.tail)
        return val

    def reset(self):
        self.head = 0
        self.tail = 0


def test_push_pop_fifo():
    rb = RingBuffer(4)
    assert rb.push(1)
    assert rb.push(2)
    assert rb.pop() == 1
    assert rb.pop() == 2
    assert rb.is_empty()


def test_full_returns_false():
    rb = RingBuffer(4)  # usable = 3
    assert rb.push(10)
    assert rb.push(11)
    assert rb.push(12)
    assert rb.is_full()
    assert rb.push(13) is False
    assert rb.size() == 3


def test_empty_pop_returns_none():
    rb = RingBuffer(4)
    assert rb.pop() is None
    assert rb.is_empty()


def test_wraparound():
    rb = RingBuffer(4)
    for i in range(5):
        rb.push(i)
        assert rb.pop() == i
    assert rb.is_empty()


def test_one_write_one_read_no_duplicate():
    rb = RingBuffer(8)
    assert rb.push(ord("a"))
    assert rb.pop() == ord("a")
    assert rb.pop() is None


def test_reset():
    rb = RingBuffer(4)
    rb.push(1)
    rb.push(2)
    rb.reset()
    assert rb.is_empty()
    assert rb.size() == 0


if __name__ == "__main__":
    import sys

    fails = 0
    for name, fn in sorted(globals().items()):
        if name.startswith("test_") and callable(fn):
            try:
                fn()
                print("PASS", name)
            except Exception as exc:  # noqa: BLE001
                fails += 1
                print("FAIL", name, exc)
    print("done", "fails=", fails)
    sys.exit(1 if fails else 0)
