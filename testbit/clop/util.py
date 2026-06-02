from typing import TypeVar, Generic, Callable, Any

T = TypeVar("T")

class Vector(Generic[T]):
    _data: list[T]
    _factory: Callable[[], T]

    def __init__(self, n: int = 0, factory: Callable[[], T] = lambda: None) -> None: # type: ignore
        self._factory = factory
        self._data: list[T] = [self._factory() for _ in range(n)]

    def push_back(self, value: T) -> None:
        self._data.append(value)

    def resize(self, size: int) -> None:
        if size > len(self._data):
            self._data += [self._factory() for _ in range(size - len(self._data))]
        else:
            del self._data[size:]

    def __getitem__(self, index: int) -> T:
        return self._data[index]

    def __setitem__(self, index: int, value: T) -> None:
        self._data[index] = value

    def size(self) -> int:
        return len(self._data)

    def fill(self, value: T) -> None:
        for i in range(len(self._data)):
            self._data[i] = value

    def pop_back(self) -> None:
        if len(self._data) == 0:
            raise ValueError("Cannot pop_back when vector is empty")
        del self._data[-1:]

    def __len__(self) -> int:
        return self.size()

    def __repr__(self) -> str:
        return f"Vector({", ".join([str(x) for x in self._data])})"

    def __eq__(self, other: Any) -> bool:
        if isinstance(other, Vector) and self._data == other._data:
            return True
        return False

    def data(self) -> list[T]:
        return self._data


